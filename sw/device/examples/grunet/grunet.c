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
//python3 util/simplespi/spitest.py -i grunet_ddr_in.txt

//python script to receive same ddr contents and send to spi
//python3 util/simplespi/spitest.py -r 196608


//used for ddr_spi functions
//DDRIN_LINES needs to be divisible by 128,
//or DDRIN_SIZE needs to be divisible by 1024,
//so SPI device can send integer patches of 1024B
const uint32_t DDRIN_LINES = 12928; //grunet_ddr_in.txt
const uint32_t DDRIN_SIZE = DDRIN_LINES * 8 ; //bytes

//used for reading wavenet result
//ddr read start address
const uint32_t RD_DDR_ADDR = 20000; //64-bit addressable in unsigned decimal
//ddr read bytes
const uint32_t RD_DDR_BYTES = 65536; //bytes


void grunet(void){

  uint32_t act_k_tanh_pos[16] = {0x157e, 0x1bf8, 0x21c4, 0x2825, 0x2dd6, 0x33c6, 0x3878, 0x3b75,
                      0x3b75, 0x3878, 0x33c6, 0x2dd6, 0x2825, 0x21c4, 0x1bf8, 0x157e};
  uint32_t act_b_tanh_pos[16] = {0xbbf3, 0xbbe0, 0xbbb0, 0xbb3f, 0xba43, 0xb85e, 0xb2ef, 0xa21e,
                      0x221e, 0x32ef, 0x385e, 0x3a43, 0x3b3f, 0x3bb0, 0x3be0, 0x3bf3};
  uint32_t act_x_tanh_pos[15] = {0xc377, 0xc266, 0xc155, 0xc044, 0xbe66, 0xbc44, 0xb844, 0x0000,
                      0x3844, 0x3c44, 0x3e66, 0x4044, 0x4155, 0x4266, 0x4377};

  uint32_t act_k_tanh_neg[16] = {0x957e, 0x9bf8, 0xa1c4, 0xa825, 0xadd6, 0xb3c6, 0xb878, 0xbb75,
                          0xbb75, 0xb878, 0xb3c6, 0xadd6, 0xa825, 0xa1c4, 0x9bf8, 0x957e};
  uint32_t act_b_tanh_neg[16] = {0x3bf3, 0x3be0, 0x3bb0, 0x3b3f, 0x3a43, 0x385e, 0x32ef, 0x221e,
                          0xa21e, 0xb2ef, 0xb85e, 0xba43, 0xbb3f, 0xbbb0, 0xbbe0, 0xbbf3};
  uint32_t act_x_tanh_neg[15] = {0xc377, 0xc266, 0xc155, 0xc044, 0xbe66, 0xbc44, 0xb844, 0x0000,
                          0x3844, 0x3c44, 0x3e66, 0x4044, 0x4155, 0x4266, 0x4377};

  uint32_t act_k_sigmoid_pos[16] = {0x190d, 0x1d96, 0x2222, 0x26a0, 0x2aea, 0x2eb8, 0x31b1, 0x33b0,
                           0x33b0, 0x31b1, 0x2eb8, 0x2aea, 0x26a0, 0x2222, 0x1d96, 0x190d};
  uint32_t act_b_sigmoid_pos[16] = {0x246b, 0x2855, 0x2c26, 0x2faa, 0x32ad, 0x3544, 0x371e, 0x37f5,
                           0x3805, 0x3870, 0x395d, 0x3a54, 0x3b0a, 0x3b7b, 0x3bba, 0x3bdc};
  uint32_t act_x_sigmoid_pos[15] = {0xc599, 0xc4cc, 0xc3ff, 0xc266, 0xc0cc, 0xbe66, 0xba65, 0x0000,
                           0x3a65, 0x3e66, 0x40cc, 0x4266, 0x43ff, 0x44cc, 0x4599};

  uint32_t act_k_sigmoid_neg[16] = {0x990d, 0x9d96, 0xa222, 0xa6a0, 0xaaea, 0xaeb8, 0xb1b1, 0xb3b0,
                              0xb3b0, 0xb1b1, 0xaeb8, 0xaaea, 0xa6a0, 0xa222, 0x9d96, 0x990d};
  uint32_t act_b_sigmoid_neg[16] = {0xa46b, 0xa855, 0xac26, 0xafaa, 0xb2ad, 0xb544, 0xb71e, 0xb7f5,
                              0xb805, 0xb870, 0xb95d, 0xba54, 0xbb0a, 0xbb7b, 0xbbba, 0xbbdc};
  uint32_t act_x_sigmoid_neg[15] = {0xc599, 0xc4cc, 0xc3ff, 0xc266, 0xc0cc, 0xbe66, 0xba65, 0x0000,
                              0x3a65, 0x3e66, 0x40cc, 0x4266, 0x43ff, 0x44cc, 0x4599};

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

  dif_dla_move_ddr2gb(&dla, 32, 12788, 0,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 32, 12818, 0,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 32, 12848, 0,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 32, 12878, 0,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 60, 12328, 30,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 60, 12388, 30,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 60, 12448, 30,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 60, 12508, 30,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 32, 12572, 90,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 32, 12602, 90,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 32, 12632, 90,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 32, 12662, 90,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 20, 12704, 120,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 20, 12724, 120,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 20, 12744, 120,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 20, 12764, 120,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 0, 0,   // baseaddr, len, ddr_addr, gb_addr 
                0, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 377, 0,   // baseaddr, len, ddr_addr, gb_addr 
                1, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 754, 0,   // baseaddr, len, ddr_addr, gb_addr 
                2, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 1131, 0,   // baseaddr, len, ddr_addr, gb_addr 
                3, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 1508, 0,   // baseaddr, len, ddr_addr, gb_addr 
                4, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 1885, 0,   // baseaddr, len, ddr_addr, gb_addr 
                5, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 2262, 0,   // baseaddr, len, ddr_addr, gb_addr 
                6, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 2639, 0,   // baseaddr, len, ddr_addr, gb_addr 
                7, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 3016, 0,   // baseaddr, len, ddr_addr, gb_addr 
                8, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 3393, 0,   // baseaddr, len, ddr_addr, gb_addr 
                9, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 3770, 0,   // baseaddr, len, ddr_addr, gb_addr 
                10, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 4147, 0,   // baseaddr, len, ddr_addr, gb_addr 
                11, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 4524, 0,   // baseaddr, len, ddr_addr, gb_addr 
                12, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 4901, 0,   // baseaddr, len, ddr_addr, gb_addr 
                13, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 5278, 0,   // baseaddr, len, ddr_addr, gb_addr 
                14, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 5655, 0,   // baseaddr, len, ddr_addr, gb_addr 
                15, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 6032, 0,   // baseaddr, len, ddr_addr, gb_addr 
                16, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 6409, 0,   // baseaddr, len, ddr_addr, gb_addr 
                17, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 6786, 0,   // baseaddr, len, ddr_addr, gb_addr 
                18, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 7163, 0,   // baseaddr, len, ddr_addr, gb_addr 
                19, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 7540, 0,   // baseaddr, len, ddr_addr, gb_addr 
                20, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 7917, 0,   // baseaddr, len, ddr_addr, gb_addr 
                21, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 8294, 0,   // baseaddr, len, ddr_addr, gb_addr 
                22, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 8671, 0,   // baseaddr, len, ddr_addr, gb_addr 
                23, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 9048, 0,   // baseaddr, len, ddr_addr, gb_addr 
                24, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 9425, 0,   // baseaddr, len, ddr_addr, gb_addr 
                25, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 9802, 0,   // baseaddr, len, ddr_addr, gb_addr 
                26, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 10179, 0,   // baseaddr, len, ddr_addr, gb_addr 
                27, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 10556, 0,   // baseaddr, len, ddr_addr, gb_addr 
                28, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 10933, 0,   // baseaddr, len, ddr_addr, gb_addr 
                29, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 11310, 0,   // baseaddr, len, ddr_addr, gb_addr 
                30, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 380, 11687, 0,   // baseaddr, len, ddr_addr, gb_addr 
                31, 1, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 40, 12164, 0,   // baseaddr, len, ddr_addr, gb_addr 
                0, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 40, 12202, 0,   // baseaddr, len, ddr_addr, gb_addr 
                1, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 40, 12240, 0,   // baseaddr, len, ddr_addr, gb_addr 
                2, 3, 0);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 40, 12278, 0,   // baseaddr, len, ddr_addr, gb_addr 
                3, 3, 0);                      // gb_idx, gb_mux, direction
  // Computing mat_mul1_0_0
  // t:0-0, oc:0-1, ic:0-2, k:0-0
  // t_meu:10, OC:2, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 0, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 140, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing gru_1_xz_gate_0_0
  // t:0-0, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 140, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 6, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 142, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xr_gate_0_0
  // t:0-0, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 140, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 10, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 144, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hz_gate_0_0
  // t:0-0, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 138, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 14, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    142, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 146, 2,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hr_gate_0_0
  // t:0-0, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 138, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 18, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    144, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    138, 142, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    146, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 144, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    138, 146, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xh_gate_0_0
  // t:0-0, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 140, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 22, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 148, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hh_gate_0_0
  // t:0-0, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 142, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 26, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    148, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 140, 6,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    140, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    144, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    146, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    138, 142, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2_0_0
  // t:0-0, oc:0-0, ic:0-1, k:0-0
  // t_meu:10, OC:1, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 142, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 30, 0x00, 15, 0       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 140, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // Computing gru_2_xz_gate_0_0
  // t:0-0, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 32, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 144, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xr_gate_0_0
  // t:0-0, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 50, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hz_gate_0_0
  // t:0-0, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 68, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    144, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 150, 9,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hr_gate_0_0
  // t:0-0, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 77, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    117, 144, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    150, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    117, 150, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xh_gate_0_0
  // t:0-0, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 86, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hh_gate_0_0
  // t:0-0, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 144, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 104, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 156, 15,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    156, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    150, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    117, 144, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xz_gate_0_0
  // t:0-0, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 113, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xr_gate_0_0
  // t:0-0, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 161, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hz_gate_0_0
  // t:0-0, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 209, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 159, 18,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hr_gate_0_0
  // t:0-0, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 245, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 24,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    84, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    84, 159, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xh_gate_0_0
  // t:0-0, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 281, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 165, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hh_gate_0_0
  // t:0-0, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 329, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    165, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 171, 30,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    171, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    84, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul3_0_0
  // t:0-0, oc:0-1, ic:0-5, k:0-0
  // t_meu:10, OC:2, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 365, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 36,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 0, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // Computing mat_mul1_0_0
  // t:1-1, oc:0-1, ic:0-2, k:0-0
  // t_meu:10, OC:2, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 3, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 0, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 84, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing gru_1_xz_gate_0_0
  // t:1-1, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 6, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 86, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xr_gate_0_0
  // t:1-1, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 10, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 88, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hz_gate_0_0
  // t:1-1, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 142, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 14, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    86, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 117, 2,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hr_gate_0_0
  // t:1-1, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 142, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 18, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    88, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    142, 86, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    117, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 88, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    142, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xh_gate_0_0
  // t:1-1, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 22, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 138, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hh_gate_0_0
  // t:1-1, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 86, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 26, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    138, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 84, 6,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    84, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    88, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    117, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    142, 86, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2_0_0
  // t:1-1, oc:0-0, ic:0-1, k:0-0
  // t_meu:10, OC:1, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 86, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 30, 0x00, 15, 0       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 2, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // Computing gru_2_xz_gate_0_0
  // t:1-1, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 3, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 32, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xr_gate_0_0
  // t:1-1, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 3, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 50, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hz_gate_0_0
  // t:1-1, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 144, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 68, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    117, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 153, 9,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hr_gate_0_0
  // t:1-1, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 144, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 77, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    144, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    144, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xh_gate_0_0
  // t:1-1, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 3, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 86, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 156, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hh_gate_0_0
  // t:1-1, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 104, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    156, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 159, 15,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    144, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xz_gate_0_0
  // t:1-1, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 3, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 113, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xr_gate_0_0
  // t:1-1, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 3, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 161, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hz_gate_0_0
  // t:1-1, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 209, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 159, 18,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hr_gate_0_0
  // t:1-1, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 245, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 24,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    147, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    147, 159, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xh_gate_0_0
  // t:1-1, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 3, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 281, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 165, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hh_gate_0_0
  // t:1-1, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 141, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 329, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    165, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 171, 30,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    171, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    147, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul3_0_0
  // t:1-1, oc:0-1, ic:0-5, k:0-0
  // t_meu:10, OC:2, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 141, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 365, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 36,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 3, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // Computing mat_mul1_0_0
  // t:2-2, oc:0-1, ic:0-2, k:0-0
  // t_meu:10, OC:2, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 0, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 84, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing gru_1_xz_gate_0_0
  // t:2-2, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 6, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 88, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xr_gate_0_0
  // t:2-2, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 10, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 138, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hz_gate_0_0
  // t:2-2, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 86, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 14, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    88, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 147, 2,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hr_gate_0_0
  // t:2-2, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 86, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 18, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    138, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    86, 88, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 138, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    86, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xh_gate_0_0
  // t:2-2, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 22, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 149, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hh_gate_0_0
  // t:2-2, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 88, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 26, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    149, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 84, 6,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    84, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    138, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    86, 88, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2_0_0
  // t:2-2, oc:0-0, ic:0-1, k:0-0
  // t_meu:10, OC:1, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 88, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 30, 0x00, 15, 0       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 5, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // Computing gru_2_xz_gate_0_0
  // t:2-2, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 32, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 84, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xr_gate_0_0
  // t:2-2, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 50, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hz_gate_0_0
  // t:2-2, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 68, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    84, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 150, 9,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hr_gate_0_0
  // t:2-2, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 77, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    117, 84, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    150, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    117, 150, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xh_gate_0_0
  // t:2-2, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 86, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hh_gate_0_0
  // t:2-2, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 104, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 156, 15,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    156, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    150, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    117, 84, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xz_gate_0_0
  // t:2-2, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 113, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xr_gate_0_0
  // t:2-2, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 161, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hz_gate_0_0
  // t:2-2, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 141, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 209, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 159, 18,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hr_gate_0_0
  // t:2-2, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 141, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 245, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 24,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    141, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    141, 159, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xh_gate_0_0
  // t:2-2, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 281, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 165, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hh_gate_0_0
  // t:2-2, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 329, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    165, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 171, 30,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    171, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    141, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul3_0_0
  // t:2-2, oc:0-1, ic:0-5, k:0-0
  // t_meu:10, OC:2, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 365, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 36,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 6, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // Computing mat_mul1_0_0
  // t:3-3, oc:0-1, ic:0-2, k:0-0
  // t_meu:10, OC:2, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 9, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 0, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing gru_1_xz_gate_0_0
  // t:3-3, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 6, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 138, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xr_gate_0_0
  // t:3-3, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 10, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hz_gate_0_0
  // t:3-3, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 88, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 14, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    138, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 143, 2,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hr_gate_0_0
  // t:3-3, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 88, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 18, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    88, 138, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    143, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    88, 143, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xh_gate_0_0
  // t:3-3, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 22, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 145, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hh_gate_0_0
  // t:3-3, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 138, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 26, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    145, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 117, 6,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    117, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    143, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    88, 138, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2_0_0
  // t:3-3, oc:0-0, ic:0-1, k:0-0
  // t_meu:10, OC:1, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 138, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 30, 0x00, 15, 0       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 8, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // Computing gru_2_xz_gate_0_0
  // t:3-3, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 9, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 32, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 87, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xr_gate_0_0
  // t:3-3, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 9, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 50, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hz_gate_0_0
  // t:3-3, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 68, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    87, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 141, 9,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hr_gate_0_0
  // t:3-3, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 77, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    117, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    84, 87, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    84, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xh_gate_0_0
  // t:3-3, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 9, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 86, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 144, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hh_gate_0_0
  // t:3-3, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 87, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 104, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    144, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 153, 15,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    117, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    84, 87, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xz_gate_0_0
  // t:3-3, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 9, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 113, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xr_gate_0_0
  // t:3-3, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 9, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 161, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hz_gate_0_0
  // t:3-3, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 209, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 159, 18,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hr_gate_0_0
  // t:3-3, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 245, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 24,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    147, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    147, 159, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xh_gate_0_0
  // t:3-3, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 9, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 281, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 165, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hh_gate_0_0
  // t:3-3, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 141, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 329, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    165, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 171, 30,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    171, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    147, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul3_0_0
  // t:3-3, oc:0-1, ic:0-5, k:0-0
  // t_meu:10, OC:2, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 141, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 365, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 36,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 9, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // Computing mat_mul1_0_0
  // t:4-4, oc:0-1, ic:0-2, k:0-0
  // t_meu:10, OC:2, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 12, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 0, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 84, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing gru_1_xz_gate_0_0
  // t:4-4, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 6, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xr_gate_0_0
  // t:4-4, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 10, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hz_gate_0_0
  // t:4-4, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 138, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 14, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    117, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 149, 2,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hr_gate_0_0
  // t:4-4, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 138, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 18, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    138, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    149, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    138, 149, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xh_gate_0_0
  // t:4-4, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 22, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 151, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hh_gate_0_0
  // t:4-4, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 26, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    151, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 84, 6,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    84, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    149, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    138, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2_0_0
  // t:4-4, oc:0-0, ic:0-1, k:0-0
  // t_meu:10, OC:1, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 30, 0x00, 15, 0       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 11, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // Computing gru_2_xz_gate_0_0
  // t:4-4, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 12, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 32, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 84, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xr_gate_0_0
  // t:4-4, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 12, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 50, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hz_gate_0_0
  // t:4-4, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 87, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 68, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    84, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 150, 9,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hr_gate_0_0
  // t:4-4, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 87, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 77, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    87, 84, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    150, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    87, 150, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xh_gate_0_0
  // t:4-4, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 12, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 86, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hh_gate_0_0
  // t:4-4, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 104, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 156, 15,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    156, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    150, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    87, 84, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xz_gate_0_0
  // t:4-4, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 12, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 113, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xr_gate_0_0
  // t:4-4, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 12, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 161, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hz_gate_0_0
  // t:4-4, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 141, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 209, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 159, 18,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hr_gate_0_0
  // t:4-4, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 141, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 245, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 24,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    141, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    141, 159, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xh_gate_0_0
  // t:4-4, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 12, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 281, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 165, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hh_gate_0_0
  // t:4-4, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 329, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    165, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 171, 30,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    171, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    141, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul3_0_0
  // t:4-4, oc:0-1, ic:0-5, k:0-0
  // t_meu:10, OC:2, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 365, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 36,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 12, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // Computing mat_mul1_0_0
  // t:5-5, oc:0-1, ic:0-2, k:0-0
  // t_meu:10, OC:2, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 15, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 0, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 87, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing gru_1_xz_gate_0_0
  // t:5-5, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 87, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 6, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 138, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xr_gate_0_0
  // t:5-5, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 87, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 10, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hz_gate_0_0
  // t:5-5, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 14, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    138, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 143, 2,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hr_gate_0_0
  // t:5-5, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 18, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    117, 138, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    143, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    117, 143, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xh_gate_0_0
  // t:5-5, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 87, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 22, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 145, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hh_gate_0_0
  // t:5-5, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 138, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 26, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    145, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 87, 6,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    87, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    143, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    117, 138, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2_0_0
  // t:5-5, oc:0-0, ic:0-1, k:0-0
  // t_meu:10, OC:1, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 138, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 30, 0x00, 15, 0       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 14, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // Computing gru_2_xz_gate_0_0
  // t:5-5, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 15, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 32, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 87, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xr_gate_0_0
  // t:5-5, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 15, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 50, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hz_gate_0_0
  // t:5-5, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 68, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    87, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 141, 9,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hr_gate_0_0
  // t:5-5, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 77, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    117, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    84, 87, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    84, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xh_gate_0_0
  // t:5-5, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 15, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 86, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 144, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hh_gate_0_0
  // t:5-5, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 87, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 104, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    144, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 153, 15,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    117, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    84, 87, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xz_gate_0_0
  // t:5-5, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 15, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 113, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xr_gate_0_0
  // t:5-5, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 15, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 161, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hz_gate_0_0
  // t:5-5, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 209, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 159, 18,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hr_gate_0_0
  // t:5-5, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 245, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 24,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    147, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    147, 159, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xh_gate_0_0
  // t:5-5, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 15, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 281, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 165, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hh_gate_0_0
  // t:5-5, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 141, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 329, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    165, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 171, 30,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    171, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    147, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul3_0_0
  // t:5-5, oc:0-1, ic:0-5, k:0-0
  // t_meu:10, OC:2, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 141, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 365, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 36,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 15, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // Computing mat_mul1_0_0
  // t:6-6, oc:0-1, ic:0-2, k:0-0
  // t_meu:10, OC:2, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 18, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 0, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 84, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing gru_1_xz_gate_0_0
  // t:6-6, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 6, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xr_gate_0_0
  // t:6-6, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 10, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hz_gate_0_0
  // t:6-6, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 138, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 14, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    117, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 149, 2,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hr_gate_0_0
  // t:6-6, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 138, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 18, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    138, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    149, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    138, 149, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xh_gate_0_0
  // t:6-6, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 22, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 151, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hh_gate_0_0
  // t:6-6, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 26, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    151, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 84, 6,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    84, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    149, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    138, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2_0_0
  // t:6-6, oc:0-0, ic:0-1, k:0-0
  // t_meu:10, OC:1, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 30, 0x00, 15, 0       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 17, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // Computing gru_2_xz_gate_0_0
  // t:6-6, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 18, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 32, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 84, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xr_gate_0_0
  // t:6-6, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 18, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 50, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hz_gate_0_0
  // t:6-6, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 87, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 68, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    84, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 150, 9,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hr_gate_0_0
  // t:6-6, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 87, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 77, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    87, 84, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    150, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    87, 150, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xh_gate_0_0
  // t:6-6, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 18, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 86, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hh_gate_0_0
  // t:6-6, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 104, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 156, 15,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    156, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    150, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    87, 84, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xz_gate_0_0
  // t:6-6, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 18, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 113, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xr_gate_0_0
  // t:6-6, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 18, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 161, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hz_gate_0_0
  // t:6-6, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 141, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 209, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 159, 18,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hr_gate_0_0
  // t:6-6, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 141, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 245, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 24,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    141, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    141, 159, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xh_gate_0_0
  // t:6-6, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 18, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 281, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 165, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hh_gate_0_0
  // t:6-6, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 329, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    165, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 171, 30,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    171, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    141, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul3_0_0
  // t:6-6, oc:0-1, ic:0-5, k:0-0
  // t_meu:10, OC:2, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 365, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 36,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 18, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // Computing mat_mul1_0_0
  // t:7-7, oc:0-1, ic:0-2, k:0-0
  // t_meu:10, OC:2, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 21, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 0, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 87, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing gru_1_xz_gate_0_0
  // t:7-7, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 87, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 6, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 138, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xr_gate_0_0
  // t:7-7, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 87, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 10, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hz_gate_0_0
  // t:7-7, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 14, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    138, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 143, 2,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hr_gate_0_0
  // t:7-7, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 18, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    117, 138, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    143, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    117, 143, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xh_gate_0_0
  // t:7-7, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 87, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 22, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 145, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hh_gate_0_0
  // t:7-7, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 138, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 26, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    145, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 87, 6,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    87, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    143, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    117, 138, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2_0_0
  // t:7-7, oc:0-0, ic:0-1, k:0-0
  // t_meu:10, OC:1, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 138, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 30, 0x00, 15, 0       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 20, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // Computing gru_2_xz_gate_0_0
  // t:7-7, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 21, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 32, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 87, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xr_gate_0_0
  // t:7-7, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 21, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 50, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hz_gate_0_0
  // t:7-7, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 68, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    87, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 141, 9,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hr_gate_0_0
  // t:7-7, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 77, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    117, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    84, 87, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    84, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xh_gate_0_0
  // t:7-7, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 21, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 86, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 144, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hh_gate_0_0
  // t:7-7, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 87, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 104, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    144, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 153, 15,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    117, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    84, 87, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xz_gate_0_0
  // t:7-7, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 21, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 113, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xr_gate_0_0
  // t:7-7, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 21, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 161, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hz_gate_0_0
  // t:7-7, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 209, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 159, 18,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hr_gate_0_0
  // t:7-7, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 245, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 24,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    147, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    147, 159, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xh_gate_0_0
  // t:7-7, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 21, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 281, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 165, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hh_gate_0_0
  // t:7-7, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 141, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 329, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    165, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 171, 30,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    171, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    147, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul3_0_0
  // t:7-7, oc:0-1, ic:0-5, k:0-0
  // t_meu:10, OC:2, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 141, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 365, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 36,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 21, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // Computing mat_mul1_0_0
  // t:8-8, oc:0-1, ic:0-2, k:0-0
  // t_meu:10, OC:2, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 24, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 0, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 84, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing gru_1_xz_gate_0_0
  // t:8-8, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 6, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xr_gate_0_0
  // t:8-8, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 10, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hz_gate_0_0
  // t:8-8, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 138, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 14, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    117, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 149, 2,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hr_gate_0_0
  // t:8-8, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 138, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 18, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    138, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    149, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    138, 149, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xh_gate_0_0
  // t:8-8, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 22, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 151, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hh_gate_0_0
  // t:8-8, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 26, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    151, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 84, 6,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    84, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    149, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    138, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2_0_0
  // t:8-8, oc:0-0, ic:0-1, k:0-0
  // t_meu:10, OC:1, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 30, 0x00, 15, 0       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 23, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // Computing gru_2_xz_gate_0_0
  // t:8-8, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 24, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 32, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 84, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xr_gate_0_0
  // t:8-8, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 24, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 50, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hz_gate_0_0
  // t:8-8, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 87, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 68, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    84, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 150, 9,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hr_gate_0_0
  // t:8-8, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 87, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 77, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    87, 84, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    150, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    87, 150, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xh_gate_0_0
  // t:8-8, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 24, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 86, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hh_gate_0_0
  // t:8-8, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 104, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 156, 15,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    156, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    150, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    87, 84, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xz_gate_0_0
  // t:8-8, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 24, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 113, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xr_gate_0_0
  // t:8-8, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 24, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 161, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hz_gate_0_0
  // t:8-8, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 141, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 209, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    147, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 159, 18,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hr_gate_0_0
  // t:8-8, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 141, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 245, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 24,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    141, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    141, 159, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xh_gate_0_0
  // t:8-8, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 24, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 281, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 165, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hh_gate_0_0
  // t:8-8, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 329, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    165, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 171, 30,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    171, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    141, 147, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul3_0_0
  // t:8-8, oc:0-1, ic:0-5, k:0-0
  // t_meu:10, OC:2, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 365, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 36,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 24, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // Computing mat_mul1_0_0
  // t:9-9, oc:0-1, ic:0-2, k:0-0
  // t_meu:10, OC:2, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 27, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 0, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 0,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 87, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
  );

  // Computing gru_1_xz_gate_0_0
  // t:9-9, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 87, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 6, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 138, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xr_gate_0_0
  // t:9-9, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 87, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 10, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hz_gate_0_0
  // t:9-9, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 14, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    138, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 143, 2,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hr_gate_0_0
  // t:9-9, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 117, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 18, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 4,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    117, 138, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    143, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    117, 143, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_xh_gate_0_0
  // t:9-9, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 87, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 22, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 145, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_hh_gate_0_0
  // t:9-9, oc:0-1, ic:0-1, k:0-0
  // t_meu:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 138, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 26, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    145, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 2, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 87, 6,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_1_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:2, IC:2, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    87, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    143, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    117, 138, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2_0_0
  // t:9-9, oc:0-0, ic:0-1, k:0-0
  // t_meu:10, OC:1, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 138, 0,  // baseaddr, src_addr, dest_addr
                  1, 1, 0, 2, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 2, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 30, 0x00, 15, 0       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 8,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    0, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 26, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // Computing gru_2_xz_gate_0_0
  // t:9-9, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 27, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 32, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 87, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xr_gate_0_0
  // t:9-9, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 27, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 50, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hz_gate_0_0
  // t:9-9, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 68, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    87, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 141, 9,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hr_gate_0_0
  // t:9-9, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 84, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 77, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    117, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 12,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    84, 87, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 117, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    84, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_xh_gate_0_0
  // t:9-9, oc:0-2, ic:0-5, k:0-0
  // t_meu:10, OC:3, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 27, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 86, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 144, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_hh_gate_0_0
  // t:9-9, oc:0-2, ic:0-2, k:0-0
  // t_meu:10, OC:3, IC:3, K:1
  dif_dla_move_fbuf2lbuf(&dla, 87, 0,  // baseaddr, src_addr, dest_addr
                  1, 2, 0, 3, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 3, 3,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 3,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 104, 0x00, 15, 2       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    144, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 3, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 153, 15,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 3,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_2_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:3, IC:3, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    117, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    84, 87, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xz_gate_0_0
  // t:9-9, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 27, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 113, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xr_gate_0_0
  // t:9-9, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 27, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 161, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hz_gate_0_0
  // t:9-9, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 209, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    141, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 159, 18,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hr_gate_0_0
  // t:9-9, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 147, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 245, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 24,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    147, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_z_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    11111, 153, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    147, 159, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_xh_gate_0_0
  // t:9-9, oc:0-5, ic:0-7, k:0-0
  // t_meu:10, OC:6, IC:8, K:1
  dif_dla_move_fbuf2lbuf(&dla, 27, 0,  // baseaddr, src_addr, dest_addr
                  1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 8, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 281, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    11111, 165, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_hh_gate_0_0
  // t:9-9, oc:0-5, ic:0-5, k:0-0
  // t_meu:10, OC:6, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 141, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 6,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 6,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 329, 0x00, 15, 5       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    165, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    1, 1, 6, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    11111, 171, 30,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 6,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing gru_3_h_act_0_0
  // oc_idx:0
  // t_meu:10, OC:6, IC:6, K:1
  // rst
  dif_dla_pbuf_rst(&dla, 64);
  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    171, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh_neg
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    153, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    159, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    5, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    147, 141, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul3_0_0
  // t:9-9, oc:0-1, ic:0-5, k:0-0
  // t_meu:10, OC:2, IC:6, K:1
  dif_dla_move_fbuf2lbuf(&dla, 141, 0,  // baseaddr, src_addr, dest_addr
                  1, 5, 0, 6, 0);       // skip, iter, len, dila, mode
  dif_dla_conv_comp(
    &dla, 0,                                   // baseaddr, mode_spar
    1, 13, 0, 0, 6, 2,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    0, 0, 0, 1, 2,         // pad_left, pad_right, pad_num, sub_col, sub_row 
    0, 365, 0x00, 15, 1       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );
  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    11111, 11111, 36,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    1, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    11111, 27, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    11111, 1, 11111, 2,                 // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
  );

  dif_dla_move_ddr2gb(&dla, 4, 12920, 140,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12921, 2,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12922, 5,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12923, 8,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12924, 11,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12925, 14,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12926, 17,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12927, 20,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12928, 23,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12929, 26,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12930, 140,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12931, 2,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12932, 5,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12933, 8,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12934, 11,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12935, 14,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12936, 17,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12937, 20,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12938, 23,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12939, 26,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12940, 140,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12941, 2,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12942, 5,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12943, 8,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12944, 11,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12945, 14,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12946, 17,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12947, 20,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12948, 23,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12949, 26,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12950, 140,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12951, 2,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12952, 5,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12953, 8,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12954, 11,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12955, 14,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12956, 17,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12957, 20,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12958, 23,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12959, 26,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12972, 0,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12974, 3,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12976, 6,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12978, 9,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12980, 12,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12982, 15,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12984, 18,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12986, 21,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12988, 24,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12990, 27,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12992, 0,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12994, 3,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12996, 6,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 12998, 9,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13000, 12,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13002, 15,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13004, 18,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13006, 21,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13008, 24,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13010, 27,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13012, 0,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13014, 3,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13016, 6,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13018, 9,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13020, 12,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13022, 15,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13024, 18,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13026, 21,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13028, 24,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13030, 27,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13032, 0,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13034, 3,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13036, 6,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13038, 9,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13040, 12,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13042, 15,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13044, 18,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13046, 21,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13048, 24,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13050, 27,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 56, 13056, 30,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 8, 13110, 141,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 56, 13116, 30,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 8, 13170, 141,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 56, 13176, 30,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 8, 13230, 141,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 56, 13236, 30,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 8, 13290, 141,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 28, 13300, 90,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13327, 87,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 28, 13330, 90,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13357, 87,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 28, 13360, 90,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13387, 87,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 28, 13390, 90,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 4, 13417, 87,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 20, 13432, 120,   // baseaddr, len, ddr_addr, gb_addr 
                0, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 20, 13452, 120,   // baseaddr, len, ddr_addr, gb_addr 
                1, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 20, 13472, 120,   // baseaddr, len, ddr_addr, gb_addr 
                2, 0, 1);                      // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 20, 13492, 120,   // baseaddr, len, ddr_addr, gb_addr 
                3, 0, 1);                      // gb_idx, gb_mux, direction
  
  
  dif_dla_move_ddr2gb(&dla, 2048, 20000, 0,   // baseaddr, len, ddr_addr, gb_addr
        0, 0, 1);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 2048, 22048, 0,   // baseaddr, len, ddr_addr, gb_addr
        1, 0, 1);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 2048, 24096, 0,   // baseaddr, len, ddr_addr, gb_addr
        2, 0, 1);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 2048, 26144, 0,   // baseaddr, len, ddr_addr, gb_addr
        3, 0, 1);            // gb_idx, gb_mux, direction

    
  
  LOG_INFO("gpio[0] = 0xf;");
}



void grunet_wrapper(void){
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
  grunet();

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
  LOG_INFO("Hello grunet!");
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

  grunet_wrapper();
  
  uint32_t gpio_state = 0;
  while (true) {
    usleep(10 * 1000);  // 10 ms
    gpio_state = demo_gpio_to_log_echo(&gpio, gpio_state);
    demo_spi_to_log_echo(&spi);
    demo_uart_to_uart_and_gpio_echo(&uart, &gpio);
  }
}
