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
#include "sw/device/lib/ddr_spi.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"  // Generated.

static dif_gpio_t gpio;
static dif_spi_device_t spi;
static dif_uart_t uart;
static dif_dla_t dla;

//python script to send ddr contents from spi
//python3 util/simplespi/spitest.py -i wavenet_ddr_in.txt

//python script to receive same ddr contents and send to spi
//python3 util/simplespi/spitest.py -r 1024000


//used for ddr_spi functions
//DDRIN_LINES needs to be divisible by 128,
//or DDRIN_SIZE needs to be divisible by 1024,
//so SPI device can send integer patches of 1024B
const uint32_t DDRIN_LINES = 128000; //lstm_ddr_in.txt
const uint32_t DDRIN_SIZE = DDRIN_LINES * 8 ; //bytes

//used for reading wavenet result
//ddr read start address
const uint32_t RD_DDR_ADDR = 130000; //64-bit addressable in unsigned decimal
//ddr read bytes
const uint32_t RD_DDR_BYTES = 65536; //bytes


void wavenet(void){
  
  uint32_t act_k_tanh[16] = {0x157e, 0x1bf8, 0x21c4, 0x2825, 0x2dd6, 0x33c6, 0x3878, 0x3b75,
                      0x3b75, 0x3878, 0x33c6, 0x2dd6, 0x2825, 0x21c4, 0x1bf8, 0x157e};
  uint32_t act_b_tanh[16] = {0xbbf3, 0xbbe0, 0xbbb0, 0xbb3f, 0xba43, 0xb85e, 0xb2ef, 0xa21e,
                      0x221e, 0x32ef, 0x385e, 0x3a43, 0x3b3f, 0x3bb0, 0x3be0, 0x3bf3};
  uint32_t act_x_tanh[15] = {0xc377, 0xc266, 0xc155, 0xc044, 0xbe66, 0xbc44, 0xb844, 0x0000,
                      0x3844, 0x3c44, 0x3e66, 0x4044, 0x4155, 0x4266, 0x4377};

  uint32_t act_k_sigmoid[16] = {0x190d, 0x1d96, 0x2222, 0x26a0, 0x2aea, 0x2eb8, 0x31b1, 0x33b0,
                           0x33b0, 0x31b1, 0x2eb8, 0x2aea, 0x26a0, 0x2222, 0x1d96, 0x190d};
  uint32_t act_b_sigmoid[16] = {0x246b, 0x2855, 0x2c26, 0x2faa, 0x32ad, 0x3544, 0x371e, 0x37f5,
                           0x3805, 0x3870, 0x395d, 0x3a54, 0x3b0a, 0x3b7b, 0x3bba, 0x3bdc};
  uint32_t act_x_sigmoid[15] = {0xc599, 0xc4cc, 0xc3ff, 0xc266, 0xc0cc, 0xbe66, 0xba65, 0x0000,
                           0x3a65, 0x3e66, 0x40cc, 0x4266, 0x43ff, 0x44cc, 0x4599};

  uint32_t act_k_dummy[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t act_b_dummy[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t act_x_dummy[15] = {0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0};

  dif_dla_pbuf_rst(&dla, 64);

  // block0-0
  LOG_INFO("gpio[0] = 1;");

  dif_dla_move_ddr2gb(&dla, 960, 0, 0,   // baseaddr, len, ddr_addr, gb_addr 
              0, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 960, 0,   // baseaddr, len, ddr_addr, gb_addr 
              1, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 1920, 0,   // baseaddr, len, ddr_addr, gb_addr 
              2, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 2880, 0,   // baseaddr, len, ddr_addr, gb_addr 
              3, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 3840, 0,   // baseaddr, len, ddr_addr, gb_addr 
              4, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 4800, 0,   // baseaddr, len, ddr_addr, gb_addr 
              5, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 5760, 0,   // baseaddr, len, ddr_addr, gb_addr 
              6, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 6720, 0,   // baseaddr, len, ddr_addr, gb_addr 
              7, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 7680, 0,   // baseaddr, len, ddr_addr, gb_addr 
              8, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 8640, 0,   // baseaddr, len, ddr_addr, gb_addr 
              9, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 9600, 0,   // baseaddr, len, ddr_addr, gb_addr 
              10, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 10560, 0,   // baseaddr, len, ddr_addr, gb_addr 
              11, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 11520, 0,   // baseaddr, len, ddr_addr, gb_addr 
              12, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 12480, 0,   // baseaddr, len, ddr_addr, gb_addr 
              13, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 13440, 0,   // baseaddr, len, ddr_addr, gb_addr 
              14, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 14400, 0,   // baseaddr, len, ddr_addr, gb_addr 
              15, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 15360, 0,   // baseaddr, len, ddr_addr, gb_addr 
              16, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 16320, 0,   // baseaddr, len, ddr_addr, gb_addr 
              17, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 17280, 0,   // baseaddr, len, ddr_addr, gb_addr 
              18, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 18240, 0,   // baseaddr, len, ddr_addr, gb_addr 
              19, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 19200, 0,   // baseaddr, len, ddr_addr, gb_addr 
              20, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 20160, 0,   // baseaddr, len, ddr_addr, gb_addr 
              21, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 21120, 0,   // baseaddr, len, ddr_addr, gb_addr 
              22, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 22080, 0,   // baseaddr, len, ddr_addr, gb_addr 
              23, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 23040, 0,   // baseaddr, len, ddr_addr, gb_addr 
              24, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 24000, 0,   // baseaddr, len, ddr_addr, gb_addr 
              25, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 24960, 0,   // baseaddr, len, ddr_addr, gb_addr 
              26, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 25920, 0,   // baseaddr, len, ddr_addr, gb_addr 
              27, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 26880, 0,   // baseaddr, len, ddr_addr, gb_addr 
              28, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 27840, 0,   // baseaddr, len, ddr_addr, gb_addr 
              29, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 28800, 0,   // baseaddr, len, ddr_addr, gb_addr 
              30, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 29760, 0,   // baseaddr, len, ddr_addr, gb_addr 
              31, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 1024, 30720, 0,   // baseaddr, len, ddr_addr, gb_addr 
              0, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 1024, 31744, 0,   // baseaddr, len, ddr_addr, gb_addr 
              1, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 1024, 32768, 0,   // baseaddr, len, ddr_addr, gb_addr 
              2, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 1024, 33792, 0,   // baseaddr, len, ddr_addr, gb_addr 
              3, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 34816, 0,   // baseaddr, len, ddr_addr, gb_addr 
              4, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 34840, 0,   // baseaddr, len, ddr_addr, gb_addr 
              5, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 34864, 0,   // baseaddr, len, ddr_addr, gb_addr 
              6, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 34888, 0,   // baseaddr, len, ddr_addr, gb_addr 
              7, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 34912, 0,   // baseaddr, len, ddr_addr, gb_addr 
              0, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 34936, 0,   // baseaddr, len, ddr_addr, gb_addr 
              1, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 34960, 0,   // baseaddr, len, ddr_addr, gb_addr 
              2, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 34984, 0,   // baseaddr, len, ddr_addr, gb_addr 
              3, 3, 0);                      // gb_idx, gb_mux, direction
  // Computing conv_filter
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
                 1, 7, 63, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 69, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 0, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 1024, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 69, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 56, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 1,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 1025, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 69, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 112, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 2,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 1026, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 69, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 168, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 3,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 1027, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 69, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 224, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 1028, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 69, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 280, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 5,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 1029, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 69, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 336, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 6,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 1030, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 69, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 392, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 7,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 1031, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_gate
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
                 1, 7, 63, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 69, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 448, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 1,                           // len, iter, post, mode, oper
      1024, 1536, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 8, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 69, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 504, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 9,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 1,                           // len, iter, post, mode, oper
      1025, 1537, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 8, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 69, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 560, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 10,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 1,                           // len, iter, post, mode, oper
      1026, 1538, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 8, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 69, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 616, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 1,                           // len, iter, post, mode, oper
      1027, 1539, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 8, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 69, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 672, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 1,                           // len, iter, post, mode, oper
      1028, 1540, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 8, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 69, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 728, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 13,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 1,                           // len, iter, post, mode, oper
      1029, 1541, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 8, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 69, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 784, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 14,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 1,                           // len, iter, post, mode, oper
      1030, 1542, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 8, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 69, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 840, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 15,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 1,                           // len, iter, post, mode, oper
      1031, 1543, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 8, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out
// t:0-63, oc:0-0, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 1536, 0,  // baseaddr, src_addr, dest_addr
                 1, 7, 63, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 63, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 896, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 16,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 1024, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 2,                           // len, iter, post, mode, oper
      0, 1536, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 8, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out
// t:0-63, oc:1-1, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 63, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 904, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 17,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 1025, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 2,                           // len, iter, post, mode, oper
      1, 1537, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 8, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out
// t:0-63, oc:2-2, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 63, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 912, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 18,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 1026, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 2,                           // len, iter, post, mode, oper
      2, 1538, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 8, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out
// t:0-63, oc:3-3, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 63, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 920, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 19,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 1027, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 2,                           // len, iter, post, mode, oper
      3, 1539, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 8, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out
// t:0-63, oc:4-4, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 63, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 928, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 20,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 1028, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 2,                           // len, iter, post, mode, oper
      4, 1540, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 8, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out
// t:0-63, oc:5-5, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 63, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 936, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 21,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 1029, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 2,                           // len, iter, post, mode, oper
      5, 1541, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 8, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out
// t:0-63, oc:6-6, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 63, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 944, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 22,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 1030, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 2,                           // len, iter, post, mode, oper
      6, 1542, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 8, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out
// t:0-63, oc:7-7, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 63, 63, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 952, 0x00, 15, 63       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 23,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 1031, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 64, 1, 0, 2,                           // len, iter, post, mode, oper
      7, 1543, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 8, 8,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:0
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      512, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      1024, 0, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:1
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      576, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      1088, 64, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:2
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      640, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      1152, 128, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:3
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      704, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      1216, 192, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:4
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      768, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      1280, 256, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:5
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      832, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      1344, 320, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:6
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      896, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      1408, 384, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:7
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      960, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      1472, 448, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // block0-1
  LOG_INFO("gpio[0] = 6;");

  dif_dla_move_ddr2gb(&dla, 960, 35008, 0,   // baseaddr, len, ddr_addr, gb_addr 
              0, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 35968, 0,   // baseaddr, len, ddr_addr, gb_addr 
              1, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 36928, 0,   // baseaddr, len, ddr_addr, gb_addr 
              2, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 37888, 0,   // baseaddr, len, ddr_addr, gb_addr 
              3, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 38848, 0,   // baseaddr, len, ddr_addr, gb_addr 
              4, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 39808, 0,   // baseaddr, len, ddr_addr, gb_addr 
              5, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 40768, 0,   // baseaddr, len, ddr_addr, gb_addr 
              6, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 41728, 0,   // baseaddr, len, ddr_addr, gb_addr 
              7, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 42688, 0,   // baseaddr, len, ddr_addr, gb_addr 
              8, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 43648, 0,   // baseaddr, len, ddr_addr, gb_addr 
              9, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 44608, 0,   // baseaddr, len, ddr_addr, gb_addr 
              10, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 45568, 0,   // baseaddr, len, ddr_addr, gb_addr 
              11, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 46528, 0,   // baseaddr, len, ddr_addr, gb_addr 
              12, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 47488, 0,   // baseaddr, len, ddr_addr, gb_addr 
              13, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 48448, 0,   // baseaddr, len, ddr_addr, gb_addr 
              14, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 49408, 0,   // baseaddr, len, ddr_addr, gb_addr 
              15, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 50368, 0,   // baseaddr, len, ddr_addr, gb_addr 
              16, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 51328, 0,   // baseaddr, len, ddr_addr, gb_addr 
              17, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 52288, 0,   // baseaddr, len, ddr_addr, gb_addr 
              18, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 53248, 0,   // baseaddr, len, ddr_addr, gb_addr 
              19, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 54208, 0,   // baseaddr, len, ddr_addr, gb_addr 
              20, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 55168, 0,   // baseaddr, len, ddr_addr, gb_addr 
              21, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 56128, 0,   // baseaddr, len, ddr_addr, gb_addr 
              22, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 57088, 0,   // baseaddr, len, ddr_addr, gb_addr 
              23, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 58048, 0,   // baseaddr, len, ddr_addr, gb_addr 
              24, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 59008, 0,   // baseaddr, len, ddr_addr, gb_addr 
              25, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 59968, 0,   // baseaddr, len, ddr_addr, gb_addr 
              26, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 60928, 0,   // baseaddr, len, ddr_addr, gb_addr 
              27, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 61888, 0,   // baseaddr, len, ddr_addr, gb_addr 
              28, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 62848, 0,   // baseaddr, len, ddr_addr, gb_addr 
              29, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 63808, 0,   // baseaddr, len, ddr_addr, gb_addr 
              30, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 64768, 0,   // baseaddr, len, ddr_addr, gb_addr 
              31, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 65728, 0,   // baseaddr, len, ddr_addr, gb_addr 
              4, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 65752, 0,   // baseaddr, len, ddr_addr, gb_addr 
              5, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 65776, 0,   // baseaddr, len, ddr_addr, gb_addr 
              6, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 65800, 0,   // baseaddr, len, ddr_addr, gb_addr 
              7, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 65824, 0,   // baseaddr, len, ddr_addr, gb_addr 
              0, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 65848, 0,   // baseaddr, len, ddr_addr, gb_addr 
              1, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 65872, 0,   // baseaddr, len, ddr_addr, gb_addr 
              2, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 65896, 0,   // baseaddr, len, ddr_addr, gb_addr 
              3, 3, 0);                      // gb_idx, gb_mux, direction
  // Computing conv_filter  dila_idx:0
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_move_fbuf2lbuf(&dla, 1536, 0,  // baseaddr, src_addr, dest_addr
                 1, 15, 31, 16, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 0, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 512, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 56, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 1,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 513, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 112, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 2,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 514, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 168, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 3,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 515, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 224, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 516, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 280, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 5,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 517, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 336, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 6,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 518, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 392, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 7,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 519, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 0, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 520, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 56, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 1,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 521, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 112, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 2,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 522, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 168, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 3,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 523, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 224, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 524, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 280, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 5,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 525, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 336, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 6,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 526, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 392, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 7,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 527, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_move_fbuf2lbuf(&dla, 1536, 0,  // baseaddr, src_addr, dest_addr
                 1, 15, 31, 16, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 448, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 1,                           // len, iter, post, mode, oper
      512, 1024, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 504, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 9,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 1,                           // len, iter, post, mode, oper
      513, 1025, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 560, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 10,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 1,                           // len, iter, post, mode, oper
      514, 1026, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 616, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 1,                           // len, iter, post, mode, oper
      515, 1027, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 672, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 1,                           // len, iter, post, mode, oper
      516, 1028, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 728, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 13,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 1,                           // len, iter, post, mode, oper
      517, 1029, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 784, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 14,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 1,                           // len, iter, post, mode, oper
      518, 1030, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 840, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 15,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 1,                           // len, iter, post, mode, oper
      519, 1031, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 448, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 1,                           // len, iter, post, mode, oper
      520, 1032, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 504, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 9,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 1,                           // len, iter, post, mode, oper
      521, 1033, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 560, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 10,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 1,                           // len, iter, post, mode, oper
      522, 1034, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 616, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 1,                           // len, iter, post, mode, oper
      523, 1035, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 672, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 1,                           // len, iter, post, mode, oper
      524, 1036, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 728, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 13,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 1,                           // len, iter, post, mode, oper
      525, 1037, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 784, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 14,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 1,                           // len, iter, post, mode, oper
      526, 1038, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 37, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 840, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 15,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 1,                           // len, iter, post, mode, oper
      527, 1039, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:0-0, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 1024, 0,  // baseaddr, src_addr, dest_addr
                 1, 15, 31, 16, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 31, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 896, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 16,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 512, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 2,                           // len, iter, post, mode, oper
      1536, 1024, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:1-1, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 31, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 904, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 17,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 513, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 2,                           // len, iter, post, mode, oper
      1537, 1025, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:2-2, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 31, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 912, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 18,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 514, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 2,                           // len, iter, post, mode, oper
      1538, 1026, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:3-3, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 31, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 920, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 19,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 515, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 2,                           // len, iter, post, mode, oper
      1539, 1027, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:4-4, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 31, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 928, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 20,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 516, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 2,                           // len, iter, post, mode, oper
      1540, 1028, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:5-5, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 31, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 936, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 21,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 517, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 2,                           // len, iter, post, mode, oper
      1541, 1029, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:6-6, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 31, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 944, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 22,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 518, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 2,                           // len, iter, post, mode, oper
      1542, 1030, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:7-7, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 31, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 952, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 23,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 519, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 2,                           // len, iter, post, mode, oper
      1543, 1031, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:0-0, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 31, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 896, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 16,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 520, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 2,                           // len, iter, post, mode, oper
      1544, 1032, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:1-1, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 31, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 904, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 17,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 521, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 2,                           // len, iter, post, mode, oper
      1545, 1033, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:2-2, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 31, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 912, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 18,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 522, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 2,                           // len, iter, post, mode, oper
      1546, 1034, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:3-3, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 31, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 920, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 19,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 523, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 2,                           // len, iter, post, mode, oper
      1547, 1035, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:4-4, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 31, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 928, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 20,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 524, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 2,                           // len, iter, post, mode, oper
      1548, 1036, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:5-5, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 31, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 936, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 21,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 525, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 2,                           // len, iter, post, mode, oper
      1549, 1037, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:6-6, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 31, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 944, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 22,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 526, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 2,                           // len, iter, post, mode, oper
      1550, 1038, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:7-7, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 31, 31, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 952, 0x00, 15, 31       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 23,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 527, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 32, 1, 0, 2,                           // len, iter, post, mode, oper
      1551, 1039, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:0
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      0, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      512, 1536, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:1
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      64, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      576, 1600, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:2
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      128, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      640, 1664, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:3
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      192, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      704, 1728, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:4
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      256, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      768, 1792, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:5
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      320, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      832, 1856, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:6
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      384, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      896, 1920, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:7
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      448, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      960, 1984, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // block0-2
  LOG_INFO("gpio[0] = 11;");

  dif_dla_move_ddr2gb(&dla, 960, 65920, 0,   // baseaddr, len, ddr_addr, gb_addr 
              0, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 66880, 0,   // baseaddr, len, ddr_addr, gb_addr 
              1, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 67840, 0,   // baseaddr, len, ddr_addr, gb_addr 
              2, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 68800, 0,   // baseaddr, len, ddr_addr, gb_addr 
              3, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 69760, 0,   // baseaddr, len, ddr_addr, gb_addr 
              4, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 70720, 0,   // baseaddr, len, ddr_addr, gb_addr 
              5, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 71680, 0,   // baseaddr, len, ddr_addr, gb_addr 
              6, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 72640, 0,   // baseaddr, len, ddr_addr, gb_addr 
              7, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 73600, 0,   // baseaddr, len, ddr_addr, gb_addr 
              8, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 74560, 0,   // baseaddr, len, ddr_addr, gb_addr 
              9, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 75520, 0,   // baseaddr, len, ddr_addr, gb_addr 
              10, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 76480, 0,   // baseaddr, len, ddr_addr, gb_addr 
              11, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 77440, 0,   // baseaddr, len, ddr_addr, gb_addr 
              12, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 78400, 0,   // baseaddr, len, ddr_addr, gb_addr 
              13, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 79360, 0,   // baseaddr, len, ddr_addr, gb_addr 
              14, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 80320, 0,   // baseaddr, len, ddr_addr, gb_addr 
              15, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 81280, 0,   // baseaddr, len, ddr_addr, gb_addr 
              16, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 82240, 0,   // baseaddr, len, ddr_addr, gb_addr 
              17, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 83200, 0,   // baseaddr, len, ddr_addr, gb_addr 
              18, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 84160, 0,   // baseaddr, len, ddr_addr, gb_addr 
              19, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 85120, 0,   // baseaddr, len, ddr_addr, gb_addr 
              20, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 86080, 0,   // baseaddr, len, ddr_addr, gb_addr 
              21, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 87040, 0,   // baseaddr, len, ddr_addr, gb_addr 
              22, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 88000, 0,   // baseaddr, len, ddr_addr, gb_addr 
              23, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 88960, 0,   // baseaddr, len, ddr_addr, gb_addr 
              24, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 89920, 0,   // baseaddr, len, ddr_addr, gb_addr 
              25, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 90880, 0,   // baseaddr, len, ddr_addr, gb_addr 
              26, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 91840, 0,   // baseaddr, len, ddr_addr, gb_addr 
              27, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 92800, 0,   // baseaddr, len, ddr_addr, gb_addr 
              28, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 93760, 0,   // baseaddr, len, ddr_addr, gb_addr 
              29, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 94720, 0,   // baseaddr, len, ddr_addr, gb_addr 
              30, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 95680, 0,   // baseaddr, len, ddr_addr, gb_addr 
              31, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 96640, 0,   // baseaddr, len, ddr_addr, gb_addr 
              4, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 96664, 0,   // baseaddr, len, ddr_addr, gb_addr 
              5, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 96688, 0,   // baseaddr, len, ddr_addr, gb_addr 
              6, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 96712, 0,   // baseaddr, len, ddr_addr, gb_addr 
              7, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 96736, 0,   // baseaddr, len, ddr_addr, gb_addr 
              0, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 96760, 0,   // baseaddr, len, ddr_addr, gb_addr 
              1, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 96784, 0,   // baseaddr, len, ddr_addr, gb_addr 
              2, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 96808, 0,   // baseaddr, len, ddr_addr, gb_addr 
              3, 3, 0);                      // gb_idx, gb_mux, direction
  // Computing conv_filter  dila_idx:0
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_move_fbuf2lbuf(&dla, 1024, 0,  // baseaddr, src_addr, dest_addr
                 1, 31, 15, 32, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 0, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 0, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 56, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 1,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 1, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 112, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 2,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 2, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 168, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 3,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 3, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 224, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 4, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 280, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 5,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 5, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 336, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 6,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 6, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 392, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 7,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 7, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 0, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 8, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 56, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 1,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 9, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 112, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 2,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 10, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 168, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 3,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 11, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 224, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 12, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 280, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 5,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 13, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 336, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 6,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 14, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 392, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 7,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 15, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:2
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 0, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 16, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:2
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 56, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 1,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 17, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:2
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 112, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 2,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 18, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:2
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 168, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 3,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 19, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:2
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 224, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 20, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:2
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 280, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 5,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 21, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:2
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 336, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 6,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 22, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:2
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 392, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 7,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 23, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:3
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 0, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 24, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:3
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 56, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 1,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 25, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:3
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 112, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 2,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 26, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:3
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 168, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 3,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 27, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:3
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 224, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 28, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:3
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 280, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 5,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 29, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:3
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 336, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 6,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 30, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:3
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 14, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 392, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 7,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 31, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_move_fbuf2lbuf(&dla, 1024, 0,  // baseaddr, src_addr, dest_addr
                 1, 31, 15, 32, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 448, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      0, 512, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 504, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 9,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      1, 513, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 560, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 10,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      2, 514, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 616, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      3, 515, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 672, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      4, 516, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 728, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 13,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      5, 517, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 784, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 14,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      6, 518, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 840, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 15,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      7, 519, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 448, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      8, 520, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 504, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 9,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      9, 521, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 560, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 10,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      10, 522, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 616, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      11, 523, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 672, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      12, 524, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 728, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 13,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      13, 525, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 784, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 14,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      14, 526, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 840, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 15,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      15, 527, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:2
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 448, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      16, 528, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:2
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 504, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 9,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      17, 529, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:2
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 560, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 10,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      18, 530, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:2
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 616, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      19, 531, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:2
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 672, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      20, 532, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:2
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 728, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 13,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      21, 533, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:2
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 784, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 14,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      22, 534, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:2
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 840, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 15,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      23, 535, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:3
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 448, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      24, 536, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:3
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 504, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 9,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      25, 537, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:3
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 560, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 10,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      26, 538, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:3
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 616, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      27, 539, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:3
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 672, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      28, 540, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:3
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 728, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 13,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      29, 541, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:3
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 784, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 14,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      30, 542, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:3
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 21, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 840, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 15,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 1,                           // len, iter, post, mode, oper
      31, 543, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:0-0, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 512, 0,  // baseaddr, src_addr, dest_addr
                 1, 31, 15, 32, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 896, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 16,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 0, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1024, 512, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:1-1, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 904, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 17,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 1, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1025, 513, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:2-2, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 912, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 18,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 2, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1026, 514, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:3-3, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 920, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 19,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 3, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1027, 515, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:4-4, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 928, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 20,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 4, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1028, 516, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:5-5, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 936, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 21,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 5, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1029, 517, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:6-6, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 944, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 22,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 6, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1030, 518, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:7-7, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 952, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 23,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 7, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1031, 519, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:0-0, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 896, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 16,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 8, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1032, 520, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:1-1, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 904, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 17,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 9, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1033, 521, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:2-2, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 912, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 18,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 10, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1034, 522, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:3-3, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 920, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 19,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 11, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1035, 523, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:4-4, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 928, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 20,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 12, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1036, 524, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:5-5, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 936, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 21,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 13, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1037, 525, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:6-6, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 944, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 22,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 14, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1038, 526, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:7-7, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 952, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 23,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 15, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1039, 527, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:2
// t:0-63, oc:0-0, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 896, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 16,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 16, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1040, 528, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:2
// t:0-63, oc:1-1, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 904, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 17,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 17, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1041, 529, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:2
// t:0-63, oc:2-2, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 912, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 18,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 18, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1042, 530, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:2
// t:0-63, oc:3-3, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 920, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 19,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 19, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1043, 531, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:2
// t:0-63, oc:4-4, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 928, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 20,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 20, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1044, 532, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:2
// t:0-63, oc:5-5, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 936, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 21,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 21, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1045, 533, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:2
// t:0-63, oc:6-6, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 944, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 22,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 22, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1046, 534, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:2
// t:0-63, oc:7-7, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 952, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 23,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 23, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1047, 535, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:3
// t:0-63, oc:0-0, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 896, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 16,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 24, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1048, 536, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:3
// t:0-63, oc:1-1, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 904, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 17,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 25, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1049, 537, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:3
// t:0-63, oc:2-2, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 912, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 18,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 26, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1050, 538, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:3
// t:0-63, oc:3-3, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 920, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 19,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 27, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1051, 539, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:3
// t:0-63, oc:4-4, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 928, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 20,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 28, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1052, 540, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:3
// t:0-63, oc:5-5, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 936, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 21,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 29, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1053, 541, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:3
// t:0-63, oc:6-6, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 944, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 22,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 30, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1054, 542, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:3
// t:0-63, oc:7-7, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 15, 15, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 952, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 23,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 31, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 16, 1, 0, 2,                           // len, iter, post, mode, oper
      1055, 543, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 32, 32,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:0
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      1536, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      0, 1024, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:1
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      1600, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      64, 1088, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:2
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      1664, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      128, 1152, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:3
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      1728, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      192, 1216, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:4
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      1792, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      256, 1280, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:5
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      1856, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      320, 1344, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:6
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      1920, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      384, 1408, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:7
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      1984, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      448, 1472, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // block0-3
  LOG_INFO("gpio[0] = 10;");

  dif_dla_move_ddr2gb(&dla, 960, 96832, 0,   // baseaddr, len, ddr_addr, gb_addr 
              0, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 97792, 0,   // baseaddr, len, ddr_addr, gb_addr 
              1, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 98752, 0,   // baseaddr, len, ddr_addr, gb_addr 
              2, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 99712, 0,   // baseaddr, len, ddr_addr, gb_addr 
              3, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 100672, 0,   // baseaddr, len, ddr_addr, gb_addr 
              4, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 101632, 0,   // baseaddr, len, ddr_addr, gb_addr 
              5, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 102592, 0,   // baseaddr, len, ddr_addr, gb_addr 
              6, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 103552, 0,   // baseaddr, len, ddr_addr, gb_addr 
              7, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 104512, 0,   // baseaddr, len, ddr_addr, gb_addr 
              8, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 105472, 0,   // baseaddr, len, ddr_addr, gb_addr 
              9, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 106432, 0,   // baseaddr, len, ddr_addr, gb_addr 
              10, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 107392, 0,   // baseaddr, len, ddr_addr, gb_addr 
              11, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 108352, 0,   // baseaddr, len, ddr_addr, gb_addr 
              12, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 109312, 0,   // baseaddr, len, ddr_addr, gb_addr 
              13, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 110272, 0,   // baseaddr, len, ddr_addr, gb_addr 
              14, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 111232, 0,   // baseaddr, len, ddr_addr, gb_addr 
              15, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 112192, 0,   // baseaddr, len, ddr_addr, gb_addr 
              16, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 113152, 0,   // baseaddr, len, ddr_addr, gb_addr 
              17, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 114112, 0,   // baseaddr, len, ddr_addr, gb_addr 
              18, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 115072, 0,   // baseaddr, len, ddr_addr, gb_addr 
              19, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 116032, 0,   // baseaddr, len, ddr_addr, gb_addr 
              20, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 116992, 0,   // baseaddr, len, ddr_addr, gb_addr 
              21, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 117952, 0,   // baseaddr, len, ddr_addr, gb_addr 
              22, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 118912, 0,   // baseaddr, len, ddr_addr, gb_addr 
              23, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 119872, 0,   // baseaddr, len, ddr_addr, gb_addr 
              24, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 120832, 0,   // baseaddr, len, ddr_addr, gb_addr 
              25, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 121792, 0,   // baseaddr, len, ddr_addr, gb_addr 
              26, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 122752, 0,   // baseaddr, len, ddr_addr, gb_addr 
              27, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 123712, 0,   // baseaddr, len, ddr_addr, gb_addr 
              28, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 124672, 0,   // baseaddr, len, ddr_addr, gb_addr 
              29, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 125632, 0,   // baseaddr, len, ddr_addr, gb_addr 
              30, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 960, 126592, 0,   // baseaddr, len, ddr_addr, gb_addr 
              31, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 127552, 0,   // baseaddr, len, ddr_addr, gb_addr 
              4, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 127576, 0,   // baseaddr, len, ddr_addr, gb_addr 
              5, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 127600, 0,   // baseaddr, len, ddr_addr, gb_addr 
              6, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 127624, 0,   // baseaddr, len, ddr_addr, gb_addr 
              7, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 127648, 0,   // baseaddr, len, ddr_addr, gb_addr 
              0, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 127672, 0,   // baseaddr, len, ddr_addr, gb_addr 
              1, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 127696, 0,   // baseaddr, len, ddr_addr, gb_addr 
              2, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 24, 127720, 0,   // baseaddr, len, ddr_addr, gb_addr 
              3, 3, 0);                      // gb_idx, gb_mux, direction
  // Computing conv_filter  dila_idx:0
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_move_fbuf2lbuf(&dla, 512, 0,  // baseaddr, src_addr, dest_addr
                 1, 31, 7, 64, 0);       // skip, iter, len, dila, mode
  dif_dla_move_fbuf2lbuf(&dla, 544, 256,  // baseaddr, src_addr, dest_addr
                 1, 31, 7, 64, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 0, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 0, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 56, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 1,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 1, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 112, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 2,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 2, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 168, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 3,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 3, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 224, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 4, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 280, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 5,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 5, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 336, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 6,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 6, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:0
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 392, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 7,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 7, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 0, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 8, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 56, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 1,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 9, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 112, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 2,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 10, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 168, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 3,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 11, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 224, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 12, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 280, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 5,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 13, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 336, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 6,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 14, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:1
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 392, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 7,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 15, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:2
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 0, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 16, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:2
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 56, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 1,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 17, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:2
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 112, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 2,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 18, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:2
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 168, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 3,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 19, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:2
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 224, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 20, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:2
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 280, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 5,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 21, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:2
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 336, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 6,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 22, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:2
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 392, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 7,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 23, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:3
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 0, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 24, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:3
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 56, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 1,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 25, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:3
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 112, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 2,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 26, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:3
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 168, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 3,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 27, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:3
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 224, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 28, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:3
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 280, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 5,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 29, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:3
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 336, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 6,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 30, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:3
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 392, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 7,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 31, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:4
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 0, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 32, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:4
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 56, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 1,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 33, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:4
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 112, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 2,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 34, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:4
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 168, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 3,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 35, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:4
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 224, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 36, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:4
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 280, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 5,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 37, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:4
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 336, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 6,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 38, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:4
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 392, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 7,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 39, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:5
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 0, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 40, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:5
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 56, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 1,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 41, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:5
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 112, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 2,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 42, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:5
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 168, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 3,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 43, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:5
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 224, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 44, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:5
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 280, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 5,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 45, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:5
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 336, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 6,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 46, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:5
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 392, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 7,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 47, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:6
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 0, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 48, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:6
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 56, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 1,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 49, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:6
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 112, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 2,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 50, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:6
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 168, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 3,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 51, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:6
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 224, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 52, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:6
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 280, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 5,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 53, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:6
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 336, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 6,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 54, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:6
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 392, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 7,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 55, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:7
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 0, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 56, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:7
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 56, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 1,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 57, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:7
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 112, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 2,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 58, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:7
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 168, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 3,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 59, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:7
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 224, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 60, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:7
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 280, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 5,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 61, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:7
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 336, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 6,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 62, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_filter  dila_idx:7
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 392, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 7,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 3,                           // len, iter, post, mode, oper
      11111, 63, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_move_fbuf2lbuf(&dla, 512, 0,  // baseaddr, src_addr, dest_addr
                 1, 31, 7, 64, 0);       // skip, iter, len, dila, mode
  dif_dla_move_fbuf2lbuf(&dla, 544, 256,  // baseaddr, src_addr, dest_addr
                 1, 31, 7, 64, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 448, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      0, 1536, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 504, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 9,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      1, 1537, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 560, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 10,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      2, 1538, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 616, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      3, 1539, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 672, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      4, 1540, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 728, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 13,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      5, 1541, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 784, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 14,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      6, 1542, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:0
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 840, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 15,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      7, 1543, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 448, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      8, 1544, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 504, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 9,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      9, 1545, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 560, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 10,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      10, 1546, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 616, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      11, 1547, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 672, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      12, 1548, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 728, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 13,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      13, 1549, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 784, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 14,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      14, 1550, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:1
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 840, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 15,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      15, 1551, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:2
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 448, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      16, 1552, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:2
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 504, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 9,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      17, 1553, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:2
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 560, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 10,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      18, 1554, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:2
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 616, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      19, 1555, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:2
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 672, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      20, 1556, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:2
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 728, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 13,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      21, 1557, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:2
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 784, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 14,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      22, 1558, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:2
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 840, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 15,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      23, 1559, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:3
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 448, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      24, 1560, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:3
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 504, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 9,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      25, 1561, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:3
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 560, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 10,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      26, 1562, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:3
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 616, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      27, 1563, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:3
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 672, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      28, 1564, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:3
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 728, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 13,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      29, 1565, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:3
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 784, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 14,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      30, 1566, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:3
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 840, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 15,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      31, 1567, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:4
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 448, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      32, 1568, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:4
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 504, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 9,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      33, 1569, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:4
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 560, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 10,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      34, 1570, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:4
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 616, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      35, 1571, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:4
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 672, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      36, 1572, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:4
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 728, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 13,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      37, 1573, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:4
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 784, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 14,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      38, 1574, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:4
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 840, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 15,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      39, 1575, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:5
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 448, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      40, 1576, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:5
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 504, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 9,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      41, 1577, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:5
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 560, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 10,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      42, 1578, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:5
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 616, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      43, 1579, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:5
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 672, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      44, 1580, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:5
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 728, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 13,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      45, 1581, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:5
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 784, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 14,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      46, 1582, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:5
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 840, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 15,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      47, 1583, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:6
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 448, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      48, 1584, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:6
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 504, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 9,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      49, 1585, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:6
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 560, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 10,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      50, 1586, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:6
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 616, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      51, 1587, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:6
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 672, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      52, 1588, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:6
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 728, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 13,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      53, 1589, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:6
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 784, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 14,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      54, 1590, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:6
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 840, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 15,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      55, 1591, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:7
// t:0-63, oc:0-0, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 448, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      56, 1592, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:7
// t:0-63, oc:1-1, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 504, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 9,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      57, 1593, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:7
// t:0-63, oc:2-2, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 560, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 10,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      58, 1594, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:7
// t:0-63, oc:3-3, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 616, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      59, 1595, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:7
// t:0-63, oc:4-4, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 672, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      60, 1596, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:7
// t:0-63, oc:5-5, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 728, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 13,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      61, 1597, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:7
// t:0-63, oc:6-6, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 784, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 14,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      62, 1598, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_gate  dila_idx:7
// t:0-63, oc:7-7, ic:0-7, k:0-6
// T:64, OC:8, IC:8, K:7
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      7, 13, 13, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      3, 3, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 840, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 15,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 1,                           // len, iter, post, mode, oper
      63, 1599, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:0-0, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 1536, 0,  // baseaddr, src_addr, dest_addr
                 1, 31, 7, 64, 0);       // skip, iter, len, dila, mode
  dif_dla_move_fbuf2lbuf(&dla, 1568, 256,  // baseaddr, src_addr, dest_addr
                 1, 31, 7, 64, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 896, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 16,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 0, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      512, 1536, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:1-1, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 904, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 17,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 1, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      513, 1537, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:2-2, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 912, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 18,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 2, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      514, 1538, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:3-3, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 920, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 19,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 3, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      515, 1539, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:4-4, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 928, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 20,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 4, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      516, 1540, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:5-5, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 936, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 21,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 5, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      517, 1541, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:6-6, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 944, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 22,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 6, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      518, 1542, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:0
// t:0-63, oc:7-7, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      0, 952, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 23,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 7, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      519, 1543, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:0-0, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 896, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 16,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 8, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      520, 1544, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:1-1, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 904, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 17,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 9, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      521, 1545, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:2-2, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 912, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 18,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 10, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      522, 1546, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:3-3, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 920, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 19,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 11, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      523, 1547, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:4-4, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 928, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 20,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 12, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      524, 1548, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:5-5, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 936, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 21,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 13, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      525, 1549, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:6-6, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 944, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 22,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 14, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      526, 1550, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:1
// t:0-63, oc:7-7, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      64, 952, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 23,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 15, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      527, 1551, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:2
// t:0-63, oc:0-0, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 896, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 16,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 16, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      528, 1552, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:2
// t:0-63, oc:1-1, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 904, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 17,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 17, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      529, 1553, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:2
// t:0-63, oc:2-2, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 912, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 18,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 18, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      530, 1554, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:2
// t:0-63, oc:3-3, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 920, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 19,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 19, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      531, 1555, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:2
// t:0-63, oc:4-4, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 928, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 20,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 20, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      532, 1556, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:2
// t:0-63, oc:5-5, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 936, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 21,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 21, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      533, 1557, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:2
// t:0-63, oc:6-6, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 944, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 22,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 22, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      534, 1558, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:2
// t:0-63, oc:7-7, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      128, 952, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 23,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 23, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      535, 1559, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:3
// t:0-63, oc:0-0, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 896, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 16,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 24, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      536, 1560, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:3
// t:0-63, oc:1-1, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 904, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 17,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 25, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      537, 1561, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:3
// t:0-63, oc:2-2, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 912, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 18,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 26, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      538, 1562, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:3
// t:0-63, oc:3-3, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 920, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 19,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 27, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      539, 1563, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:3
// t:0-63, oc:4-4, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 928, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 20,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 28, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      540, 1564, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:3
// t:0-63, oc:5-5, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 936, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 21,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 29, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      541, 1565, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:3
// t:0-63, oc:6-6, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 944, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 22,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 30, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      542, 1566, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:3
// t:0-63, oc:7-7, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      192, 952, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 23,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 31, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      543, 1567, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:4
// t:0-63, oc:0-0, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 896, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 16,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 32, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      544, 1568, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:4
// t:0-63, oc:1-1, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 904, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 17,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 33, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      545, 1569, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:4
// t:0-63, oc:2-2, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 912, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 18,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 34, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      546, 1570, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:4
// t:0-63, oc:3-3, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 920, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 19,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 35, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      547, 1571, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:4
// t:0-63, oc:4-4, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 928, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 20,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 36, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      548, 1572, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:4
// t:0-63, oc:5-5, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 936, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 21,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 37, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      549, 1573, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:4
// t:0-63, oc:6-6, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 944, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 22,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 38, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      550, 1574, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:4
// t:0-63, oc:7-7, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      256, 952, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 23,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 39, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      551, 1575, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:5
// t:0-63, oc:0-0, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 896, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 16,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 40, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      552, 1576, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:5
// t:0-63, oc:1-1, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 904, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 17,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 41, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      553, 1577, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:5
// t:0-63, oc:2-2, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 912, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 18,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 42, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      554, 1578, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:5
// t:0-63, oc:3-3, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 920, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 19,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 43, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      555, 1579, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:5
// t:0-63, oc:4-4, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 928, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 20,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 44, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      556, 1580, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:5
// t:0-63, oc:5-5, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 936, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 21,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 45, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      557, 1581, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:5
// t:0-63, oc:6-6, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 944, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 22,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 46, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      558, 1582, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:5
// t:0-63, oc:7-7, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      320, 952, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 23,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 47, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      559, 1583, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:6
// t:0-63, oc:0-0, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 896, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 16,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 48, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      560, 1584, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:6
// t:0-63, oc:1-1, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 904, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 17,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 49, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      561, 1585, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:6
// t:0-63, oc:2-2, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 912, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 18,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 50, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      562, 1586, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:6
// t:0-63, oc:3-3, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 920, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 19,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 51, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      563, 1587, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:6
// t:0-63, oc:4-4, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 928, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 20,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 52, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      564, 1588, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:6
// t:0-63, oc:5-5, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 936, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 21,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 53, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      565, 1589, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:6
// t:0-63, oc:6-6, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 944, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 22,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 54, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      566, 1590, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:6
// t:0-63, oc:7-7, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      384, 952, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 23,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 55, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      567, 1591, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:7
// t:0-63, oc:0-0, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 896, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 16,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 56, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      568, 1592, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:7
// t:0-63, oc:1-1, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 904, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 17,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 57, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      569, 1593, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:7
// t:0-63, oc:2-2, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 912, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 18,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 58, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      570, 1594, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:7
// t:0-63, oc:3-3, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 920, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 19,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 59, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      571, 1595, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:7
// t:0-63, oc:4-4, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 928, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 20,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 60, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      572, 1596, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:7
// t:0-63, oc:5-5, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 936, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 21,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 61, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      573, 1597, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:7
// t:0-63, oc:6-6, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 944, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 22,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 62, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      574, 1598, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing conv_out  dila_idx:7
// t:0-63, oc:7-7, ic:0-7, k:0-0
// T:64, OC:8, IC:8, K:1
  dif_dla_conv_comp(
      &dla, 0,                                   // baseaddr, mode_spar
      1, 14, 7, 7, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
      448, 952, 0x00, 15, 7       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bn
  dif_dla_ppe_comp_full (
      &dla, 0, 0, 2, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 0, 0, 3,                           // len, iter, post, mode, oper
      11111, 11111, 23,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 2, 0, 3,                           // len, iter, post, mode, oper
      11111, 63, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 1, 11111, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_tanh, act_b_tanh, act_x_tanh
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      0, 8, 1, 0, 2,                           // len, iter, post, mode, oper
      575, 1599, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      1, 1, 64, 64,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:0
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      1024, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      0, 512, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:1
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      1088, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      64, 576, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:2
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      1152, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      128, 640, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:3
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      1216, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      192, 704, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:4
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      1280, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      256, 768, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:5
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      1344, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      320, 832, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:6
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      1408, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      384, 896, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing add1
// oc_idx:7
// T:64, OC:8, IC:8, K:1
  // rst
dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
      1472, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
      63, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
      448, 960, 11111,                               // fbuf_src, fbuf_dest, abuf_src
      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
      act_k_dummy, act_b_dummy, act_x_dummy
  );

  dif_dla_move_ddr2gb(&dla, 2048, 130000, 0,   // baseaddr, len, ddr_addr, gb_addr 
          0, 0, 1);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 2048, 132048, 0,   // baseaddr, len, ddr_addr, gb_addr 
          1, 0, 1);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 2048, 134096, 0,   // baseaddr, len, ddr_addr, gb_addr 
          2, 0, 1);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 2048, 136144, 0,   // baseaddr, len, ddr_addr, gb_addr 
          3, 0, 1);            // gb_idx, gb_mux, direction

  LOG_INFO("gpio[0] = 0xf;");
}


void wavenet_wrapper(void){
  //init ddr
  write_to_ddr_from_spi(&dla, &spi, DDRIN_SIZE);
  LOG_INFO("DDR initialized!");
  CHECK(dif_dla_set_ppe(&dla, 0xffff, 0xffff) == kDifDlaOk);

  wavenet();

  LOG_INFO("DLA done!");
  //read from DDR
  read_from_ddr_to_spi(&dla, &spi, RD_DDR_ADDR, RD_DDR_BYTES);
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
  LOG_INFO("Hello WaveNet!");
  LOG_INFO("Built at: " __DATE__ ", " __TIME__);

  CHECK(dif_dla_init(
            (dif_dla_params_t){
              .base_addr = mmio_region_from_addr(TOP_EARLGREY_DLA_BASE_ADDR),
            },&dla) == kDifDlaOk);
  LOG_INFO("DLA initialized!");

  LOG_INFO("Waiting for DDR calibration.");
  CHECK(dif_dla_ddr_ctrl_init_calib(&dla)== kDifDlaOk);
  LOG_INFO("DDR calibration completed.");
  
  ddr_test(&dla, &spi, DDRIN_SIZE);
  LOG_INFO("DDR-SPI sanity test finished.");

  // wavenet_wrapper();
  
  uint32_t gpio_state = 0;
  while (true) {
    usleep(10 * 1000);  // 10 ms
    gpio_state = demo_gpio_to_log_echo(&gpio, gpio_state);
    demo_spi_to_log_echo(&spi);
    demo_uart_to_uart_and_gpio_echo(&uart, &gpio);
  }
}
