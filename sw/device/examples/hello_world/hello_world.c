// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "sw/device/examples/demos.h"
#include "sw/device/lib/arch/device.h"
#include "sw/device/lib/dif/dif_gpio.h"
#include "sw/device/lib/dif/dif_spi_device.h"
#include "sw/device/lib/dif/dif_uart.h"
#include "sw/device/lib/dif/dif_dla.h"
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
static dif_dla_t dla;

const uint32_t DLA_DDR_BURST_MAX = 2048; //bytes
const uint32_t SPI_RXF_SIZE = 1024; //bytes
const uint32_t NUM_SPI_PATCH = 4;//weight_512.txt
//const uint32_t NUM_SPI_PATCH = 38;//weight.txt 4864 lines

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
    
    CHECK(dif_dla_ddr_ctrl_write_buf(&dla, ddr_start_addr, buffer, spi_total_recv)== kDifDlaOk);
    LOG_INFO("DDR has written the buffer content.");
    
  }
  //*/
  
  //CHECK(dif_ddr_ctrl_write(&dla, 0x12340000, 0xff000000)== kDifDdrCtrlOk);//simple 8 64-bit data counted from the inputs
  
  //read from DDR
  uint32_t bytes_left = WEIGHT_SIZE;
  uint32_t patch=0;
  
  while (bytes_left>0) {
    uint32_t dest_ddr[DLA_DDR_BURST_MAX/4];
    uint32_t wptr=0;
    uint32_t bytes_read = (bytes_left>DLA_DDR_BURST_MAX) ?DLA_DDR_BURST_MAX :bytes_left;
    uint32_t ddr_start_addr = patch*(DLA_DDR_BURST_MAX/8); //64-bit/double-word addressable
    LOG_INFO("Patch %d, total left: %d, read %d, ddr_start_addr %d.", patch, bytes_left, bytes_read, ddr_start_addr);
    
    CHECK(dif_dla_ddr_ctrl_read(&dla, ddr_start_addr, dest_ddr, bytes_read, &wptr)== kDifDlaOk);
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
      LOG_INFO("DMEM i %d: %x %x.", i+patch*(DLA_DDR_BURST_MAX/8), dest_ddr[i*2+1], dest_ddr[i*2]);
    }*/
    
    bytes_left -= bytes_read;
    patch++;
  }
  LOG_INFO("DDR-SPI test finished.");
}


void conv_test(void){
  uint64_t CONV1_IFMAP_DDR_ADDR     =   0x00000;  // 1x14 -> 1x16 / 4 = 4 total address 0x0004
  uint64_t CONV1_WEIGHT_DDR_ADDR    =   0x00010;  // 1 x 3 x 16 / 8 = 2 address 
  uint64_t CONV1_BIAS_DDR_ADDR      =   0x00020;  // 1 / 8 = 1 = address
  uint64_t CONV1_OFMAP_DDR_ADDR     =   0x00030;  // 1 x 16 / 4 = 4 address
  
  uint32_t conv1_ifmap_addr = 0x0000;
  uint32_t conv1_ofmap_addr = 0x0010;  // (7 ICs each FBUF) 7 * 130t = 910 < 912d = 0x390
  // BIAS ABUF address
  uint32_t conv1_bias_addr  = 0x0000;
  // WEIGHT WBUF address
  uint32_t conv1_weight_addr = 0x0000;
  
  //init ddr
  //CHECK(dif_dla_init_ddr(&dla) == kDifDlaOk);
  //usleep(1 * 1000);  // 1 ms

  uint32_t wptr=0;

  uint32_t src[72]; //multiple of 32 bytes
  //feature
  for (uint32_t i=0; i<16; i++) { //0-31
    src[i*2]    = 0;
    src[i*2+1]  = i;
  }
  for (uint32_t i=0; i<40; i++) { //32-71
    src[32+i] = 0;
  }
  //weight
  src[32]=0; src[33]=1;
  src[34]=0; src[35]=1;
  src[36]=0; src[37]=0;
  //bias
  src[64]=0;
  src[65]=0x45a6;
  CHECK(dif_dla_ddr_ctrl_write_buf(&dla, 0, src, 4*72)== kDifDlaOk);
  
  LOG_INFO("DDR initialized!");
  CHECK(dif_dla_ddr_ctrl_wptr(&dla, &wptr)== kDifDlaOk);
  LOG_INFO("WPTR %d", wptr);
  
  //load weight
  CHECK(dif_dla_ddr2gb(&dla, 8-1, (uint32_t) CONV1_WEIGHT_DDR_ADDR, 
                          (uint32_t) (CONV1_WEIGHT_DDR_ADDR>>32), 
                          conv1_weight_addr, 0, 1, 0) == kDifDlaOk);
  CHECK(dif_dla_wait_for_done(&dla) == kDifDlaOk);
  LOG_INFO("Weight loaded!");
  
  //load bias
  CHECK(dif_dla_ddr2gb(&dla, 8-1, (uint32_t) CONV1_BIAS_DDR_ADDR, 
                          (uint32_t) (CONV1_BIAS_DDR_ADDR>>32), 
                          conv1_bias_addr, 0, 3, 0) == kDifDlaOk);
  CHECK(dif_dla_wait_for_done(&dla) == kDifDlaOk);
  LOG_INFO("Bias loaded!");
  
  //load feature
  CHECK(dif_dla_ddr2gb(&dla, 16-1, (uint32_t) CONV1_IFMAP_DDR_ADDR, 
                          (uint32_t) (CONV1_IFMAP_DDR_ADDR>>32), 
                          conv1_ifmap_addr, 0, 0, 0) == kDifDlaOk);
  CHECK(dif_dla_wait_for_done(&dla) == kDifDlaOk);
  LOG_INFO("Feature loaded!");
  
  //fbuf to lbuf
  CHECK(dif_dla_fbuf2lbuf(&dla, conv1_ifmap_addr, 0, 1, 0, 16-1, 1, 1) == kDifDlaOk);
  CHECK(dif_dla_wait_for_done(&dla) == kDifDlaOk);
  LOG_INFO("Fbuf to Lbuf!");
  
  //conv
  CHECK(dif_dla_set_ppe(&dla, 0x0001, 0x0001) == kDifDlaOk);
  CHECK(dif_dla_ppe_reset(&dla) == kDifDlaOk);
  CHECK(dif_dla_conv(&dla, 0, 14, 3, 17, 15, 1, 1, 1, 1, 0, 1, 1, 0, conv1_weight_addr, 0) == kDifDlaOk);
  CHECK(dif_dla_wait_for_done(&dla) == kDifDlaOk);
  CHECK(dif_dla_ppe_psum_accum(&dla, 15, 0) == kDifDlaOk);
  CHECK(dif_dla_wait_for_done(&dla) == kDifDlaOk);
  LOG_INFO("Convolution!");
  
  //bias
  CHECK(dif_dla_ppe_comp(&dla, 0, 1, 0, 0, 0, 16, 2, 0, 2, 0, 
                            conv1_ofmap_addr, conv1_bias_addr, 1, 1, 1, 1) == kDifDlaOk);
  CHECK(dif_dla_wait_for_done(&dla) == kDifDlaOk);
  LOG_INFO("Bias!");
  
  //fbuf to ddr
  CHECK(dif_dla_ddr2gb(&dla, 16-1, (uint32_t) CONV1_OFMAP_DDR_ADDR, 
                          (uint32_t) (CONV1_OFMAP_DDR_ADDR>>32), 
                          conv1_ofmap_addr, 0, 0, 1) == kDifDlaOk);
  CHECK(dif_dla_wait_for_done(&dla) == kDifDlaOk);
  LOG_INFO("Fbuf to DDR!");
  /*
  uint32_t dest[34];
  //CHECK(dif_dla_read_ddr(&dla, 384, dest, 34*4) == kDifDlaOk);
  
  uint32_t wptr=0;
  CHECK(dif_dla_ddr_ctrl_read(&dla, (uint32_t) CONV1_OFMAP_DDR_ADDR, dest, 32*4, &wptr)== kDifDlaOk);
  dest[32] = 0; dest[33] = 0;

  LOG_INFO("Destination array is:");
  for (uint32_t i=0; i<34; i++) {
    //if (i%2==0)
    LOG_INFO("  %d %d", i, dest[i]);
  }*/

  //read from DDR
  //read from ddr addr: 0x0 to 0x3f, we need 64*8 + 32 bytes (due to the 1 ddr write ack)
  uint32_t bytes_left = 512 + 32;
  uint32_t patch=0;
  
  while (bytes_left>0) {
    uint32_t dest_ddr[DLA_DDR_BURST_MAX/4];
    //uint32_t wptr=0;
    uint32_t bytes_read = (bytes_left>DLA_DDR_BURST_MAX) ?DLA_DDR_BURST_MAX :bytes_left;
    uint32_t ddr_start_addr = patch*(DLA_DDR_BURST_MAX/8); //64-bit/double-word addressable
    LOG_INFO("Patch %d, total left: %d, read %d, ddr_start_addr %d.", patch, bytes_left, bytes_read, ddr_start_addr);
    
    CHECK(dif_dla_ddr_ctrl_read(&dla, ddr_start_addr, dest_ddr, bytes_read, &wptr)== kDifDlaOk);
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
      LOG_INFO("DMEM i %d: %x %x.", i+patch*(DLA_DDR_BURST_MAX/8), dest_ddr[i*2+1], dest_ddr[i*2]);
    }*/
    
    bytes_left -= bytes_read;
    patch++;
  }
 
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

  CHECK(dif_dla_init(
            (dif_dla_params_t){
              .base_addr = mmio_region_from_addr(TOP_EARLGREY_DLA_BASE_ADDR),
            },&dla) == kDifDlaOk);
  LOG_INFO("DLA initialized!");

  LOG_INFO("Waiting for DDR calibration.");
  CHECK(dif_dla_ddr_ctrl_init_calib(&dla)== kDifDlaOk);
  LOG_INFO("DDR calibration completed.");
  
  //ddr_test();

  conv_test();
  
  uint32_t gpio_state = 0;
  while (true) {
    usleep(10 * 1000);  // 10 ms
    gpio_state = demo_gpio_to_log_echo(&gpio, gpio_state);
    demo_spi_to_log_echo(&spi);
    demo_uart_to_uart_and_gpio_echo(&uart, &gpio);
  }
}
