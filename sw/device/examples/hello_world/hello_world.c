// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "sw/device/examples/demos.h"
#include "sw/device/lib/arch/device.h"
#include "sw/device/lib/dif/dif_gpio.h"
#include "sw/device/lib/dif/dif_spi_device.h"
#include "sw/device/lib/dif/dif_uart.h"
#include "sw/device/lib/dif/dif_vec_dot.h"
#include "sw/device/lib/dif/dif_ddr_ctrl.h"
#include "sw/device/lib/pinmux.h"
#include "sw/device/lib/runtime/hart.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/runtime/print.h"
#include "sw/device/lib/testing/check.h"
#include "sw/device/lib/testing/test_status.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"  // Generated.

static dif_gpio_t gpio;
static dif_spi_device_t spi;
static dif_uart_t uart;
static dif_vec_dot_t vec_dot;
static dif_ddr_ctrl_t ddr_ctrl;

const uint32_t DLA_DDR_BURST_MAX = 2048; //bytes
const uint32_t SPI_RXF_SIZE = 1024; //bytes
const uint32_t NUM_SPI_PATCH = 38;//4;
//WEIGHT_SIZE: minimum grain-size is 32B/256 bits
//which is the width of DDR on Genesys 2
const uint32_t WEIGHT_SIZE = NUM_SPI_PATCH*SPI_RXF_SIZE+320; //bytes

//src: source array
//len: number of bytes
void convert_endian(void * src, size_t len) {
  //reverse byte-order
  //for each src[i]
  //before: 0x 67 45 23 01 
  //after:  0x 01 23 45 67
  for (uint32_t i=0; i<(len>>2); i++) {
    //LOG_INFO("DMEM i %d: %x.", i, ((uint8_t *)src)[i*4]);
    //LOG_INFO("DMEM i %d: %x.", i, ((uint8_t *)src)[i*4+1]);
    //LOG_INFO("DMEM i %d: %x.", i, ((uint8_t *)src)[i*4+2]);
    //LOG_INFO("DMEM i %d: %x.", i, ((uint8_t *)src)[i*4+3]);
    ((uint32_t *)src)[i] =  ((uint8_t *)src)[i*4]   << 24 |
                            ((uint8_t *)src)[i*4+1] << 16 |
                            ((uint8_t *)src)[i*4+2] << 8  |
                            ((uint8_t *)src)[i*4+3];
  }
}


void ddr_test(void) {
  ///*
  //read from SPI and write to DDR
  uint32_t buffer[SPI_RXF_SIZE/4]; //used for receiving from spi
  for(uint32_t i =0; i<NUM_SPI_PATCH; i++){
    
    size_t spi_len=0, spi_total_recv=0;
    uint32_t ddr_start_addr = i*(SPI_RXF_SIZE/8); //64-bit/double-word addressable
    
    
    //Prepare echo
    size_t bytes=0; 
    //SPI will reverse back the byte-order
    uint32_t echo_word = buffer[0] ^ 0x01010101; 
    CHECK(dif_spi_device_send(&spi, &echo_word, sizeof(uint32_t), &bytes) == kDifSpiDeviceOk);
    LOG_INFO("Echo send: %d bytes, %x to SPI host.", bytes, echo_word);
    /*
    uint32_t echo_buf[8];
    do {
      CHECK(dif_spi_device_recv(&spi, echo_buf, sizeof(echo_buf), &bytes) == kDifSpiDeviceOk);
      if (bytes > 0)
        LOG_INFO("Echo receive: %d bytes, %x from SPI host.", bytes, echo_buf[0]);
    } while (echo_buf[0] != echo_word);
    */
    
    LOG_INFO("Patch %d: waiting for SPI input.", i);
    while (spi_total_recv < SPI_RXF_SIZE){
      CHECK(dif_spi_device_recv(&spi, (uint8_t *)buffer+spi_total_recv, sizeof(buffer)-spi_total_recv, &spi_len) ==
            kDifSpiDeviceOk);
      
      if (spi_len > 0){
        spi_total_recv += spi_len;
        LOG_INFO("SPI has received %d bytes.", spi_len);
      }
    }
    
    LOG_INFO("SPI has received total %d bytes.", spi_total_recv);
    if (spi_total_recv != SPI_RXF_SIZE) LOG_INFO("ERROR! TOTAL RECEIVED BYTES IS NOT %d.", SPI_RXF_SIZE);
    
    convert_endian(buffer, spi_total_recv);
    //for (uint32_t i=0; i<(spi_total_recv>>2); i++) {
    //  LOG_INFO("SPI buffer i %d: %x.", i, buffer[i]);
    //}
    
    CHECK(dif_ddr_ctrl_write_buf(&ddr_ctrl, ddr_start_addr, buffer, spi_total_recv)== kDifDdrCtrlOk);
    LOG_INFO("DDR has written the buffer content.");
    
  }
  //*/
  
  //CHECK(dif_ddr_ctrl_write(&ddr_ctrl, 0x12340000, 0xff000000)== kDifDdrCtrlOk);//simple 8 64-bit data counted from the inputs
  
  //read from DDR
  uint32_t bytes_left = WEIGHT_SIZE;
  uint32_t patch=0;
  
  while (bytes_left>0) {
    uint32_t dest_ddr[DLA_DDR_BURST_MAX/4];
    uint32_t data_u=0, data_l=0, wptr=0;
    uint32_t bytes_read = (bytes_left>DLA_DDR_BURST_MAX) ?DLA_DDR_BURST_MAX :bytes_left;
    uint32_t ddr_start_addr = patch*(DLA_DDR_BURST_MAX/8); //64-bit/double-word addressable
    LOG_INFO("Patch %d, total left: %d, read %d, ddr_start_addr %d.", patch, bytes_left, bytes_read, ddr_start_addr);
    
    CHECK(dif_ddr_ctrl_read(&ddr_ctrl, ddr_start_addr, &data_u, &data_l, dest_ddr, bytes_read, &wptr)== kDifDdrCtrlOk);
    LOG_INFO("DDR read data are %x %x.", data_u, data_l);
    LOG_INFO("DMEM write pointer is at %d.", wptr);


    //send to SPI
    LOG_INFO("Patch %d: Writing to SPI.", patch);
    size_t spi_len=0, spi_total_send=0;
    while (spi_total_send < bytes_read){
      CHECK(dif_spi_device_send(&spi, (uint8_t *)dest_ddr+spi_total_send, sizeof(dest_ddr)-spi_total_send, &spi_len) ==
            kDifSpiDeviceOk);
      
      if (spi_len > 0){
        spi_total_send += spi_len;
        LOG_INFO("SPI has sent %d bytes, remaining %d bytes", spi_len, sizeof(dest_ddr)-spi_total_send);
      }
    }
    /*
    for (uint32_t i=0; i<(bytes_read>>3); i++) {
      //LOG_INFO("DMEM i %d: %x.", i, ((uint8_t *)buffer)[i]);
      LOG_INFO("DMEM i %d: %x %x.", i+patch*(DLA_DDR_BURST_MAX/8), dest_ddr[i*2+1], dest_ddr[i*2]);
    }*/
    
    bytes_left -= bytes_read;
    patch++;
  }
  LOG_INFO("DDR-SPI test finished.");
}


int main(int argc, char **argv) {
  CHECK(dif_uart_init(
            (dif_uart_params_t){
                .base_addr = mmio_region_from_addr(TOP_EARLGREY_UART_BASE_ADDR),
            },
            &uart) == kDifUartOk);
  CHECK(dif_uart_configure(&uart, (dif_uart_config_t){
                                      .baudrate = kUartBaudrate,
                                      .clk_freq_hz = kClockFreqPeripheralHz,
                                      .parity_enable = kDifUartToggleDisabled,
                                      .parity = kDifUartParityEven,
                                  }) == kDifUartConfigOk);
  base_uart_stdout(&uart);

  pinmux_init();

  CHECK(dif_spi_device_init(
            (dif_spi_device_params_t){
                .base_addr =
                    mmio_region_from_addr(TOP_EARLGREY_SPI_DEVICE_BASE_ADDR),
            },
            &spi) == kDifSpiDeviceOk);
  CHECK(dif_spi_device_configure(
            &spi, (dif_spi_device_config_t){
                      .clock_polarity = kDifSpiDeviceEdgePositive,
                      .data_phase = kDifSpiDeviceEdgeNegative,
                      .tx_order = kDifSpiDeviceBitOrderMsbToLsb,
                      .rx_order = kDifSpiDeviceBitOrderMsbToLsb,
                      .rx_fifo_timeout = 63,
                      .rx_fifo_len = kDifSpiDeviceBufferLen / 2,
                      .tx_fifo_len = kDifSpiDeviceBufferLen / 2,
                  }) == kDifSpiDeviceOk);

  dif_gpio_params_t gpio_params = {
      .base_addr = mmio_region_from_addr(TOP_EARLGREY_GPIO_BASE_ADDR),
  };
  CHECK(dif_gpio_init(gpio_params, &gpio) == kDifGpioOk);
  // Enable GPIO: 0-7 and 16 is input; 8-15 is output.
  CHECK(dif_gpio_output_set_enabled_all(&gpio, 0x0ff00) == kDifGpioOk);

  // Add DATE and TIME because I keep fooling myself with old versions
  LOG_INFO("Hello World!");
  LOG_INFO("Built at: " __DATE__ ", " __TIME__);

  demo_gpio_startup(&gpio);

  // Now have UART <-> Buttons/LEDs demo
  // all LEDs off
  CHECK(dif_gpio_write_all(&gpio, 0x0000) == kDifGpioOk);
  LOG_INFO("Try out the switches on the board");
  LOG_INFO("or type anything into the console window.");
  LOG_INFO("The LEDs show the ASCII code of the last character.");

  CHECK(dif_spi_device_send(&spi, "SPI!", 4, /*bytes_sent=*/NULL) ==
        kDifSpiDeviceOk);
  
  
  CHECK(dif_vec_dot_init(
			      (dif_vec_dot_params_t){
                .base_addr =
                    mmio_region_from_addr(TOP_EARLGREY_VEC_DOT_BASE_ADDR),
            },
            &vec_dot) == kDifVecDotOk);
  CHECK(dif_vec_dot_mode(&vec_dot, 1) == kDifVecDotOk);
  CHECK(dif_vec_dot_send_vectors_reg(&vec_dot, 0) == kDifVecDotOk);
  CHECK(dif_vec_dot_send_vectors_ram(&vec_dot, 0) == kDifVecDotOk);
  CHECK(dif_vec_dot_start(&vec_dot) == kDifVecDotOk);
  LOG_INFO("Vector inner product has started.");
  bool busy = true;
  while(busy){
    LOG_INFO("Vector inner product is busy.");
    CHECK(dif_vec_dot_is_busy(&vec_dot, &busy) == kDifVecDotOk);
  }
  uint32_t result;
  CHECK(dif_vec_dot_read(&vec_dot, &result) == kDifVecDotOk);
  LOG_INFO("Vector inner product is %d.", result);
  /*
  uint32_t dest[1024];
  uint32_t sum=0;
  CHECK(dif_vec_dot_dmem_read(&vec_dot, 0, dest, 1024*4) == kDifVecDotOk);
  for (uint32_t i=0; i<1024; i++) {
    sum += dest[i];
  }
  LOG_INFO("Destination array sum is %d.", sum);
  */
  CHECK(dif_ddr_ctrl_init(
			      (dif_ddr_ctrl_params_t){
                .base_addr =
                    mmio_region_from_addr(TOP_EARLGREY_DDR_CTRL_BASE_ADDR),
            },
            &ddr_ctrl) == kDifDdrCtrlOk);
  LOG_INFO("Waiting for DDR calibration.");
  CHECK(dif_ddr_ctrl_init_calib(&ddr_ctrl)== kDifDdrCtrlOk);
  LOG_INFO("DDR calibration completed.");
  
  ddr_test();
  

  uint32_t gpio_state = 0;
  while (true) {
    usleep(10 * 1000);  // 10 ms
    gpio_state = demo_gpio_to_log_echo(&gpio, gpio_state);
    //demo_spi_to_log_echo(&spi);
    demo_uart_to_uart_and_gpio_echo(&uart, &gpio);
  }
}
