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


void conv_test(uint32_t i){
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
  
  CHECK(dif_dla_init(
            (dif_dla_params_t){
              .base_addr = mmio_region_from_addr(TOP_EARLGREY_DLA_BASE_ADDR),
            },&dla) == kDifDlaOk);
  LOG_INFO("DLA initialized!");
  //init ddr
  CHECK(dif_dla_init_ddr(&dla) == kDifDlaOk);
  //usleep(1 * 1000);  // 1 ms
  LOG_INFO("DDR initialized!");
  
  //load weight
  CHECK(dif_dla_ddr2gb(&dla, 3-1, (uint32_t) CONV1_WEIGHT_DDR_ADDR, 
                          (uint32_t) (CONV1_WEIGHT_DDR_ADDR>>32), 
                          conv1_weight_addr, 0, 1, 0) == kDifDlaOk);
  CHECK(dif_dla_wait_for_done(&dla) == kDifDlaOk);
  LOG_INFO("Weight loaded!");
  
  //load bias
  CHECK(dif_dla_ddr2gb(&dla, 0, (uint32_t) CONV1_BIAS_DDR_ADDR, 
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
  
  uint32_t dest[34];
  CHECK(dif_dla_read_ddr(&dla, 384, dest, 34*4) == kDifDlaOk);
  LOG_INFO("Destination array is:");
  for (uint32_t i=0; i<34; i++) {
    //if (i%2==0)
    LOG_INFO("  %d %d", i, dest[i]);
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

  conv_test(1);
  
  uint32_t gpio_state = 0;
  while (true) {
    usleep(10 * 1000);  // 10 ms
    gpio_state = demo_gpio_to_log_echo(&gpio, gpio_state);
    demo_spi_to_log_echo(&spi);
    demo_uart_to_uart_and_gpio_echo(&uart, &gpio);
  }
}
