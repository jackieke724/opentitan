// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "sw/device/examples/demos.h"
#include "sw/device/lib/arch/device.h"
#include "sw/device/lib/dif/dif_gpio.h"
#include "sw/device/lib/dif/dif_spi_device.h"
#include "sw/device/lib/dif/dif_uart.h"
#include "sw/device/lib/dif/dif_rv_timer.h"
#include "sw/device/lib/dif/dif_dla.h"
#include "sw/device/lib/pinmux.h"
#include "sw/device/lib/runtime/hart.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/runtime/print.h"
#include "sw/device/lib/testing/check.h"
#include "sw/device/lib/testing/test_status.h"
#include "sw/device/lib/ddr_spi.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"  // Generated.

static const uint32_t kHart = (uint32_t)kTopEarlgreyPlicTargetIbex0;
// static const uint32_t kComparator = 0;

static dif_gpio_t gpio;
static dif_spi_device_t spi;
static dif_uart_t uart;
static dif_rv_timer_t timer;
static dif_dla_t dla;

//python script to send ddr contents from spi
//python3 util/simplespi/spitest.py -i lpcnet1_ddr_in.txt

//python script to receive same ddr contents and send to spi
//python3 util/simplespi/spitest.py -r 196608


//used for ddr_spi functions
//DDRIN_LINES needs to be divisible by 128,
//or DDRIN_SIZE needs to be divisible by 1024,
//so SPI device can send integer patches of 1024B
const uint32_t DDRIN_LINES = 21632; //lpcnet1_ddr_in.txt
const uint32_t DDRIN_SIZE = DDRIN_LINES * 8 ; //bytes

//used for reading wavenet result
//ddr read start address
const uint32_t RD_DDR_ADDR = 30000; //64-bit addressable in unsigned decimal
//ddr read bytes
const uint32_t RD_DDR_BYTES = 65536; //bytes


void lpcnet1(void){
  uint32_t act_k_tanh_pos[16] = {0x157e, 0x1bf8, 0x21c4, 0x2825, 0x2dd6, 0x33c6, 0x3878, 0x3b75,
                      0x3b75, 0x3878, 0x33c6, 0x2dd6, 0x2825, 0x21c4, 0x1bf8, 0x157e};
  uint32_t act_b_tanh_pos[16] = {0xbbf3, 0xbbe0, 0xbbb0, 0xbb3f, 0xba43, 0xb85e, 0xb2ef, 0xa21e,
                      0x221e, 0x32ef, 0x385e, 0x3a43, 0x3b3f, 0x3bb0, 0x3be0, 0x3bf3};
  uint32_t act_x_tanh_pos[15] = {0xc377, 0xc266, 0xc155, 0xc044, 0xbe66, 0xbc44, 0xb844, 0x0000,
                      0x3844, 0x3c44, 0x3e66, 0x4044, 0x4155, 0x4266, 0x4377};

  // uint32_t act_k_tanh_neg[16] = {0x957e, 0x9bf8, 0xa1c4, 0xa825, 0xadd6, 0xb3c6, 0xb878, 0xbb75,
  //                         0xbb75, 0xb878, 0xb3c6, 0xadd6, 0xa825, 0xa1c4, 0x9bf8, 0x957e};
  // uint32_t act_b_tanh_neg[16] = {0x3bf3, 0x3be0, 0x3bb0, 0x3b3f, 0x3a43, 0x385e, 0x32ef, 0x221e,
  //                         0xa21e, 0xb2ef, 0xb85e, 0xba43, 0xbb3f, 0xbbb0, 0xbbe0, 0xbbf3};
  // uint32_t act_x_tanh_neg[15] = {0xc377, 0xc266, 0xc155, 0xc044, 0xbe66, 0xbc44, 0xb844, 0x0000,
  //                         0x3844, 0x3c44, 0x3e66, 0x4044, 0x4155, 0x4266, 0x4377};

  // uint32_t act_k_sigmoid_pos[16] = {0x190d, 0x1d96, 0x2222, 0x26a0, 0x2aea, 0x2eb8, 0x31b1, 0x33b0,
  //                          0x33b0, 0x31b1, 0x2eb8, 0x2aea, 0x26a0, 0x2222, 0x1d96, 0x190d};
  // uint32_t act_b_sigmoid_pos[16] = {0x246b, 0x2855, 0x2c26, 0x2faa, 0x32ad, 0x3544, 0x371e, 0x37f5,
  //                          0x3805, 0x3870, 0x395d, 0x3a54, 0x3b0a, 0x3b7b, 0x3bba, 0x3bdc};
  // uint32_t act_x_sigmoid_pos[15] = {0xc599, 0xc4cc, 0xc3ff, 0xc266, 0xc0cc, 0xbe66, 0xba65, 0x0000,
  //                          0x3a65, 0x3e66, 0x40cc, 0x4266, 0x43ff, 0x44cc, 0x4599};

  // uint32_t act_k_sigmoid_neg[16] = {0x990d, 0x9d96, 0xa222, 0xa6a0, 0xaaea, 0xaeb8, 0xb1b1, 0xb3b0,
  //                             0xb3b0, 0xb1b1, 0xaeb8, 0xaaea, 0xa6a0, 0xa222, 0x9d96, 0x990d};
  // uint32_t act_b_sigmoid_neg[16] = {0xa46b, 0xa855, 0xac26, 0xafaa, 0xb2ad, 0xb544, 0xb71e, 0xb7f5,
  //                             0xb805, 0xb870, 0xb95d, 0xba54, 0xbb0a, 0xbb7b, 0xbbba, 0xbbdc};
  // uint32_t act_x_sigmoid_neg[15] = {0xc599, 0xc4cc, 0xc3ff, 0xc266, 0xc0cc, 0xbe66, 0xba65, 0x0000,
  //                             0x3a65, 0x3e66, 0x40cc, 0x4266, 0x43ff, 0x44cc, 0x4599};

  uint32_t act_k_dummy[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t act_b_dummy[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t act_x_dummy[15] = {0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0};

  // uint32_t* act_k[5] = {act_k_tanh_pos, act_k_tanh_neg, act_k_sigmoid_pos, act_k_sigmoid_neg, act_k_dummy};
  // uint32_t* act_b[5] = {act_b_tanh_pos, act_b_tanh_neg, act_b_sigmoid_pos, act_b_sigmoid_neg, act_b_dummy};
  // uint32_t* act_x[5] = {act_x_tanh_pos, act_x_tanh_neg, act_x_sigmoid_pos, act_x_sigmoid_neg, act_x_dummy};

  dif_dla_pbuf_rst(&dla, 64);

  // meu
  LOG_INFO("gpio[0] = 1;");

  dif_dla_move_ddr2gb(&dla, 280, 15688, 0,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 280, 15968, 0,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 280, 16248, 0,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 280, 16528, 0,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 0, 0,   // baseaddr, len, ddr_addr, gb_addr 
                0, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 488, 0,   // baseaddr, len, ddr_addr, gb_addr 
                1, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 976, 0,   // baseaddr, len, ddr_addr, gb_addr 
                2, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 1464, 0,   // baseaddr, len, ddr_addr, gb_addr 
                3, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 1952, 0,   // baseaddr, len, ddr_addr, gb_addr 
                4, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 2440, 0,   // baseaddr, len, ddr_addr, gb_addr 
                5, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 2928, 0,   // baseaddr, len, ddr_addr, gb_addr 
                6, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 3416, 0,   // baseaddr, len, ddr_addr, gb_addr 
                7, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 3904, 0,   // baseaddr, len, ddr_addr, gb_addr 
                8, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 4392, 0,   // baseaddr, len, ddr_addr, gb_addr 
                9, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 4880, 0,   // baseaddr, len, ddr_addr, gb_addr 
                10, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 5368, 0,   // baseaddr, len, ddr_addr, gb_addr 
                11, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 5856, 0,   // baseaddr, len, ddr_addr, gb_addr 
                12, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 6344, 0,   // baseaddr, len, ddr_addr, gb_addr 
                13, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 6832, 0,   // baseaddr, len, ddr_addr, gb_addr 
                14, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 7320, 0,   // baseaddr, len, ddr_addr, gb_addr 
                15, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 7808, 0,   // baseaddr, len, ddr_addr, gb_addr 
                16, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 8296, 0,   // baseaddr, len, ddr_addr, gb_addr 
                17, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 8784, 0,   // baseaddr, len, ddr_addr, gb_addr 
                18, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 9272, 0,   // baseaddr, len, ddr_addr, gb_addr 
                19, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 9760, 0,   // baseaddr, len, ddr_addr, gb_addr 
                20, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 10248, 0,   // baseaddr, len, ddr_addr, gb_addr 
                21, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 10736, 0,   // baseaddr, len, ddr_addr, gb_addr 
                22, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 11224, 0,   // baseaddr, len, ddr_addr, gb_addr 
                23, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 11712, 0,   // baseaddr, len, ddr_addr, gb_addr 
                24, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 12200, 0,   // baseaddr, len, ddr_addr, gb_addr 
                25, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 12688, 0,   // baseaddr, len, ddr_addr, gb_addr 
                26, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 13176, 0,   // baseaddr, len, ddr_addr, gb_addr 
                27, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 13664, 0,   // baseaddr, len, ddr_addr, gb_addr 
                28, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 14152, 0,   // baseaddr, len, ddr_addr, gb_addr 
                29, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 14640, 0,   // baseaddr, len, ddr_addr, gb_addr 
                30, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 15128, 0,   // baseaddr, len, ddr_addr, gb_addr 
                31, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 16, 15620, 0,   // baseaddr, len, ddr_addr, gb_addr 
                0, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 16, 15636, 0,   // baseaddr, len, ddr_addr, gb_addr 
                1, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 16, 15652, 0,   // baseaddr, len, ddr_addr, gb_addr 
                2, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 16, 15668, 0,   // baseaddr, len, ddr_addr, gb_addr 
                3, 3, 0);                      // gb_idx, gb_mux, direction
  // Computing conv_1_0_0
  // t:0-39, oc:0-0, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
                  1, 6, 39, 7, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 0, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 280, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:1-1, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 21, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 281, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:2-2, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 42, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 282, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:3-3, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 63, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 283, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:4-4, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 84, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 284, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:5-5, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 105, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 285, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:6-6, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 126, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 286, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:7-7, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 147, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 287, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:0-0, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_move_fbuf2lbuf(&dla, 280, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 39, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 168, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 600, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:1-1, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 192, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 601, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:2-2, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 216, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 602, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:3-3, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 240, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 603, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:4-4, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 264, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 604, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:5-5, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 288, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 605, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:6-6, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 312, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 606, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:7-7, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 336, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 607, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:0-0, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 600, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 39, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 360, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 0, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:1-1, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 368, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 1,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 1, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:2-2, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 376, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 2,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 2, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:3-3, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 384, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 3,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 3, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:4-4, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 392, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 4, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:5-5, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 400, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 5,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 5, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:6-6, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 408, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 6,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 6, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:7-7, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 416, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 7,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 7, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:0-0, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 39, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 424, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 320, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:1-1, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 432, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 9,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 321, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:2-2, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 440, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 10,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 322, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:3-3, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 448, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 11,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 323, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:4-4, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 456, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 324, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:5-5, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 464, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 13,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 325, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:6-6, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 472, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 14,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 326, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:7-7, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 480, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 15,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 327, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  dif_dla_move_ddr2gb(&dla, 320, 16812, 320,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 320, 17132, 320,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 320, 17452, 320,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 320, 17772, 320,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  // meu
  LOG_INFO("gpio[0] = 3;");

  dif_dla_move_ddr2gb(&dla, 280, 18096, 0,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 280, 18376, 0,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 280, 18656, 0,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 280, 18936, 0,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 0, 0,   // baseaddr, len, ddr_addr, gb_addr 
                0, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 488, 0,   // baseaddr, len, ddr_addr, gb_addr 
                1, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 976, 0,   // baseaddr, len, ddr_addr, gb_addr 
                2, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 1464, 0,   // baseaddr, len, ddr_addr, gb_addr 
                3, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 1952, 0,   // baseaddr, len, ddr_addr, gb_addr 
                4, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 2440, 0,   // baseaddr, len, ddr_addr, gb_addr 
                5, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 2928, 0,   // baseaddr, len, ddr_addr, gb_addr 
                6, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 3416, 0,   // baseaddr, len, ddr_addr, gb_addr 
                7, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 3904, 0,   // baseaddr, len, ddr_addr, gb_addr 
                8, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 4392, 0,   // baseaddr, len, ddr_addr, gb_addr 
                9, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 4880, 0,   // baseaddr, len, ddr_addr, gb_addr 
                10, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 5368, 0,   // baseaddr, len, ddr_addr, gb_addr 
                11, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 5856, 0,   // baseaddr, len, ddr_addr, gb_addr 
                12, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 6344, 0,   // baseaddr, len, ddr_addr, gb_addr 
                13, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 6832, 0,   // baseaddr, len, ddr_addr, gb_addr 
                14, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 7320, 0,   // baseaddr, len, ddr_addr, gb_addr 
                15, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 7808, 0,   // baseaddr, len, ddr_addr, gb_addr 
                16, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 8296, 0,   // baseaddr, len, ddr_addr, gb_addr 
                17, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 8784, 0,   // baseaddr, len, ddr_addr, gb_addr 
                18, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 9272, 0,   // baseaddr, len, ddr_addr, gb_addr 
                19, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 9760, 0,   // baseaddr, len, ddr_addr, gb_addr 
                20, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 10248, 0,   // baseaddr, len, ddr_addr, gb_addr 
                21, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 10736, 0,   // baseaddr, len, ddr_addr, gb_addr 
                22, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 11224, 0,   // baseaddr, len, ddr_addr, gb_addr 
                23, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 11712, 0,   // baseaddr, len, ddr_addr, gb_addr 
                24, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 12200, 0,   // baseaddr, len, ddr_addr, gb_addr 
                25, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 12688, 0,   // baseaddr, len, ddr_addr, gb_addr 
                26, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 13176, 0,   // baseaddr, len, ddr_addr, gb_addr 
                27, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 13664, 0,   // baseaddr, len, ddr_addr, gb_addr 
                28, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 14152, 0,   // baseaddr, len, ddr_addr, gb_addr 
                29, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 14640, 0,   // baseaddr, len, ddr_addr, gb_addr 
                30, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 15128, 0,   // baseaddr, len, ddr_addr, gb_addr 
                31, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 16, 15620, 0,   // baseaddr, len, ddr_addr, gb_addr 
                0, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 16, 15636, 0,   // baseaddr, len, ddr_addr, gb_addr 
                1, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 16, 15652, 0,   // baseaddr, len, ddr_addr, gb_addr 
                2, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 16, 15668, 0,   // baseaddr, len, ddr_addr, gb_addr 
                3, 3, 0);                      // gb_idx, gb_mux, direction
  // Computing conv_1_0_0
  // t:0-39, oc:0-0, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
                  1, 6, 39, 7, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 0, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 280, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:1-1, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 21, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 281, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:2-2, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 42, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 282, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:3-3, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 63, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 283, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:4-4, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 84, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 284, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:5-5, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 105, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 285, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:6-6, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 126, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 286, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:7-7, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 147, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 287, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:0-0, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_move_fbuf2lbuf(&dla, 280, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 39, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 168, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 600, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:1-1, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 192, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 601, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:2-2, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 216, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 602, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:3-3, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 240, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 603, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:4-4, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 264, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 604, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:5-5, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 288, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 605, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:6-6, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 312, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 606, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:7-7, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 336, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 607, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:0-0, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 600, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 39, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 360, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 0, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:1-1, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 368, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 1,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 1, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:2-2, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 376, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 2,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 2, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:3-3, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 384, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 3,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 3, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:4-4, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 392, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 4, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:5-5, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 400, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 5,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 5, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:6-6, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 408, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 6,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 6, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:7-7, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 416, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 7,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 7, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:0-0, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 39, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 424, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 320, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:1-1, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 432, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 9,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 321, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:2-2, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 440, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 10,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 322, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:3-3, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 448, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 11,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 323, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:4-4, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 456, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 324, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:5-5, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 464, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 13,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 325, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:6-6, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 472, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 14,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 326, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:7-7, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 480, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 15,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 327, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  dif_dla_move_ddr2gb(&dla, 320, 19220, 320,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 320, 19540, 320,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 320, 19860, 320,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 320, 20180, 320,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  // meu
  LOG_INFO("gpio[0] = 5;");

  dif_dla_move_ddr2gb(&dla, 280, 20504, 0,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 280, 20784, 0,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 280, 21064, 0,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 280, 21344, 0,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 0, 0,   // baseaddr, len, ddr_addr, gb_addr 
                0, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 488, 0,   // baseaddr, len, ddr_addr, gb_addr 
                1, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 976, 0,   // baseaddr, len, ddr_addr, gb_addr 
                2, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 1464, 0,   // baseaddr, len, ddr_addr, gb_addr 
                3, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 1952, 0,   // baseaddr, len, ddr_addr, gb_addr 
                4, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 2440, 0,   // baseaddr, len, ddr_addr, gb_addr 
                5, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 2928, 0,   // baseaddr, len, ddr_addr, gb_addr 
                6, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 3416, 0,   // baseaddr, len, ddr_addr, gb_addr 
                7, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 3904, 0,   // baseaddr, len, ddr_addr, gb_addr 
                8, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 4392, 0,   // baseaddr, len, ddr_addr, gb_addr 
                9, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 4880, 0,   // baseaddr, len, ddr_addr, gb_addr 
                10, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 5368, 0,   // baseaddr, len, ddr_addr, gb_addr 
                11, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 5856, 0,   // baseaddr, len, ddr_addr, gb_addr 
                12, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 6344, 0,   // baseaddr, len, ddr_addr, gb_addr 
                13, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 6832, 0,   // baseaddr, len, ddr_addr, gb_addr 
                14, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 7320, 0,   // baseaddr, len, ddr_addr, gb_addr 
                15, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 7808, 0,   // baseaddr, len, ddr_addr, gb_addr 
                16, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 8296, 0,   // baseaddr, len, ddr_addr, gb_addr 
                17, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 8784, 0,   // baseaddr, len, ddr_addr, gb_addr 
                18, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 9272, 0,   // baseaddr, len, ddr_addr, gb_addr 
                19, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 9760, 0,   // baseaddr, len, ddr_addr, gb_addr 
                20, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 10248, 0,   // baseaddr, len, ddr_addr, gb_addr 
                21, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 10736, 0,   // baseaddr, len, ddr_addr, gb_addr 
                22, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 11224, 0,   // baseaddr, len, ddr_addr, gb_addr 
                23, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 11712, 0,   // baseaddr, len, ddr_addr, gb_addr 
                24, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 12200, 0,   // baseaddr, len, ddr_addr, gb_addr 
                25, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 12688, 0,   // baseaddr, len, ddr_addr, gb_addr 
                26, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 13176, 0,   // baseaddr, len, ddr_addr, gb_addr 
                27, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 13664, 0,   // baseaddr, len, ddr_addr, gb_addr 
                28, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 14152, 0,   // baseaddr, len, ddr_addr, gb_addr 
                29, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 14640, 0,   // baseaddr, len, ddr_addr, gb_addr 
                30, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 488, 15128, 0,   // baseaddr, len, ddr_addr, gb_addr 
                31, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 16, 15620, 0,   // baseaddr, len, ddr_addr, gb_addr 
                0, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 16, 15636, 0,   // baseaddr, len, ddr_addr, gb_addr 
                1, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 16, 15652, 0,   // baseaddr, len, ddr_addr, gb_addr 
                2, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 16, 15668, 0,   // baseaddr, len, ddr_addr, gb_addr 
                3, 3, 0);                      // gb_idx, gb_mux, direction
  // Computing conv_1_0_0
  // t:0-39, oc:0-0, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
                  1, 6, 39, 7, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 0, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 280, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:1-1, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 21, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 281, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:2-2, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 42, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 282, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:3-3, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 63, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 283, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:4-4, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 84, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 284, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:5-5, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 105, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 285, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:6-6, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 126, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 286, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_1_0_0
  // t:0-39, oc:7-7, ic:0-6, k:0-2
  // t_meu:40, OC:8, IC:7, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 7, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 147, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 287, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:0-0, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_move_fbuf2lbuf(&dla, 280, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 39, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 168, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 600, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:1-1, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 192, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 601, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:2-2, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 216, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 602, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:3-3, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 240, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 603, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:4-4, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 264, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 604, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:5-5, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 288, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 605, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:6-6, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 312, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 606, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing conv_2_0_0
  // t:0-39, oc:7-7, ic:0-7, k:0-2
  // t_meu:40, OC:8, IC:8, K:3
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    3, 11, 41, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    1, 1, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 336, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 607, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:0-0, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 600, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 39, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 360, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 0, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:1-1, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 368, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 1,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 1, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:2-2, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 376, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 2,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 2, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:3-3, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 384, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 3,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 3, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:4-4, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 392, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 4, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:5-5, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 400, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 5,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 5, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:6-6, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 408, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 6,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 6, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_1_0_0
  // t:0-39, oc:7-7, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 416, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 7,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 7, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:0-0, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 39, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 424, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 320, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:1-1, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 432, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 9,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 321, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:2-2, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 440, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 10,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 322, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:3-3, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 448, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 11,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 323, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:4-4, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 456, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 324, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:5-5, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 464, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 13,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 325, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:6-6, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 472, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 14,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 326, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing fc_2_0_0
  // t:0-39, oc:7-7, ic:0-7, k:0-0
  // t_meu:40, OC:8, IC:8, K:1
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 12, 39, 39, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 480, 0x00, 15, 39       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 15,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 40, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 327, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 8,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  dif_dla_move_ddr2gb(&dla, 320, 21628, 320,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 320, 21948, 320,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 320, 22268, 320,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 320, 22588, 320,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction


  dif_dla_move_ddr2gb(&dla, 2048, 30000, 0,   // baseaddr, len, ddr_addr, gb_addr
          0, 0, 1);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 2048, 32048, 0,   // baseaddr, len, ddr_addr, gb_addr
          1, 0, 1);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 2048, 34096, 0,   // baseaddr, len, ddr_addr, gb_addr
          2, 0, 1);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 2048, 36144, 0,   // baseaddr, len, ddr_addr, gb_addr
          3, 0, 1);            // gb_idx, gb_mux, direction


  LOG_INFO("gpio[0] = 0xf;");
}



void lpcnet1_wrapper(void){
  //init ddr
  write_to_ddr_from_spi(&dla, &spi, DDRIN_SIZE);
  LOG_INFO("DDR initialized!");
  CHECK(dif_dla_set_ppe(&dla, 0xffff, 0xffff) == kDifDlaOk);

  uint64_t current_time;
  uint64_t kDeadline =
      (kDeviceType == kDeviceSimDV) ? 100 /* 100 us */ : 30000 /* 30 ms */;
  CHECK(dif_rv_timer_counter_read(&timer, kHart, &current_time) ==
        kDifRvTimerOk);
  LOG_INFO("Current time: %d; timer theshold: %d", (uint32_t)current_time,
           (uint32_t)(current_time + kDeadline));
  CHECK(dif_rv_timer_counter_set_enabled(&timer, kHart, kDifRvTimerEnabled) ==
        kDifRvTimerOk);
  lpcnet1();

  CHECK(dif_rv_timer_counter_set_enabled(&timer, kHart, kDifRvTimerDisabled) ==
        kDifRvTimerOk);
  CHECK(dif_rv_timer_counter_read(&timer, kHart, &current_time) ==
        kDifRvTimerOk);
  LOG_INFO("Current time: %d; timer theshold: %d", (uint32_t)current_time,
           (uint32_t)(current_time + kDeadline));

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

  mmio_region_t timer_reg =
      mmio_region_from_addr(TOP_EARLGREY_RV_TIMER_BASE_ADDR);
  CHECK(dif_rv_timer_init(
            timer_reg,
            (dif_rv_timer_config_t){.hart_count = 1, .comparator_count = 1},
            &timer) == kDifRvTimerOk);

  // Add DATE and TIME because I keep fooling myself with old versions
  LOG_INFO("Hello lpcnet1!");
  LOG_INFO("Built at: " __DATE__ ", " __TIME__);

  CHECK(dif_dla_init(
            (dif_dla_params_t){
              .base_addr = mmio_region_from_addr(TOP_EARLGREY_DLA_BASE_ADDR),
            },&dla) == kDifDlaOk);
  LOG_INFO("DLA initialized!");

  LOG_INFO("Waiting for DDR calibration.");
  CHECK(dif_dla_ddr_ctrl_init_calib(&dla)== kDifDlaOk);
  LOG_INFO("DDR calibration completed.");
  
  // ddr_test(&dla, &spi, DDRIN_SIZE);
  // LOG_INFO("DDR-SPI sanity test finished.");

  lpcnet1_wrapper();
  
  uint32_t gpio_state = 0;
  while (true) {
    usleep(10 * 1000);  // 10 ms
    gpio_state = demo_gpio_to_log_echo(&gpio, gpio_state);
    demo_spi_to_log_echo(&spi);
    demo_uart_to_uart_and_gpio_echo(&uart, &gpio);
  }
}
