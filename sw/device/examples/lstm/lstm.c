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
//python3 util/simplespi/spitest.py -i lstm_ddr_in.txt

//python script to receive same ddr contents and send to spi
//python3 util/simplespi/spitest.py -r 196608


//used for ddr_spi functions
//DDRIN_LINES needs to be divisible by 128,
//or DDRIN_SIZE needs to be divisible by 1024,
//so SPI device can send integer patches of 1024B
const uint32_t DDRIN_LINES = 24576; //lstm_ddr_in.txt
const uint32_t DDRIN_SIZE = DDRIN_LINES * 8 ; //bytes

//used for reading wavenet result
//ddr read start address
const uint32_t RD_DDR_ADDR = 20000; //64-bit addressable in unsigned decimal
//ddr read bytes
const uint32_t RD_DDR_BYTES = 65536; //bytes


void lstm(void){
  // uint64_t current_time;
  // uint64_t kDeadline =
  //     (kDeviceType == kDeviceSimDV) ? 100 /* 100 us */ : 30000 /* 30 ms */;

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

  // meu
  LOG_INFO("gpio[0] = 1;");

  // LOG_INFO("DDR2GB");
  // CHECK(dif_rv_timer_counter_read(&timer, kHart, &current_time) ==
  //       kDifRvTimerOk);
  // LOG_INFO("Current time: %d; timer theshold: %d", (uint32_t)current_time,
  //          (uint32_t)(current_time + kDeadline));
  // CHECK(dif_rv_timer_counter_set_enabled(&timer, kHart, kDifRvTimerEnabled) ==
  //       kDifRvTimerOk);
  
  dif_dla_move_ddr2gb(&dla, 552, 0, 0,   // baseaddr, len, ddr_addr, gb_addr 
        0, 1, 0);            // gb_idx, gb_mux, direction
  
  // CHECK(dif_rv_timer_counter_set_enabled(&timer, kHart, kDifRvTimerDisabled) ==
  //       kDifRvTimerOk);
  // CHECK(dif_rv_timer_counter_read(&timer, kHart, &current_time) ==
  //       kDifRvTimerOk);
  // LOG_INFO("Current time: %d; timer theshold: %d", (uint32_t)current_time,
  //          (uint32_t)(current_time + kDeadline));

  dif_dla_move_ddr2gb(&dla, 552, 552, 0,   // baseaddr, len, ddr_addr, gb_addr 
        1, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 1104, 0,   // baseaddr, len, ddr_addr, gb_addr 
        2, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 1656, 0,   // baseaddr, len, ddr_addr, gb_addr 
        3, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 2208, 0,   // baseaddr, len, ddr_addr, gb_addr 
        4, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 2760, 0,   // baseaddr, len, ddr_addr, gb_addr 
        5, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 3312, 0,   // baseaddr, len, ddr_addr, gb_addr 
        6, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 3864, 0,   // baseaddr, len, ddr_addr, gb_addr 
        7, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 4416, 0,   // baseaddr, len, ddr_addr, gb_addr 
        8, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 4968, 0,   // baseaddr, len, ddr_addr, gb_addr 
        9, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 5520, 0,   // baseaddr, len, ddr_addr, gb_addr 
        10, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 6072, 0,   // baseaddr, len, ddr_addr, gb_addr 
        11, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 6624, 0,   // baseaddr, len, ddr_addr, gb_addr 
        12, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 7176, 0,   // baseaddr, len, ddr_addr, gb_addr 
        13, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 7728, 0,   // baseaddr, len, ddr_addr, gb_addr 
        14, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 8280, 0,   // baseaddr, len, ddr_addr, gb_addr 
        15, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 8832, 0,   // baseaddr, len, ddr_addr, gb_addr 
        16, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 9384, 0,   // baseaddr, len, ddr_addr, gb_addr 
        17, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 9936, 0,   // baseaddr, len, ddr_addr, gb_addr 
        18, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 10488, 0,   // baseaddr, len, ddr_addr, gb_addr 
        19, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 11040, 0,   // baseaddr, len, ddr_addr, gb_addr 
        20, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 11592, 0,   // baseaddr, len, ddr_addr, gb_addr 
        21, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 12144, 0,   // baseaddr, len, ddr_addr, gb_addr 
        22, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 12696, 0,   // baseaddr, len, ddr_addr, gb_addr 
        23, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 13248, 0,   // baseaddr, len, ddr_addr, gb_addr 
        24, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 13800, 0,   // baseaddr, len, ddr_addr, gb_addr 
        25, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 14352, 0,   // baseaddr, len, ddr_addr, gb_addr 
        26, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 14904, 0,   // baseaddr, len, ddr_addr, gb_addr 
        27, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 15456, 0,   // baseaddr, len, ddr_addr, gb_addr 
        28, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 16008, 0,   // baseaddr, len, ddr_addr, gb_addr 
        29, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 16560, 0,   // baseaddr, len, ddr_addr, gb_addr 
        30, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 552, 17112, 0,   // baseaddr, len, ddr_addr, gb_addr 
        31, 1, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 328, 17664, 0,   // baseaddr, len, ddr_addr, gb_addr 
        0, 0, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 328, 17992, 0,   // baseaddr, len, ddr_addr, gb_addr 
        1, 0, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 328, 18320, 0,   // baseaddr, len, ddr_addr, gb_addr 
        2, 0, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 328, 18648, 0,   // baseaddr, len, ddr_addr, gb_addr 
        3, 0, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 144, 18976, 0,   // baseaddr, len, ddr_addr, gb_addr 
        0, 3, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 144, 19120, 0,   // baseaddr, len, ddr_addr, gb_addr 
        1, 3, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 144, 19264, 0,   // baseaddr, len, ddr_addr, gb_addr 
        2, 3, 0);            // gb_idx, gb_mux, direction
  dif_dla_move_ddr2gb(&dla, 144, 19408, 0,   // baseaddr, len, ddr_addr, gb_addr 
        3, 3, 0);            // gb_idx, gb_mux, direction
  // Computing lstm_1_xi_gate
  // t:1-2, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  // LOG_INFO("FBUF2LBUF");
  // CHECK(dif_rv_timer_counter_read(&timer, kHart, &current_time) ==
  //       kDifRvTimerOk);
  // LOG_INFO("Current time: %d; timer theshold: %d", (uint32_t)current_time,
  //          (uint32_t)(current_time + kDeadline));
  // CHECK(dif_rv_timer_counter_set_enabled(&timer, kHart, kDifRvTimerEnabled) ==
  //       kDifRvTimerOk);
  
  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  
  // CHECK(dif_rv_timer_counter_set_enabled(&timer, kHart, kDifRvTimerDisabled) ==
  //       kDifRvTimerOk);
  // CHECK(dif_rv_timer_counter_read(&timer, kHart, &current_time) ==
  //       kDifRvTimerOk);
  // LOG_INFO("Current time: %d; timer theshold: %d", (uint32_t)current_time,
  //          (uint32_t)(current_time + kDeadline));

  // LOG_INFO("FCCOMP");
  // CHECK(dif_rv_timer_counter_read(&timer, kHart, &current_time) ==
  //       kDifRvTimerOk);
  // LOG_INFO("Current time: %d; timer theshold: %d", (uint32_t)current_time,
  //          (uint32_t)(current_time + kDeadline));
  // CHECK(dif_rv_timer_counter_set_enabled(&timer, kHart, kDifRvTimerEnabled) ==
  //       kDifRvTimerOk);

  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 0, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // CHECK(dif_rv_timer_counter_set_enabled(&timer, kHart, kDifRvTimerDisabled) ==
  //       kDifRvTimerOk);
  // CHECK(dif_rv_timer_counter_read(&timer, kHart, &current_time) ==
  //       kDifRvTimerOk);
  // LOG_INFO("Current time: %d; timer theshold: %d", (uint32_t)current_time,
  //          (uint32_t)(current_time + kDeadline));

  // bypass
  // LOG_INFO("PPE_COMP");
  // CHECK(dif_rv_timer_counter_read(&timer, kHart, &current_time) ==
  //       kDifRvTimerOk);
  // LOG_INFO("Current time: %d; timer theshold: %d", (uint32_t)current_time,
  //          (uint32_t)(current_time + kDeadline));
  // CHECK(dif_rv_timer_counter_set_enabled(&timer, kHart, kDifRvTimerEnabled) ==
  //       kDifRvTimerOk);

  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 328, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // CHECK(dif_rv_timer_counter_set_enabled(&timer, kHart, kDifRvTimerDisabled) ==
  //       kDifRvTimerOk);
  // CHECK(dif_rv_timer_counter_read(&timer, kHart, &current_time) ==
  //       kDifRvTimerOk);
  // LOG_INFO("Current time: %d; timer theshold: %d", (uint32_t)current_time,
  //          (uint32_t)(current_time + kDeadline));

  // Computing lstm_1_xf_gate
  // t:1-2, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 64, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 330, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xc_gate
  // t:1-2, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 128, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 332, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xo_gate
  // t:1-2, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 192, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 334, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hi_gate
  // t:1-2, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 322, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 256, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    328, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 0,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_1_hf_gate
  // t:1-2, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 322, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 260, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    330, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 2,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    326, 2, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hc_gate
  // t:1-2, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 322, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 264, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    332, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 4,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    2, 4, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_1_ho_gate
  // t:1-2, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 322, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 268, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    334, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 6,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 2, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xi_gate
  // t:1-2, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 272, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xf_gate
  // t:1-2, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 276, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 6, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xc_gate
  // t:1-2, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 280, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 8, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xo_gate
  // t:1-2, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 284, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 10, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hi_gate
  // t:1-2, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 324, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 288, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 8,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 12, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_2_hf_gate
  // t:1-2, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 324, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 292, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    6, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 10,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    320, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hc_gate
  // t:1-2, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 324, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 296, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    8, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 12,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    12, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    0, 6, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_2_ho_gate
  // t:1-2, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 324, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 300, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    10, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 14,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 8, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:1-2, oc:0-63, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 64, 1, 64,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 304, 0x00, 15, 63     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    63, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 326, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:1-2, oc:1-64, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 57, 1, 57,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 432, 0x00, 15, 56     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    56, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 326, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2
  // t:1-2, oc:0-2, ic:0-1, k:0-0
  // T:10, OC:3, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 3, 1, 3,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 546, 0x00, 15, 2     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 10, 137,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xi_gate
  // t:2-3, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 32, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 0, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xf_gate
  // t:2-3, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 32, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 64, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 13, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xc_gate
  // t:2-3, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 32, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 128, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 15, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xo_gate
  // t:2-3, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 32, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 192, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 17, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hi_gate
  // t:2-3, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 256, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 0,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 19, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_1_hf_gate
  // t:2-3, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 260, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    13, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 2,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    4, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hc_gate
  // t:2-3, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 264, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    15, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 4,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    19, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    0, 13, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_1_ho_gate
  // t:2-3, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 268, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    17, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 6,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 4, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xi_gate
  // t:2-3, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 272, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xf_gate
  // t:2-3, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 276, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 15, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xc_gate
  // t:2-3, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 280, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 17, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xo_gate
  // t:2-3, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 284, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 19, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hi_gate
  // t:2-3, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 288, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 8,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 2, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_2_hf_gate
  // t:2-3, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 292, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    15, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 10,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    6, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hc_gate
  // t:2-3, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 296, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    17, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 12,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    2, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    0, 15, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_2_ho_gate
  // t:2-3, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 300, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    19, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 14,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 2, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:2-3, oc:0-63, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 64, 1, 64,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 304, 0x00, 15, 63     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    63, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 447, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:2-3, oc:1-64, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 57, 1, 57,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 432, 0x00, 15, 56     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    56, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 447, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2
  // t:2-3, oc:0-2, ic:0-1, k:0-0
  // T:10, OC:3, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 3, 1, 3,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 546, 0x00, 15, 2     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 17, 137,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xi_gate
  // t:3-4, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 64, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 0, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xf_gate
  // t:3-4, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 64, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 64, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 6, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xc_gate
  // t:3-4, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 64, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 128, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 8, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xo_gate
  // t:3-4, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 64, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 192, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 20, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hi_gate
  // t:3-4, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 256, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 0,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 22, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_1_hf_gate
  // t:3-4, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 260, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    6, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 2,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    13, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hc_gate
  // t:3-4, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 264, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    8, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 4,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    22, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    0, 6, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_1_ho_gate
  // t:3-4, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 268, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    20, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 6,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 8, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xi_gate
  // t:3-4, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 272, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xf_gate
  // t:3-4, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 276, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 13, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xc_gate
  // t:3-4, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 280, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 20, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xo_gate
  // t:3-4, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 284, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 22, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hi_gate
  // t:3-4, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 288, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 8,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 4, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_2_hf_gate
  // t:3-4, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 292, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    13, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 10,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    15, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hc_gate
  // t:3-4, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 296, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    20, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 12,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    4, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    0, 13, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_2_ho_gate
  // t:3-4, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 300, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    22, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 14,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 4, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:3-4, oc:0-63, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 64, 1, 64,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 304, 0x00, 15, 63     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    63, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 568, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:3-4, oc:1-64, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 57, 1, 57,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 432, 0x00, 15, 56     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    56, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 568, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2
  // t:3-4, oc:0-2, ic:0-1, k:0-0
  // T:10, OC:3, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 3, 1, 3,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 546, 0x00, 15, 2     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 20, 137,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xi_gate
  // t:4-5, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 96, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 0, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xf_gate
  // t:4-5, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 96, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 64, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 2, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xc_gate
  // t:4-5, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 96, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 128, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 15, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xo_gate
  // t:4-5, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 96, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 192, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 23, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hi_gate
  // t:4-5, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 256, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 0,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 25, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_1_hf_gate
  // t:4-5, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 260, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    2, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 2,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    6, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hc_gate
  // t:4-5, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 264, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    15, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 4,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    25, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    0, 2, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_1_ho_gate
  // t:4-5, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 268, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    23, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 6,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 6, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xi_gate
  // t:4-5, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 272, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xf_gate
  // t:4-5, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 276, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 15, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xc_gate
  // t:4-5, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 280, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 23, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xo_gate
  // t:4-5, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 284, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 25, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hi_gate
  // t:4-5, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 288, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 8,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 8, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_2_hf_gate
  // t:4-5, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 292, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    15, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 10,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    13, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hc_gate
  // t:4-5, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 296, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    23, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 12,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    8, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    0, 15, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_2_ho_gate
  // t:4-5, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 300, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    25, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 14,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 8, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:4-5, oc:0-63, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 64, 1, 64,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 304, 0x00, 15, 63     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    63, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 689, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:4-5, oc:1-64, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 57, 1, 57,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 432, 0x00, 15, 56     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    56, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 689, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2
  // t:4-5, oc:0-2, ic:0-1, k:0-0
  // T:10, OC:3, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 3, 1, 3,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 546, 0x00, 15, 2     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 23, 137,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xi_gate
  // t:5-6, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 128, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 0, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xf_gate
  // t:5-6, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 128, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 64, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 4, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xc_gate
  // t:5-6, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 128, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 128, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 13, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xo_gate
  // t:5-6, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 128, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 192, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 26, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hi_gate
  // t:5-6, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 256, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 0,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 28, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_1_hf_gate
  // t:5-6, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 260, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    4, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 2,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    2, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hc_gate
  // t:5-6, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 264, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    13, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 4,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    28, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    0, 4, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_1_ho_gate
  // t:5-6, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 268, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    26, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 6,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 2, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xi_gate
  // t:5-6, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 272, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xf_gate
  // t:5-6, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 276, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 13, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xc_gate
  // t:5-6, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 280, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 26, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xo_gate
  // t:5-6, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 284, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 28, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hi_gate
  // t:5-6, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 288, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 8,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 6, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_2_hf_gate
  // t:5-6, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 292, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    13, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 10,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    15, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hc_gate
  // t:5-6, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 296, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    26, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 12,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    6, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    0, 13, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_2_ho_gate
  // t:5-6, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 300, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    28, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 14,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 6, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:5-6, oc:0-63, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 64, 1, 64,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 304, 0x00, 15, 63     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    63, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 26, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:5-6, oc:1-64, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 57, 1, 57,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 432, 0x00, 15, 56     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    56, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 26, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2
  // t:5-6, oc:0-2, ic:0-1, k:0-0
  // T:10, OC:3, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 3, 1, 3,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 546, 0x00, 15, 2     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 147, 137,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xi_gate
  // t:6-7, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 160, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 0, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xf_gate
  // t:6-7, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 160, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 64, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 8, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xc_gate
  // t:6-7, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 160, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 128, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 15, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xo_gate
  // t:6-7, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 160, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 192, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 150, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hi_gate
  // t:6-7, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 256, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 0,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 152, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_1_hf_gate
  // t:6-7, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 260, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    8, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 2,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    4, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hc_gate
  // t:6-7, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 264, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    15, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 4,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    152, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    0, 8, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_1_ho_gate
  // t:6-7, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 268, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    150, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 6,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 4, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xi_gate
  // t:6-7, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 272, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xf_gate
  // t:6-7, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 276, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 15, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xc_gate
  // t:6-7, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 280, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 150, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xo_gate
  // t:6-7, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 284, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 152, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hi_gate
  // t:6-7, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 288, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 8,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 2, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_2_hf_gate
  // t:6-7, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 292, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    15, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 10,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    13, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hc_gate
  // t:6-7, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 296, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    150, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 12,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    2, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    0, 15, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_2_ho_gate
  // t:6-7, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 300, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    152, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 14,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 2, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:6-7, oc:0-63, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 64, 1, 64,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 304, 0x00, 15, 63     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    63, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 810, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:6-7, oc:1-64, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 57, 1, 57,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 432, 0x00, 15, 56     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    56, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 810, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2
  // t:6-7, oc:0-2, ic:0-1, k:0-0
  // T:10, OC:3, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 3, 1, 3,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 546, 0x00, 15, 2     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 150, 137,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xi_gate
  // t:7-8, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 192, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 0, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xf_gate
  // t:7-8, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 192, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 64, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 6, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xc_gate
  // t:7-8, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 192, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 128, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 13, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xo_gate
  // t:7-8, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 192, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 192, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 153, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hi_gate
  // t:7-8, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 256, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 0,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 155, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_1_hf_gate
  // t:7-8, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 260, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    6, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 2,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    8, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hc_gate
  // t:7-8, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 264, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    13, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 4,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    155, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    0, 6, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_1_ho_gate
  // t:7-8, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 268, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    153, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 6,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 8, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xi_gate
  // t:7-8, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 272, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xf_gate
  // t:7-8, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 276, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 13, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xc_gate
  // t:7-8, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 280, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 153, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xo_gate
  // t:7-8, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 284, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 155, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hi_gate
  // t:7-8, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 288, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 8,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 4, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_2_hf_gate
  // t:7-8, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 292, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    13, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 10,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    15, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hc_gate
  // t:7-8, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 296, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    153, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 12,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    4, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    0, 13, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_2_ho_gate
  // t:7-8, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 300, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    155, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 14,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 4, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:7-8, oc:0-63, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 64, 1, 64,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 304, 0x00, 15, 63     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    63, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 931, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:7-8, oc:1-64, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 57, 1, 57,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 432, 0x00, 15, 56     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    56, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 931, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2
  // t:7-8, oc:0-2, ic:0-1, k:0-0
  // T:10, OC:3, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 3, 1, 3,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 546, 0x00, 15, 2     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 153, 137,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xi_gate
  // t:8-9, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 224, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 0, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xf_gate
  // t:8-9, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 224, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 64, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 2, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xc_gate
  // t:8-9, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 224, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 128, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 15, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xo_gate
  // t:8-9, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 224, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 192, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 156, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hi_gate
  // t:8-9, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 256, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 0,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 158, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_1_hf_gate
  // t:8-9, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 260, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    2, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 2,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    6, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hc_gate
  // t:8-9, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 264, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    15, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 4,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    158, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    0, 2, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_1_ho_gate
  // t:8-9, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 268, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    156, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 6,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 6, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xi_gate
  // t:8-9, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 272, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xf_gate
  // t:8-9, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 276, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 15, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xc_gate
  // t:8-9, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 280, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 156, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xo_gate
  // t:8-9, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 284, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 158, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hi_gate
  // t:8-9, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 288, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 8,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 8, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_2_hf_gate
  // t:8-9, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 292, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    15, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 10,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    13, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hc_gate
  // t:8-9, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 296, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    156, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 12,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    8, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    0, 15, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_2_ho_gate
  // t:8-9, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 300, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    158, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 14,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 8, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:8-9, oc:0-63, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 64, 1, 64,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 304, 0x00, 15, 63     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    63, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 1052, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:8-9, oc:1-64, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 57, 1, 57,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 432, 0x00, 15, 56     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    56, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 1052, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2
  // t:8-9, oc:0-2, ic:0-1, k:0-0
  // T:10, OC:3, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 3, 1, 3,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 546, 0x00, 15, 2     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 156, 137,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xi_gate
  // t:9-10, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 256, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 0, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xf_gate
  // t:9-10, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 256, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 64, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 4, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xc_gate
  // t:9-10, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 256, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 128, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 13, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xo_gate
  // t:9-10, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 256, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 192, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 159, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hi_gate
  // t:9-10, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 256, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 0,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 161, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_1_hf_gate
  // t:9-10, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 260, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    4, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 2,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    2, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hc_gate
  // t:9-10, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 264, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    13, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 4,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    161, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    0, 4, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_1_ho_gate
  // t:9-10, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 268, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    159, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 6,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 2, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xi_gate
  // t:9-10, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 272, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xf_gate
  // t:9-10, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 276, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 13, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xc_gate
  // t:9-10, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 280, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 159, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xo_gate
  // t:9-10, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 284, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 161, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hi_gate
  // t:9-10, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 288, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 8,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 6, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_2_hf_gate
  // t:9-10, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 292, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    13, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 10,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    15, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hc_gate
  // t:9-10, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 296, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    159, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 12,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    6, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    0, 13, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_2_ho_gate
  // t:9-10, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 8, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 300, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    161, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 14,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 6, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:9-10, oc:0-63, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 64, 1, 64,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 304, 0x00, 15, 63     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    63, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 159, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:9-10, oc:1-64, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 57, 1, 57,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 432, 0x00, 15, 56     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    56, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 159, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2
  // t:9-10, oc:0-2, ic:0-1, k:0-0
  // T:10, OC:3, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 3, 1, 3,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 546, 0x00, 15, 2     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 280, 137,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xi_gate
  // t:10-11, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 288, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 0, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xf_gate
  // t:10-11, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 288, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 64, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 8, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xc_gate
  // t:10-11, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 288, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 128, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 15, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_xo_gate
  // t:10-11, oc:0-1, ic:0-31, k:0-0
  // T:10, OC:2, IC:32, K:1
  dif_dla_move_fbuf2lbuf(&dla, 288, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 31, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 32, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 192, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 283, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hi_gate
  // t:10-11, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 256, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 0,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 285, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_1_hf_gate
  // t:10-11, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 260, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    8, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 2,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    4, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_1_hc_gate
  // t:10-11, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 264, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    15, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 4,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    285, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    0, 8, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_1_ho_gate
  // t:10-11, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 268, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    283, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 6,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 4, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xi_gate
  // t:10-11, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 272, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xf_gate
  // t:10-11, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 276, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 15, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xc_gate
  // t:10-11, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 280, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 283, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_xo_gate
  // t:10-11, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 4, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 284, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bypass
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 0, 1, 15,  // baseaddr, act, elem, bias, pass, row
    0, 2, 1, 0, 0,               // len, iter, post, mode, oper
    11111, 285, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 2, 11111, 1,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hi_gate
  // t:10-11, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 288, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    0, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 8,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 2, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // Computing lstm_2_hf_gate
  // t:10-11, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 292, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    15, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 10,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    13, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing lstm_2_hc_gate
  // t:10-11, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    14, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 296, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    283, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 12,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 1,               // len, iter, post, mode, oper
    2, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 2, 0, 2,               // len, iter, post, mode, oper
    0, 15, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // tanh
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 3,               // len, iter, post, mode, oper
    11111, 0, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_tanh, act_b_tanh, act_x_tanh
  );

  // Computing lstm_2_ho_gate
  // t:10-11, oc:0-1, ic:0-1, k:0-0
  // T:10, OC:2, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 6, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 2, 1, 2,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 300, 0x00, 15, 1     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // add
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    285, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 2,               // len, iter, post, mode, oper
    11111, 11111, 14,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // sigmoid
  dif_dla_ppe_comp_full (
    &dla, 1, 0, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 0, 0, 3,               // len, iter, post, mode, oper
    11111, 11111, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_sigmoid, act_b_sigmoid, act_x_sigmoid
  );

  // mul
  dif_dla_ppe_comp_full (
    &dla, 0, 1, 0, 0, 15,  // baseaddr, act, elem, bias, pass, row
    1, 11111, 1, 0, 1,               // len, iter, post, mode, oper
    0, 2, 11111,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:10-11, oc:0-63, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 64, 1, 64,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 304, 0x00, 15, 63     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    63, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 1173, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul1
  // t:10-11, oc:1-64, ic:0-1, k:0-0
  // T:10, OC:121, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    16, 2, 57, 1, 57,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 432, 0x00, 15, 56     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    56, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 1173, 16,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

  // Computing mat_mul2
  // t:10-11, oc:0-2, ic:0-1, k:0-0
  // T:10, OC:3, IC:2, K:1
  dif_dla_move_fbuf2lbuf(&dla, 2, 0,  // baseaddr, src_addr, dest_addr
           0, 0, 1, 1, 0);     // skip, iter, len, dila, mode
  dif_dla_fc_comp(
    &dla, 0,                   // baseaddr, mode_spar
    15, 2, 3, 1, 3,        // k_scale, if_chl, of_chl, sub_col, sub_row
    0, 546, 0x00, 15, 2     // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
  );

  // bias
  dif_dla_ppe_comp_full (
    &dla, 0, 0, 1, 0, 15,  // baseaddr, act, elem, bias, pass, row
    2, 11111, 1, 0, 2,               // len, iter, post, mode, oper
    11111, 283, 137,                 // fbuf_src, fbuf_dest, abuf_src
    11111, 11111, 11111, 11111,         // src_dila, dest_dila, src_skip, dest_skip
    act_k_dummy, act_b_dummy, act_x_dummy
  );

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

void lstm_wrapper(void){
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
  lstm();

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
  LOG_INFO("Hello LSTM!");
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

  lstm_wrapper();
  
  uint32_t gpio_state = 0;
  while (true) {
    usleep(10 * 1000);  // 10 ms
    gpio_state = demo_gpio_to_log_echo(&gpio, gpio_state);
    demo_spi_to_log_echo(&spi);
    demo_uart_to_uart_and_gpio_echo(&uart, &gpio);
  }
}
