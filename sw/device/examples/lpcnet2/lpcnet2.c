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
//python3 util/simplespi/spitest.py -i lpcnet2_ddr_in.txt

//python script to receive same ddr contents and send to spi
//python3 util/simplespi/spitest.py -r 196608


//used for ddr_spi functions
//DDRIN_LINES needs to be divisible by 128,
//or DDRIN_SIZE needs to be divisible by 1024,
//so SPI device can send integer patches of 1024B
const uint32_t DDRIN_LINES = 150016; //lpcnet2_ddr_in.txt
const uint32_t DDRIN_SIZE = DDRIN_LINES * 8 ; //bytes

//used for reading wavenet result
//ddr read start address
const uint32_t RD_DDR_ADDR = 160000; //64-bit addressable in unsigned decimal
//ddr read bytes
const uint32_t RD_DDR_BYTES = 65536; //bytes


void lpcnet2(void) {
  dif_dla_pbuf_rst(&dla, 64);

    // meu
    // gpio[0] = 9;
    dif_dla_move_ddr2gb(&dla, 240, 142768, 0,   // baseaddr, len, ddr_addr, gb_addr
                0, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 240, 143008, 0,   // baseaddr, len, ddr_addr, gb_addr
                1, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 240, 143248, 0,   // baseaddr, len, ddr_addr, gb_addr
                2, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 240, 143488, 0,   // baseaddr, len, ddr_addr, gb_addr
                3, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 12, 139824, 240,   // baseaddr, len, ddr_addr, gb_addr
                0, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 12, 139836, 240,   // baseaddr, len, ddr_addr, gb_addr
                1, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 12, 139848, 240,   // baseaddr, len, ddr_addr, gb_addr
                2, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 12, 139860, 240,   // baseaddr, len, ddr_addr, gb_addr
                3, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 240, 135968, 250,   // baseaddr, len, ddr_addr, gb_addr
                0, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 240, 136208, 250,   // baseaddr, len, ddr_addr, gb_addr
                1, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 240, 136448, 250,   // baseaddr, len, ddr_addr, gb_addr
                2, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 240, 136688, 250,   // baseaddr, len, ddr_addr, gb_addr
                3, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 80, 133716, 490,   // baseaddr, len, ddr_addr, gb_addr
                0, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 80, 133796, 490,   // baseaddr, len, ddr_addr, gb_addr
                1, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 80, 133876, 490,   // baseaddr, len, ddr_addr, gb_addr
                2, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 80, 133956, 490,   // baseaddr, len, ddr_addr, gb_addr
                3, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 240, 139876, 570,   // baseaddr, len, ddr_addr, gb_addr
                0, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 240, 140116, 570,   // baseaddr, len, ddr_addr, gb_addr
                1, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 240, 140356, 570,   // baseaddr, len, ddr_addr, gb_addr
                2, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 240, 140596, 570,   // baseaddr, len, ddr_addr, gb_addr
                3, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 12, 143732, 810,   // baseaddr, len, ddr_addr, gb_addr
                0, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 12, 143742, 810,   // baseaddr, len, ddr_addr, gb_addr
                1, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 12, 143752, 810,   // baseaddr, len, ddr_addr, gb_addr
                2, 0, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 12, 143762, 810,   // baseaddr, len, ddr_addr, gb_addr
                3, 0, 0);                      // gb_idx, gb_mux, direction

    dif_dla_move_ddr2gb(&dla, 700, 111068, 0,   // baseaddr, len, ddr_addr, gb_addr
                0, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 111767, 0,   // baseaddr, len, ddr_addr, gb_addr
                1, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 112466, 0,   // baseaddr, len, ddr_addr, gb_addr
                2, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 113165, 0,   // baseaddr, len, ddr_addr, gb_addr
                3, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 113864, 0,   // baseaddr, len, ddr_addr, gb_addr
                4, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 114563, 0,   // baseaddr, len, ddr_addr, gb_addr
                5, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 115262, 0,   // baseaddr, len, ddr_addr, gb_addr
                6, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 115961, 0,   // baseaddr, len, ddr_addr, gb_addr
                7, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 116660, 0,   // baseaddr, len, ddr_addr, gb_addr
                8, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 117359, 0,   // baseaddr, len, ddr_addr, gb_addr
                9, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 118058, 0,   // baseaddr, len, ddr_addr, gb_addr
                10, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 118757, 0,   // baseaddr, len, ddr_addr, gb_addr
                11, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 119456, 0,   // baseaddr, len, ddr_addr, gb_addr
                12, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 120155, 0,   // baseaddr, len, ddr_addr, gb_addr
                13, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 120854, 0,   // baseaddr, len, ddr_addr, gb_addr
                14, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 121553, 0,   // baseaddr, len, ddr_addr, gb_addr
                15, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 122252, 0,   // baseaddr, len, ddr_addr, gb_addr
                16, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 122951, 0,   // baseaddr, len, ddr_addr, gb_addr
                17, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 123650, 0,   // baseaddr, len, ddr_addr, gb_addr
                18, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 124349, 0,   // baseaddr, len, ddr_addr, gb_addr
                19, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 125048, 0,   // baseaddr, len, ddr_addr, gb_addr
                20, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 125747, 0,   // baseaddr, len, ddr_addr, gb_addr
                21, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 126446, 0,   // baseaddr, len, ddr_addr, gb_addr
                22, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 127145, 0,   // baseaddr, len, ddr_addr, gb_addr
                23, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 127844, 0,   // baseaddr, len, ddr_addr, gb_addr
                24, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 128543, 0,   // baseaddr, len, ddr_addr, gb_addr
                25, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 129242, 0,   // baseaddr, len, ddr_addr, gb_addr
                26, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 129941, 0,   // baseaddr, len, ddr_addr, gb_addr
                27, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 130640, 0,   // baseaddr, len, ddr_addr, gb_addr
                28, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 131339, 0,   // baseaddr, len, ddr_addr, gb_addr
                29, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 132038, 0,   // baseaddr, len, ddr_addr, gb_addr
                30, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 700, 132737, 0,   // baseaddr, len, ddr_addr, gb_addr
                31, 1, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 60, 133472, 0,   // baseaddr, len, ddr_addr, gb_addr
                0, 3, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 60, 133531, 0,   // baseaddr, len, ddr_addr, gb_addr
                1, 3, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 60, 133590, 0,   // baseaddr, len, ddr_addr, gb_addr
                2, 3, 0);                      // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 60, 133649, 0,   // baseaddr, len, ddr_addr, gb_addr
                3, 3, 0);                      // gb_idx, gb_mux, direction

//    dif_dla_move_ddr2gb(&dla, 2048, 150000, 0,   // baseaddr, len, ddr_addr, gb_addr
//        0, 0, 0);            // gb_idx, gb_mux, direction
//    dif_dla_move_ddr2gb(&dla, 2048, 152048, 0,   // baseaddr, len, ddr_addr, gb_addr
//        1, 0, 0);            // gb_idx, gb_mux, direction
//    dif_dla_move_ddr2gb(&dla, 2048, 154096, 0,   // baseaddr, len, ddr_addr, gb_addr
//        2, 0, 0);            // gb_idx, gb_mux, direction
//    dif_dla_move_ddr2gb(&dla, 2048, 156144, 0,   // baseaddr, len, ddr_addr, gb_addr
//        3, 0, 0);            // gb_idx, gb_mux, direction


    // // gpio[0] = 0;
    // // Computing gru_1_hz_gate_0_0
  // // t:0-0, oc:0-23, ic:0-23, k:0-0
  // // t_meu:10, OC:24, IC:24, K:1
    // dif_dla_move_fbuf2lbuf(&dla, 786, 0,  // baseaddr, src_addr, dest_addr
    //                1, 23, 0, 24, 0);       // skip, iter, len, dila, mode
    // conv_comp(
    //     &dla, 0,                                   // baseaddr, mode_spar
    //     1, 12, 0, 0, 24, 24,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    //     0, 0, 0, 1, 24,         // pad_left, pad_right, pad_num, sub_col, sub_row
    //     0, 0, 0x00, 15, 23       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
    // );

    // // gpio[0] = 3;
    // // add
    // dif_dla_ppe_comp_full (
    //     &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //     23, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    //     250, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //     1, 1, 24, 24,                 // src_dila, dest_dila, src_skip, dest_skip
    //     act_k_dummy, act_b_dummy, act_x_dummy
    // );

    // // gpio[0] = 2;
    // // bias
    // dif_dla_ppe_comp_full (
    //     &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //     23, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    //     11111, 820, 0,                               // fbuf_src, fbuf_dest, abuf_src
    //     11111, 1, 11111, 24,                 // src_dila, dest_dila, src_skip, dest_skip
    //     act_k_dummy, act_b_dummy, act_x_dummy
    // );

    // //  gpio[0] = 1;
    //  // Computing gru_1_z_act_0_0
  // // oc_idx:0
  // // t_meu:10, OC:24, IC:24, K:1
    //  // rst
  // dif_dla_pbuf_rst(&dla, 64);
    //  // add
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      23, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    //      820, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    //  // sigmoid_neg
    //  dif_dla_ppe_comp_full (
    //      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      23, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    //      11111, 250, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
    //  );

    //  // mul
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      23, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    //      0, 820, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    // //  gpio[0] = 2;
    //  // Computing gru_1_tmp2_0_0
  // // oc_idx:0
  // // t_meu:10, OC:24, IC:24, K:1
    //  // rst
  // dif_dla_pbuf_rst(&dla, 64);
    //  // add
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      23, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    //      250, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    //  // mul
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      23, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    //      786, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    //  // add
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      23, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    //      820, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    //  // add
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      23, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    //      786, 0, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    //  gpio[0] = 3;
    //  // Computing gru_2_xz_gate_0_1
  // // t:0-0, oc:0-0, ic:0-23, k:0-0
  // // t_meu:10, OC:1, IC:24, K:1
    //  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
    //                 1, 23, 0, 24, 0);       // skip, iter, len, dila, mode
    //  conv_comp(
    //      &dla, 0,                                   // baseaddr, mode_spar
    //      1, 11, 0, 0, 24, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    //      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row
    //      0, 576, 0x00, 15, 0       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
    //  );
    //  // add
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    //      240, 250, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      1, 1, 1, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    // //  gpio[0] = 4;
    //  // Computing gru_2_xr_gate_0_0
  // // t:0-0, oc:0-0, ic:0-7, k:0-0
  // // t_meu:10, OC:1, IC:8, K:1
    //  dif_dla_move_fbuf2lbuf(&dla, 490, 0,  // baseaddr, src_addr, dest_addr
    //                 1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
    //  conv_comp(
    //      &dla, 0,                                   // baseaddr, mode_spar
    //      1, 11, 0, 0, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    //      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row
    //      0, 600, 0x00, 15, 0       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
    //  );
    //  // bypass
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    //      11111, 240, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    // //  gpio[0] = 5;
    //  // Computing gru_2_xr_gate_0_1
  // // t:0-0, oc:0-0, ic:0-23, k:0-0
  // // t_meu:10, OC:1, IC:24, K:1
    //  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
    //                 1, 23, 0, 24, 0);       // skip, iter, len, dila, mode
    //  conv_comp(
    //      &dla, 0,                                   // baseaddr, mode_spar
    //      1, 11, 0, 0, 24, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    //      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row
    //      0, 608, 0x00, 15, 0       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
    //  );
    //  // add
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    //      240, 251, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      1, 1, 1, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    // //  gpio[0] = 6;
    //  // Computing gru_2_xh_gate_0_0
  // // t:0-0, oc:0-0, ic:0-7, k:0-0
  // // t_meu:10, OC:1, IC:8, K:1
    //  dif_dla_move_fbuf2lbuf(&dla, 490, 0,  // baseaddr, src_addr, dest_addr
    //                 1, 7, 0, 8, 0);       // skip, iter, len, dila, mode
    //  conv_comp(
    //      &dla, 0,                                   // baseaddr, mode_spar
    //      1, 11, 0, 0, 8, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    //      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row
    //      0, 632, 0x00, 15, 0       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
    //  );
    //  // bypass
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 0, 0, 1, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 1, 1, 0, 0,                           // len, iter, post, mode, oper
    //      11111, 240, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    // //  gpio[0] = 7;
    //  // Computing gru_2_xh_gate_0_1
  // // t:0-0, oc:0-0, ic:0-23, k:0-0
  // // t_meu:10, OC:1, IC:24, K:1
    //  dif_dla_move_fbuf2lbuf(&dla, 0, 0,  // baseaddr, src_addr, dest_addr
    //                 1, 23, 0, 24, 0);       // skip, iter, len, dila, mode
    //  conv_comp(
    //      &dla, 0,                                   // baseaddr, mode_spar
    //      1, 11, 0, 0, 24, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    //      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row
    //      0, 640, 0x00, 15, 0       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
    //  );
    //  // add
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    //      240, 252, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      1, 1, 1, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    // //  gpio[0] = 8;
    //  // Computing gru_2_hz_gate_0_0
  // // t:0-0, oc:0-0, ic:0-0, k:0-0
  // // t_meu:10, OC:1, IC:1, K:1
    //  dif_dla_move_fbuf2lbuf(&dla, 819, 0,  // baseaddr, src_addr, dest_addr
    //                 1, 0, 0, 1, 0);       // skip, iter, len, dila, mode
    //  conv_comp(
    //      &dla, 0,                                   // baseaddr, mode_spar
    //      1, 14, 0, 0, 1, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    //      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row
    //      0, 664, 0x00, 15, 0       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
    //  );
    //  // add
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    //      250, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      1, 1, 1, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    //  // bias
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    //      11111, 240, 24,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    // //  gpio[0] = 9;
    //  // Computing gru_2_hr_gate_0_0
  // // t:0-0, oc:0-0, ic:0-0, k:0-0
  // // t_meu:10, OC:1, IC:1, K:1
    //  dif_dla_move_fbuf2lbuf(&dla, 819, 0,  // baseaddr, src_addr, dest_addr
    //                 1, 0, 0, 1, 0);       // skip, iter, len, dila, mode
    //  conv_comp(
    //      &dla, 0,                                   // baseaddr, mode_spar
    //      1, 13, 0, 0, 1, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    //      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row
    //      0, 665, 0x00, 15, 0       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
    //  );
    //  // add
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    //      251, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      1, 1, 1, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    //  // bias
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    //      11111, 11111, 25,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    //  // sigmoid
    //  dif_dla_ppe_comp_full (
    //      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    //      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_sigmoid_pos, act_b_sigmoid_pos, act_x_sigmoid_pos
    //  );

    //  // mul
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 1, 1, 0, 1,                           // len, iter, post, mode, oper
    //      819, 250, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      1, 1, 1, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    // //  gpio[0] = 10;
    //  // Computing gru_2_z_act_0_0
  // // oc_idx:0
  // // t_meu:10, OC:1, IC:1, K:1
    //  // rst
  // dif_dla_pbuf_rst(&dla, 64);
    //  // add
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    //      240, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    //  // sigmoid_neg
    //  dif_dla_ppe_comp_full (
    //      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 11111, 2, 0, 3,                           // len, iter, post, mode, oper
    //      11111, 251, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_sigmoid_neg, act_b_sigmoid_neg, act_x_sigmoid_neg
    //  );

    //  // mul
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 11111, 1, 0, 1,                           // len, iter, post, mode, oper
    //      819, 240, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    // //  gpio[0] = 11;
    //  // Computing gru_2_hh_gate_0_0
  // // t:0-0, oc:0-0, ic:0-0, k:0-0
  // // t_meu:10, OC:1, IC:1, K:1
    //  dif_dla_move_fbuf2lbuf(&dla, 250, 0,  // baseaddr, src_addr, dest_addr
    //                 1, 0, 0, 1, 0);       // skip, iter, len, dila, mode
    //  conv_comp(
    //      &dla, 0,                                   // baseaddr, mode_spar
    //      1, 14, 0, 0, 1, 1,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    //      0, 0, 0, 1, 1,         // pad_left, pad_right, pad_num, sub_col, sub_row
    //      0, 666, 0x00, 15, 0       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
    //  );
    //  // add
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    //      252, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      1, 1, 1, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    //  // bias
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 1, 1, 0, 2,                           // len, iter, post, mode, oper
    //      11111, 253, 26,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 1, 11111, 1,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    //  // Computing gru_2_h_act_0_0
  // // oc_idx:0
  // // t_meu:10, OC:1, IC:1, K:1
    //  // rst
  // dif_dla_pbuf_rst(&dla, 64);
    //  // add
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    //      253, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    //  // tanh_neg
    //  dif_dla_ppe_comp_full (
    //      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 11111, 0, 0, 3,                           // len, iter, post, mode, oper
    //      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_tanh_neg, act_b_tanh_neg, act_x_tanh_neg
    //  );

    //  // mul
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 11111, 0, 0, 1,                           // len, iter, post, mode, oper
    //      251, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    //  // add
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 11111, 0, 0, 2,                           // len, iter, post, mode, oper
    //      240, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    //  // add
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      0, 11111, 1, 0, 2,                           // len, iter, post, mode, oper
    //      819, 250, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 11111, 11111, 11111,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    //  // Computing fc_3_0_0
  // // t:0-0, oc:0-15, ic:0-0, k:0-0
  // // t_meu:10, OC:16, IC:1, K:1
    //  dif_dla_move_fbuf2lbuf(&dla, 250, 0,  // baseaddr, src_addr, dest_addr
    //                 1, 0, 0, 1, 0);       // skip, iter, len, dila, mode
    //  conv_comp(
    //      &dla, 0,                                   // baseaddr, mode_spar
    //      1, 10, 0, 0, 1, 16,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    //      0, 0, 0, 1, 16,         // pad_left, pad_right, pad_num, sub_col, sub_row
    //      0, 667, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
    //  );
    //  // bias
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      15, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    //      11111, 11111, 27,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    //  // tanh
    //  dif_dla_ppe_comp_full (
    //      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      15, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    //      11111, 251, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
    //  );

    //  // Computing fc_4_0_0
  // // t:0-0, oc:0-15, ic:0-0, k:0-0
  // // t_meu:10, OC:16, IC:1, K:1
    //  dif_dla_move_fbuf2lbuf(&dla, 250, 0,  // baseaddr, src_addr, dest_addr
    //                 1, 0, 0, 1, 0);       // skip, iter, len, dila, mode
    //  conv_comp(
    //      &dla, 0,                                   // baseaddr, mode_spar
    //      1, 10, 0, 0, 1, 16,     // k_size, k_scale, if_len, of_len, if_chl, of_chl
    //      0, 0, 0, 1, 16,         // pad_left, pad_right, pad_num, sub_col, sub_row
    //      0, 683, 0x00, 15, 15       // lbuf_addr, wbuf_addr, ibuf_addr, row_num, acc_len
    //  );
    //  // bias
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 0, 1, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      15, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    //      11111, 11111, 43,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    //  // tanh
    //  dif_dla_ppe_comp_full (
    //      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      15, 1, 0, 0, 3,                           // len, iter, post, mode, oper
    //      11111, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
    //  );

    //  // add
    //  dif_dla_ppe_comp_full (
    //      &dla, 0, 1, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      15, 1, 0, 0, 2,                           // len, iter, post, mode, oper
    //      251, 11111, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      1, 1, 16, 16,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_dummy, act_b_dummy, act_x_dummy
    //  );

    //  // tanh
    //  dif_dla_ppe_comp_full (
    //      &dla, 1, 0, 0, 0, 15,    // baseaddr, act, elem, bias, pass, row
    //      15, 1, 1, 0, 3,                           // len, iter, post, mode, oper
    //      11111, 820, 11111,                               // fbuf_src, fbuf_dest, abuf_src
    //      11111, 1, 11111, 16,                 // src_dila, dest_dila, src_skip, dest_skip
    //      act_k_tanh_pos, act_b_tanh_pos, act_x_tanh_pos
    //  );

    dif_dla_move_ddr2gb(&dla, 2048, 160000, 0,   // baseaddr, len, ddr_addr, gb_addr
        0, 0, 1);            // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 2048, 162048, 0,   // baseaddr, len, ddr_addr, gb_addr
        1, 0, 1);            // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 2048, 164096, 0,   // baseaddr, len, ddr_addr, gb_addr
        2, 0, 1);            // gb_idx, gb_mux, direction
    dif_dla_move_ddr2gb(&dla, 2048, 166144, 0,   // baseaddr, len, ddr_addr, gb_addr
        3, 0, 1);            // gb_idx, gb_mux, direction

    // gpio[0] = 0xf;
}




void lpcnet2_wrapper(void){
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
  lpcnet2();

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
  LOG_INFO("Hello lpcnet2!");
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

  lpcnet2_wrapper();
  
  uint32_t gpio_state = 0;
  while (true) {
    usleep(10 * 1000);  // 10 ms
    gpio_state = demo_gpio_to_log_echo(&gpio, gpio_state);
    demo_spi_to_log_echo(&spi);
    demo_uart_to_uart_and_gpio_echo(&uart, &gpio);
  }
}
