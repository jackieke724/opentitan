// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//


`include "prim_assert.sv"

module ddr_ctrl import ddr_ctrl_pkg::*; (
  input clk_i,
  input rst_ni,
  
  input clk_ddr_i,

  input  tlul_pkg::tl_h2d_t tl_i,
  output tlul_pkg::tl_d2h_t tl_o,
  
  output mig_pins_out_t mig_pins_out_o,
  inout mig_pins_inout_t mig_pins_inout_o,
  
  /*
  input [31:0]	cio_ddr3_dq_i,      
  input [3:0] 	cio_ddr3_dqs_n_i,   
  input [3:0] 	cio_ddr3_dqs_p_i,   
  
  output [14:0]	cio_ddr3_addr_o,      
  output [14:0]	cio_ddr3_addr_en_o,   
  output [2:0] 	cio_ddr3_ba_o,        
  output [2:0] 	cio_ddr3_ba_en_o,     
  output       	cio_ddr3_ras_n_o,     
  output       	cio_ddr3_ras_n_en_o,  
  output       	cio_ddr3_cas_n_o,     
  output       	cio_ddr3_cas_n_en_o,  
  output       	cio_ddr3_we_n_o,      
  output       	cio_ddr3_we_n_en_o,   
  output       	cio_ddr3_reset_n_o,   
  output       	cio_ddr3_reset_n_en_o,
  output       	cio_ddr3_ck_p_o,      
  output       	cio_ddr3_ck_p_en_o,   
  output       	cio_ddr3_ck_n_o,      
  output       	cio_ddr3_ck_n_en_o,   
  output       	cio_ddr3_cke_o,       
  output       	cio_ddr3_cke_en_o,    
  output       	cio_ddr3_cs_n_o,      
  output       	cio_ddr3_cs_n_en_o,   
  output [3:0] 	cio_ddr3_dm_o,        
  output [3:0] 	cio_ddr3_dm_en_o,     
  output       	cio_ddr3_o,dt_o,       
  output       	cio_ddr3_o,dt_en_o,    
  output [31:0]	cio_ddr3_dq_o,        
  output [31:0]	cio_ddr3_dq_en_o,     
  output [3:0] 	cio_ddr3_dqs_n_o,     
  output [3:0] 	cio_ddr3_dqs_n_en_o,  
  output [3:0] 	cio_ddr3_dqs_p_o,     
  output [3:0] 	cio_ddr3_dqs_p_en_o,
  */
  
  // Inter-module signals
  output logic idle_o

  // Interrupts
  //output logic intr_done_o
);
  
  import ddr_ctrl_reg_pkg::*;

  ddr_ctrl_reg2hw_t reg2hw;
  ddr_ctrl_hw2reg_t hw2reg;
  
  logic start, done;
  logic busy_d, busy_q;

  
  // Inter-module signals ======================================================

  // TODO: Better define what "idle" means -- only the core, or also the
  // register interface?
  assign idle_o = 1'b0;


  // Interrupts ================================================================
  /*
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
  */
  
  /////////////////////////////////////////////////
  // Connecting register interface to the signal //
  /////////////////////////////////////////////////
  
  // Register module
  ddr_ctrl_reg_top u_reg (
    .clk_i,
    .rst_ni,

    .tl_i,
    .tl_o,
    
    .reg2hw,
    .hw2reg,

    .devmode_i  (1'b1)
  );
  
  
  //ddrif
  wire            ui_clk;
  wire            ui_rst;

  wire    [28:0]  app_addr;
  wire    [2:0]   app_cmd;
  wire            app_en;
  wire            app_rdy;

  wire            app_wdf_rdy;
  wire    [255:0] app_wdf_data;
  wire    [31:0]  app_wdf_mask;
  wire            app_wdf_wren;
  wire            app_wdf_end;

  wire    [255:0] app_rd_data;
  wire            app_rd_data_valid;
  
  wire            init_calib_complete;
  
  wire    [63:0]  ddrs_miso;
  wire            ddrs_miso_valid;
  wire    [63:0]  ddrs_mosi = {reg2hw.ddrs_mosi_u.q, reg2hw.ddrs_mosi_l.q};
  wire            ddrs_mosi_valid = reg2hw.ddrs_mosi_valid.qe;
  assign          hw2reg.ddrs_miso_u.d = ddrs_miso[63:32];
  assign          hw2reg.ddrs_miso_u.de = ddrs_miso_valid;
  assign          hw2reg.ddrs_miso_l.d = ddrs_miso[31:0];
  assign          hw2reg.ddrs_miso_l.de = ddrs_miso_valid;
  assign          hw2reg.ddrs_miso_valid.d = ddrs_miso_valid;
  assign          hw2reg.ddrs_miso_valid.de = ddrs_miso_valid;
  assign          hw2reg.init_calib_complete.d = init_calib_complete;
  assign          hw2reg.init_calib_complete.de = 1'b1;
  

  ddrif #(
    .APP_AW     (29),
    .APP_DW     (256),
    .APP_MW     (32),
    .DLA_DW     (64),
    .DDR_DW     (32)
  ) ddrif_inst (
    .ddrs_mosi_clk              (clk_i),
    .rst                        (~rst_ni),
    .ui_clk                     (ui_clk),
    .ui_rst                     (ui_rst),

    .ddrs_mosi                  (ddrs_mosi),
    .ddrs_miso                  (ddrs_miso),
    .ddrs_mosi_valid            (ddrs_mosi_valid),
    .ddrs_miso_valid            (ddrs_miso_valid),

    .app_addr                   (app_addr),
    .app_cmd                    (app_cmd),
    .app_en                     (app_en),
    .app_rdy                    (app_rdy),

    .app_wdf_rdy                (app_wdf_rdy),
    .app_wdf_data               (app_wdf_data),
    .app_wdf_mask               (app_wdf_mask),
    .app_wdf_wren               (app_wdf_wren),
    .app_wdf_end                (app_wdf_end),

    .app_rd_data                (app_rd_data),
    .app_rd_data_valid          (app_rd_data_valid)
  );
    
  // MIG 7
  mig7 u_mig7 (

    // Memory interface ports
    .ddr3_addr                      (mig_pins_out_o.ddr3_addr),  // output [14:0]		ddr3_addr
    .ddr3_ba                        (mig_pins_out_o.ddr3_ba),  // output [2:0]		ddr3_ba
    .ddr3_cas_n                     (mig_pins_out_o.ddr3_cas_n),  // output			ddr3_cas_n
    .ddr3_ck_n                      (mig_pins_out_o.ddr3_ck_n),  // output [0:0]		ddr3_ck_n
    .ddr3_ck_p                      (mig_pins_out_o.ddr3_ck_p),  // output [0:0]		ddr3_ck_p
    .ddr3_cke                       (mig_pins_out_o.ddr3_cke),  // output [0:0]		ddr3_cke
    .ddr3_ras_n                     (mig_pins_out_o.ddr3_ras_n),  // output			ddr3_ras_n
    .ddr3_reset_n                   (mig_pins_out_o.ddr3_reset_n),  // output			ddr3_reset_n
    .ddr3_we_n                      (mig_pins_out_o.ddr3_we_n),  // output			ddr3_we_n
    .ddr3_dq                        (mig_pins_inout_o.ddr3_dq),  // inout [31:0]		ddr3_dq
    .ddr3_dqs_n                     (mig_pins_inout_o.ddr3_dqs_n),  // inout [3:0]		ddr3_dqs_n
    .ddr3_dqs_p                     (mig_pins_inout_o.ddr3_dqs_p),  // inout [3:0]		ddr3_dqs_p
    .init_calib_complete            (init_calib_complete),  // output			init_calib_complete
      
    .ddr3_cs_n                      (mig_pins_out_o.ddr3_cs_n),  // output [0:0]		ddr3_cs_n
    .ddr3_dm                        (mig_pins_out_o.ddr3_dm),  // output [3:0]		ddr3_dm
    .ddr3_odt                       (mig_pins_out_o.ddr3_odt),  // output [0:0]		ddr3_odt
    // Application interface ports
    .app_addr                       (app_addr),  // input [28:0]		app_addr
    .app_cmd                        (app_cmd),  // input [2:0]		app_cmd
    .app_en                         (app_en),  // input				app_en
    .app_wdf_data                   (app_wdf_data),  // input [255:0]		app_wdf_data
    .app_wdf_end                    (app_wdf_end),  // input				app_wdf_end
    .app_wdf_wren                   (app_wdf_wren),  // input				app_wdf_wren
    .app_rd_data                    (app_rd_data),  // output [255:0]		app_rd_data
    //.app_rd_data_end                (app_rd_data_end),  // output			app_rd_data_end
    .app_rd_data_valid              (app_rd_data_valid),  // output			app_rd_data_valid
    .app_rdy                        (app_rdy),  // output			app_rdy
    .app_wdf_rdy                    (app_wdf_rdy),  // output			app_wdf_rdy
    .app_wdf_mask                   (app_wdf_mask),  // input [31:0]		app_wdf_mask
    .app_sr_req                     (1'b0),  // input			app_sr_req
    .app_ref_req                    (1'b0),  // input			app_ref_req
    .app_zq_req                     (1'b0),  // input			app_zq_req
    //.app_sr_active                  (app_sr_active),  // output			app_sr_active
    //.app_ref_ack                    (app_ref_ack),  // output			app_ref_ack
    //.app_zq_ack                     (app_zq_ack),  // output			app_zq_ack
    .ui_clk                         (ui_clk),  // output			ui_clk
    .ui_clk_sync_rst                (ui_rst),  // output			ui_clk_sync_rst
    
    // System Clock Ports
    .sys_clk_i                      (clk_ddr_i),  // input				sys_clk_p
    .sys_rst                        (rst_ni) // input sys_rst
  );


/*
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
*/

  
  ////////////////
  // Assertions //
  ////////////////
  //`ASSERT_KNOWN(TlODValidKnown, tl_o.d_valid)
endmodule
