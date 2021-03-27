// Copyright lowRISC contributors.
// Licensed under the Apache License; Version 2.0; see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0


package ddr_ctrl_pkg;


  typedef struct packed {
    logic [14:0]        ddr3_addr;
    logic [2:0]         ddr3_ba;
    logic               ddr3_ras_n;
    logic               ddr3_cas_n;
    logic               ddr3_we_n;
    logic               ddr3_reset_n;
    logic [0:0]         ddr3_ck_p;
    logic [0:0]         ddr3_ck_n;
    logic [0:0]         ddr3_cke;
    logic [0:0]         ddr3_cs_n;
    logic [3:0]         ddr3_dm;
    logic [0:0]         ddr3_odt;
  } mig_pins_out_t;
  
  typedef struct packed {
    logic [31:0]        ddr3_dq;
    logic [3:0]         ddr3_dqs_n;
    logic [3:0]         ddr3_dqs_p;
  } mig_pins_inout_t;

  
endpackage // ddr_ctrl_pkg
