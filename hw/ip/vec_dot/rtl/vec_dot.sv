// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//


`include "prim_assert.sv"

module vec_dot (
  input clk_i,
  input rst_ni,

  input  tlul_pkg::tl_h2d_t tl_i,
  output tlul_pkg::tl_d2h_t tl_o,
  
  // Inter-module signals
  output logic idle_o,

  // Interrupts
  output logic intr_done_o
);

  localparam int VEC_LEN  = 4;
  
  logic start, done, busy;
  logic busy_d, busy_q;

  import vec_dot_reg_pkg::*;

  vec_dot_reg2hw_t reg2hw;
  vec_dot_hw2reg_t hw2reg;
  
  
  // Inter-module signals ======================================================

  // TODO: Better define what "idle" means -- only the core, or also the
  // register interface?
  assign idle_o = ~busy_q & ~start;


  // Interrupts ================================================================

  prim_intr_hw #(
    .Width(1)
  ) u_intr_hw_done (
    .clk_i,
    .rst_ni,
    .event_intr_i           (done),
    .reg2hw_intr_enable_q_i (reg2hw.intr_enable.q),
    .reg2hw_intr_test_q_i   (reg2hw.intr_test.q),
    .reg2hw_intr_test_qe_i  (reg2hw.intr_test.qe),
    .reg2hw_intr_state_q_i  (reg2hw.intr_state.q),
    .hw2reg_intr_state_de_o (hw2reg.intr_state.de),
    .hw2reg_intr_state_d_o  (hw2reg.intr_state.d),
    .intr_o                 (intr_done_o)
  );
  
  
  /////////////////////////////////////////////////
  // Connecting register interface to the signal //
  /////////////////////////////////////////////////
  
  vec_dot_core #(
    .VEC_LEN (VEC_LEN)
  ) u_core (
    .clk_i,
    .rst_ni,
    
    .data_i   (reg2hw.wdata.q),
    .wr_en_i  (reg2hw.wdata.qe),
    .start_i  (start),
    .busy_o   (busy),
    .result_o (hw2reg.dotp_result.d),
    .done_o   (done)
  );


  // Register module
  vec_dot_reg_top u_reg (
    .clk_i,
    .rst_ni,

    .tl_i,
    .tl_o,

    .reg2hw,
    .hw2reg,

    .devmode_i  (1'b1)
  );
  
  
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      busy_q <= 1'b0;
    end else begin
      busy_q <= busy_d;
    end
  end
  assign busy_d = (busy_q | start) & ~done;
  
	assign start = reg2hw.cmd.start.qe & reg2hw.cmd.start.q;
	assign hw2reg.status.busy.d = busy_q;
	assign hw2reg.status.dummy.d = 1'b0;

  ////////////////
  // Assertions //
  ////////////////
  //`ASSERT_KNOWN(TlODValidKnown, tl_o.d_valid)
endmodule
