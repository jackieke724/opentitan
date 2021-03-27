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
  CHECK(dif_vec_dot_mode(&vec_dot, 0) == kDifVecDotOk);
  CHECK(dif_vec_dot_send_vectors_reg(&vec_dot, 0) == kDifVecDotOk);
  CHECK(dif_vec_dot_send_vectors_ram(&vec_dot, 2) == kDifVecDotOk);
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
  
  uint32_t dest[1024];
  uint32_t sum=0;
  CHECK(dif_vec_dot_dmem_read(&vec_dot, 0, dest, 1024*4) == kDifVecDotOk);
  for (uint32_t i=0; i<1024; i++) {
    sum += dest[i];
  }
  LOG_INFO("Destination array sum is %d.", sum);
  
  CHECK(dif_ddr_ctrl_init(
			      (dif_ddr_ctrl_params_t){
                .base_addr =
                    mmio_region_from_addr(TOP_EARLGREY_DDR_CTRL_BASE_ADDR),
            },
            &ddr_ctrl) == kDifDdrCtrlOk);
  LOG_INFO("Waiting for DDR calibration.");
  CHECK(dif_ddr_ctrl_init_calib(&ddr_ctrl)== kDifDdrCtrlOk);
  LOG_INFO("DDR calibration completed.");
  CHECK(dif_ddr_ctrl_write(&ddr_ctrl, 1234, 0xdeadbeef)== kDifDdrCtrlOk);
  LOG_INFO("1234 0xdeadbeef sent.");
  uint32_t data_u=0, data_l=0;
  CHECK(dif_ddr_ctrl_read(&ddr_ctrl, &data_u, &data_l)== kDifDdrCtrlOk);
  LOG_INFO("DDR read data are %d %h.", data_u, data_l);

  uint32_t gpio_state = 0;
  while (true) {
    usleep(10 * 1000);  // 10 ms
    gpio_state = demo_gpio_to_log_echo(&gpio, gpio_state);
    demo_spi_to_log_echo(&spi);
    demo_uart_to_uart_and_gpio_echo(&uart, &gpio);
  }
}
