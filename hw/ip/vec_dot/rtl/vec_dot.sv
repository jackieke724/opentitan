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
  
  import prim_util_pkg::vbits;
  
  localparam int VEC_LEN  = 4;
  localparam int WLEN    = 64;

  localparam int DmemSizeByte = otbn_reg_pkg::OTBN_DMEM_SIZE;
  localparam int DmemAddrWidth = vbits(DmemSizeByte);

  logic start, done;
  logic busy_d, busy_q;

  import vec_dot_reg_pkg::*;

  vec_dot_reg2hw_t reg2hw;
  vec_dot_hw2reg_t hw2reg;
  
  tlul_pkg::tl_h2d_t tl_win_h2d[1];
  tlul_pkg::tl_d2h_t tl_win_d2h[1];
  
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
  
  
  // Data Memory (DMEM) ========================================================

  localparam int DmemSizeWords = DmemSizeByte / (WLEN / 8);
  localparam int DmemIndexWidth = vbits(DmemSizeWords);

  // Access select to DMEM: core (1), or bus (0)
  logic dmem_access_core;

  logic dmem_req;
  logic dmem_write;
  logic [DmemIndexWidth-1:0] dmem_index;
  logic [WLEN-1:0] dmem_wdata;
  logic [WLEN-1:0] dmem_wmask;
  logic [WLEN-1:0] dmem_rdata;
  logic dmem_rvalid;
  logic [1:0] dmem_rerror_vec;
  logic dmem_rerror;

  logic dmem_req_core;
  logic dmem_write_core;
  logic [DmemIndexWidth-1:0] dmem_index_core;
  logic [WLEN-1:0] dmem_wdata_core;
  logic [WLEN-1:0] dmem_wmask_core;
  logic [WLEN-1:0] dmem_rdata_core;
  logic dmem_rvalid_core;
  logic dmem_rerror_core;

  logic dmem_req_bus;
  logic dmem_write_bus;
  logic [DmemIndexWidth-1:0] dmem_index_bus;
  logic [WLEN-1:0] dmem_wdata_bus;
  logic [WLEN-1:0] dmem_wmask_bus;
  logic [WLEN-1:0] dmem_rdata_bus;
  logic dmem_rvalid_bus;
  logic [1:0] dmem_rerror_bus;

  logic [DmemAddrWidth-1:0] dmem_addr_core;
  assign dmem_index_core = dmem_addr_core[DmemAddrWidth-1:DmemAddrWidth-DmemIndexWidth];

  prim_ram_1p_adv #(
    .Width           (WLEN),
    .Depth           (DmemSizeWords),
    .DataBitsPerMask (32), // 32b write masks for 32b word writes from bus
    .CfgW            (8)
  ) u_dmem (
    .clk_i,
    .rst_ni,
    .req_i    (dmem_req),
    .write_i  (dmem_write),
    .addr_i   (dmem_index),
    .wdata_i  (dmem_wdata),
    .wmask_i  (dmem_wmask),
    .rdata_o  (dmem_rdata),
    .rvalid_o (dmem_rvalid),
    .rerror_o (dmem_rerror_vec),
    .cfg_i    ('0)
  );

  // Combine uncorrectable / correctable errors. See note above definition of imem_rerror for
  // details.
  assign dmem_rerror = |dmem_rerror_vec;

  // DMEM access from main TL-UL bus
  logic dmem_gnt_bus;
  assign dmem_gnt_bus = dmem_req_bus & ~dmem_access_core;

  tlul_adapter_sram #(
    .SramAw      (DmemIndexWidth),
    .SramDw      (WLEN),
    .Outstanding (1),
    .ByteAccess  (0),
    .ErrOnRead   (0)
  ) u_tlul_adapter_sram_dmem (
    .clk_i,
    .rst_ni,

    .tl_i     (tl_win_h2d[0]),
    .tl_o     (tl_win_d2h[0]),

    .req_o    (dmem_req_bus   ),
    .gnt_i    (dmem_gnt_bus   ),
    .we_o     (dmem_write_bus ),
    .addr_o   (dmem_index_bus ),
    .wdata_o  (dmem_wdata_bus ),
    .wmask_o  (dmem_wmask_bus ),
    .rdata_i  (dmem_rdata_bus ),
    .rvalid_i (dmem_rvalid_bus),
    .rerror_i (dmem_rerror_bus)
  );

  // Mux core and bus access into dmem
  assign dmem_access_core = busy_q;

  assign dmem_req   = dmem_access_core ? dmem_req_core   : dmem_req_bus;
  assign dmem_write = dmem_access_core ? dmem_write_core : dmem_write_bus;
  assign dmem_wmask = dmem_access_core ? dmem_wmask_core : dmem_wmask_bus;
  assign dmem_index = dmem_access_core ? dmem_index_core : dmem_index_bus;
  assign dmem_wdata = dmem_access_core ? dmem_wdata_core : dmem_wdata_bus;

  // Explicitly tie off bus interface during core operation to avoid leaking
  // DMEM data through the bus unintentionally.
  assign dmem_rdata_bus  = !dmem_access_core ? dmem_rdata : '0;
  assign dmem_rdata_core = dmem_rdata;

  assign dmem_rvalid_bus  = !dmem_access_core ? dmem_rvalid : 1'b0;
  assign dmem_rvalid_core = dmem_access_core  ? dmem_rvalid : 1'b0;

  // Expand the error signal to 2 bits and mask when the core has access. See note above
  // imem_rerror_bus for details.
  assign dmem_rerror_bus  = !dmem_access_core ? {dmem_rerror, 1'b0} : 2'b00;
  assign dmem_rerror_core = dmem_rerror;

  
  /////////////////////////////////////////////////
  // Connecting register interface to the signal //
  /////////////////////////////////////////////////
  
  // Register module
  vec_dot_reg_top u_reg (
    .clk_i,
    .rst_ni,

    .tl_i,
    .tl_o,
    
    .tl_win_o (tl_win_h2d),
    .tl_win_i (tl_win_d2h),

    .reg2hw,
    .hw2reg,

    .devmode_i  (1'b1)
  );
  
  // Core
  vec_dot_core #(
    .VEC_LEN (VEC_LEN)
  ) u_core (
    .clk_i,
    .rst_ni,
    
    .data_i   (reg2hw.wdata.q),
    .wr_en_i  (reg2hw.wdata.qe),
    .mode_i   (reg2hw.mode.q),
    .start_i  (start),
    .result_o (hw2reg.dotp_result.d),
    .done_o   (done),
    
    .dmem_req_o    (dmem_req_core),
    .dmem_write_o  (dmem_write_core),
    .dmem_addr_o   (dmem_addr_core),
    .dmem_wdata_o  (dmem_wdata_core),
    .dmem_wmask_o  (dmem_wmask_core),
    .dmem_rdata_i  (dmem_rdata_core),
    .dmem_rvalid_i (dmem_rvalid_core),
    .dmem_rerror_i (dmem_rerror_core)
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
