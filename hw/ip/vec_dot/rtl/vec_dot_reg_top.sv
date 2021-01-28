// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Register Top module auto-generated by `reggen`

`include "prim_assert.sv"

module vec_dot_reg_top (
  input clk_i,
  input rst_ni,

  // Below Regster interface can be changed
  input  tlul_pkg::tl_h2d_t tl_i,
  output tlul_pkg::tl_d2h_t tl_o,
  // To HW
  output vec_dot_reg_pkg::vec_dot_reg2hw_t reg2hw, // Write
  input  vec_dot_reg_pkg::vec_dot_hw2reg_t hw2reg, // Read

  // Config
  input devmode_i // If 1, explicit error return for unmapped register access
);

  import vec_dot_reg_pkg::* ;

  localparam int AW = 5;
  localparam int DW = 32;
  localparam int DBW = DW/8;                    // Byte Width

  // register signals
  logic           reg_we;
  logic           reg_re;
  logic [AW-1:0]  reg_addr;
  logic [DW-1:0]  reg_wdata;
  logic [DBW-1:0] reg_be;
  logic [DW-1:0]  reg_rdata;
  logic           reg_error;

  logic          addrmiss, wr_err;

  logic [DW-1:0] reg_rdata_next;

  tlul_pkg::tl_h2d_t tl_reg_h2d;
  tlul_pkg::tl_d2h_t tl_reg_d2h;

  assign tl_reg_h2d = tl_i;
  assign tl_o       = tl_reg_d2h;

  tlul_adapter_reg #(
    .RegAw(AW),
    .RegDw(DW)
  ) u_reg_if (
    .clk_i,
    .rst_ni,

    .tl_i (tl_reg_h2d),
    .tl_o (tl_reg_d2h),

    .we_o    (reg_we),
    .re_o    (reg_re),
    .addr_o  (reg_addr),
    .wdata_o (reg_wdata),
    .be_o    (reg_be),
    .rdata_i (reg_rdata),
    .error_i (reg_error)
  );

  assign reg_rdata = reg_rdata_next ;
  assign reg_error = (devmode_i & addrmiss) | wr_err ;

  // Define SW related signals
  // Format: <reg>_<field>_{wd|we|qs}
  //        or <reg>_{wd|we|qs} if field == 1 or 0
  logic intr_state_qs;
  logic intr_state_wd;
  logic intr_state_we;
  logic intr_enable_qs;
  logic intr_enable_wd;
  logic intr_enable_we;
  logic intr_test_wd;
  logic intr_test_we;
  logic cmd_start_wd;
  logic cmd_start_we;
  logic cmd_dummy_wd;
  logic cmd_dummy_we;
  logic status_busy_qs;
  logic status_busy_re;
  logic status_dummy_qs;
  logic status_dummy_re;
  logic [31:0] dotp_result_qs;
  logic dotp_result_re;
  logic [31:0] wdata_wd;
  logic wdata_we;

  // Register instances
  // R[intr_state]: V(False)

  prim_subreg #(
    .DW      (1),
    .SWACCESS("W1C"),
    .RESVAL  (1'h0)
  ) u_intr_state (
    .clk_i   (clk_i    ),
    .rst_ni  (rst_ni  ),

    // from register interface
    .we     (intr_state_we),
    .wd     (intr_state_wd),

    // from internal hardware
    .de     (hw2reg.intr_state.de),
    .d      (hw2reg.intr_state.d ),

    // to internal hardware
    .qe     (),
    .q      (reg2hw.intr_state.q ),

    // to register interface (read)
    .qs     (intr_state_qs)
  );


  // R[intr_enable]: V(False)

  prim_subreg #(
    .DW      (1),
    .SWACCESS("RW"),
    .RESVAL  (1'h0)
  ) u_intr_enable (
    .clk_i   (clk_i    ),
    .rst_ni  (rst_ni  ),

    // from register interface
    .we     (intr_enable_we),
    .wd     (intr_enable_wd),

    // from internal hardware
    .de     (1'b0),
    .d      ('0  ),

    // to internal hardware
    .qe     (),
    .q      (reg2hw.intr_enable.q ),

    // to register interface (read)
    .qs     (intr_enable_qs)
  );


  // R[intr_test]: V(True)

  prim_subreg_ext #(
    .DW    (1)
  ) u_intr_test (
    .re     (1'b0),
    .we     (intr_test_we),
    .wd     (intr_test_wd),
    .d      ('0),
    .qre    (),
    .qe     (reg2hw.intr_test.qe),
    .q      (reg2hw.intr_test.q ),
    .qs     ()
  );


  // R[cmd]: V(True)

  //   F[start]: 0:0
  prim_subreg_ext #(
    .DW    (1)
  ) u_cmd_start (
    .re     (1'b0),
    .we     (cmd_start_we),
    .wd     (cmd_start_wd),
    .d      ('0),
    .qre    (),
    .qe     (reg2hw.cmd.start.qe),
    .q      (reg2hw.cmd.start.q ),
    .qs     ()
  );


  //   F[dummy]: 1:1
  prim_subreg_ext #(
    .DW    (1)
  ) u_cmd_dummy (
    .re     (1'b0),
    .we     (cmd_dummy_we),
    .wd     (cmd_dummy_wd),
    .d      ('0),
    .qre    (),
    .qe     (reg2hw.cmd.dummy.qe),
    .q      (reg2hw.cmd.dummy.q ),
    .qs     ()
  );


  // R[status]: V(True)

  //   F[busy]: 0:0
  prim_subreg_ext #(
    .DW    (1)
  ) u_status_busy (
    .re     (status_busy_re),
    .we     (1'b0),
    .wd     ('0),
    .d      (hw2reg.status.busy.d),
    .qre    (),
    .qe     (),
    .q      (),
    .qs     (status_busy_qs)
  );


  //   F[dummy]: 1:1
  prim_subreg_ext #(
    .DW    (1)
  ) u_status_dummy (
    .re     (status_dummy_re),
    .we     (1'b0),
    .wd     ('0),
    .d      (hw2reg.status.dummy.d),
    .qre    (),
    .qe     (),
    .q      (),
    .qs     (status_dummy_qs)
  );


  // R[dotp_result]: V(True)

  prim_subreg_ext #(
    .DW    (32)
  ) u_dotp_result (
    .re     (dotp_result_re),
    .we     (1'b0),
    .wd     ('0),
    .d      (hw2reg.dotp_result.d),
    .qre    (),
    .qe     (),
    .q      (),
    .qs     (dotp_result_qs)
  );


  // R[wdata]: V(False)

  prim_subreg #(
    .DW      (32),
    .SWACCESS("WO"),
    .RESVAL  (32'h0)
  ) u_wdata (
    .clk_i   (clk_i    ),
    .rst_ni  (rst_ni  ),

    // from register interface
    .we     (wdata_we),
    .wd     (wdata_wd),

    // from internal hardware
    .de     (1'b0),
    .d      ('0  ),

    // to internal hardware
    .qe     (reg2hw.wdata.qe),
    .q      (reg2hw.wdata.q ),

    .qs     ()
  );




  logic [6:0] addr_hit;
  always_comb begin
    addr_hit = '0;
    addr_hit[0] = (reg_addr == VEC_DOT_INTR_STATE_OFFSET);
    addr_hit[1] = (reg_addr == VEC_DOT_INTR_ENABLE_OFFSET);
    addr_hit[2] = (reg_addr == VEC_DOT_INTR_TEST_OFFSET);
    addr_hit[3] = (reg_addr == VEC_DOT_CMD_OFFSET);
    addr_hit[4] = (reg_addr == VEC_DOT_STATUS_OFFSET);
    addr_hit[5] = (reg_addr == VEC_DOT_DOTP_RESULT_OFFSET);
    addr_hit[6] = (reg_addr == VEC_DOT_WDATA_OFFSET);
  end

  assign addrmiss = (reg_re || reg_we) ? ~|addr_hit : 1'b0 ;

  // Check sub-word write is permitted
  always_comb begin
    wr_err = 1'b0;
    if (addr_hit[0] && reg_we && (VEC_DOT_PERMIT[0] != (VEC_DOT_PERMIT[0] & reg_be))) wr_err = 1'b1 ;
    if (addr_hit[1] && reg_we && (VEC_DOT_PERMIT[1] != (VEC_DOT_PERMIT[1] & reg_be))) wr_err = 1'b1 ;
    if (addr_hit[2] && reg_we && (VEC_DOT_PERMIT[2] != (VEC_DOT_PERMIT[2] & reg_be))) wr_err = 1'b1 ;
    if (addr_hit[3] && reg_we && (VEC_DOT_PERMIT[3] != (VEC_DOT_PERMIT[3] & reg_be))) wr_err = 1'b1 ;
    if (addr_hit[4] && reg_we && (VEC_DOT_PERMIT[4] != (VEC_DOT_PERMIT[4] & reg_be))) wr_err = 1'b1 ;
    if (addr_hit[5] && reg_we && (VEC_DOT_PERMIT[5] != (VEC_DOT_PERMIT[5] & reg_be))) wr_err = 1'b1 ;
    if (addr_hit[6] && reg_we && (VEC_DOT_PERMIT[6] != (VEC_DOT_PERMIT[6] & reg_be))) wr_err = 1'b1 ;
  end

  assign intr_state_we = addr_hit[0] & reg_we & ~wr_err;
  assign intr_state_wd = reg_wdata[0];

  assign intr_enable_we = addr_hit[1] & reg_we & ~wr_err;
  assign intr_enable_wd = reg_wdata[0];

  assign intr_test_we = addr_hit[2] & reg_we & ~wr_err;
  assign intr_test_wd = reg_wdata[0];

  assign cmd_start_we = addr_hit[3] & reg_we & ~wr_err;
  assign cmd_start_wd = reg_wdata[0];

  assign cmd_dummy_we = addr_hit[3] & reg_we & ~wr_err;
  assign cmd_dummy_wd = reg_wdata[1];

  assign status_busy_re = addr_hit[4] && reg_re;

  assign status_dummy_re = addr_hit[4] && reg_re;

  assign dotp_result_re = addr_hit[5] && reg_re;

  assign wdata_we = addr_hit[6] & reg_we & ~wr_err;
  assign wdata_wd = reg_wdata[31:0];

  // Read data return
  always_comb begin
    reg_rdata_next = '0;
    unique case (1'b1)
      addr_hit[0]: begin
        reg_rdata_next[0] = intr_state_qs;
      end

      addr_hit[1]: begin
        reg_rdata_next[0] = intr_enable_qs;
      end

      addr_hit[2]: begin
        reg_rdata_next[0] = '0;
      end

      addr_hit[3]: begin
        reg_rdata_next[0] = '0;
        reg_rdata_next[1] = '0;
      end

      addr_hit[4]: begin
        reg_rdata_next[0] = status_busy_qs;
        reg_rdata_next[1] = status_dummy_qs;
      end

      addr_hit[5]: begin
        reg_rdata_next[31:0] = dotp_result_qs;
      end

      addr_hit[6]: begin
        reg_rdata_next[31:0] = '0;
      end

      default: begin
        reg_rdata_next = '1;
      end
    endcase
  end

  // Assertions for Register Interface
  `ASSERT_PULSE(wePulse, reg_we)
  `ASSERT_PULSE(rePulse, reg_re)

  `ASSERT(reAfterRv, $rose(reg_re || reg_we) |=> tl_o.d_valid)

  `ASSERT(en2addrHit, (reg_we || reg_re) |-> $onehot0(addr_hit))

  // this is formulated as an assumption such that the FPV testbenches do disprove this
  // property by mistake
  `ASSUME(reqParity, tl_reg_h2d.a_valid |-> tl_reg_h2d.a_user.parity_en == 1'b0)

endmodule
