// +FHDR========================================================================
//  License:
//      Copyright (c) 2019 Authors and BCRC. All rights reserved.
// =============================================================================
//  File Name:      dla.v
//                  Huifeng Ke
//  Organization:   Brain-Chip Research Center (BCRC), Fudan University
//  Description:
//      DLA Top Wrapper


module dla (
    // Global Signals
    input               SYSCLK_P,
    input               SYSCLK_N,
    input               IO_RST_N,
    output [3:0]        led,

    //AXI Chip2Chip FMC ports - converted from TLUL
    output wire axi_c2c_selio_tx_diff_clk_out_p,
    output wire axi_c2c_selio_tx_diff_clk_out_n,
    input wire axi_c2c_selio_rx_diff_clk_in_p,
    input wire axi_c2c_selio_rx_diff_clk_in_n,
    output wire [30 : 0] axi_c2c_selio_tx_data_out,
    input wire [30 : 0] axi_c2c_selio_rx_data_in,

    //DDR3 pins
    output [14:0]        ddr3_addr,
    output [2:0]         ddr3_ba,
    output               ddr3_ras_n,
    output               ddr3_cas_n,
    output               ddr3_we_n,
    output               ddr3_reset_n,
    output [0:0]         ddr3_ck_p,
    output [0:0]         ddr3_ck_n,
    output [0:0]         ddr3_cke,
    output [0:0]         ddr3_cs_n,
    output [3:0]         ddr3_dm,
    output [0:0]         ddr3_odt,
    inout [31:0]         ddr3_dq,
    inout [3:0]          ddr3_dqs_n,
    inout [3:0]          ddr3_dqs_p,

    // Inter-module signals
    output logic        idle_o,
  
    // Interface - Interrupt
    output              intr_done_o
);
    //assign  ddr_mosi_clk = clk;
    localparam int WLEN    = 64;
    localparam int DmemSizeByte = dla_reg_pkg::DLA_DMEM_SIZE;
    localparam int DmemAddrWidth = vbits(DmemSizeByte);
    localparam int DmemSizeWords = DmemSizeByte / (WLEN / 8);
    localparam int DmemIndexWidth = vbits(DmemSizeWords);
    
    import dla_reg_pkg::*;
    import prim_util_pkg::vbits;

    dla_reg2hw_t reg2hw;
    dla_hw2reg_t hw2reg;

    tlul_pkg::tl_h2d_t tl_win_h2d[1];
    tlul_pkg::tl_d2h_t tl_win_d2h[1];
    
    logic done;
    logic init_calib_complete;
    logic [DmemAddrWidth:0] wptr;

    // Chip2Chip ==============================================================
    tlul_pkg::tl_h2d_t tl_h2d;
    tlul_pkg::tl_d2h_t tl_d2h;

    logic clk_200mhz, clk_i, rst_ni;

    dla_clkgen clk_gen_inst(
        .SYSCLK_P,
        .SYSCLK_N,
        .IO_RST_N,
        .clk_200mhz(clk_200mhz),
        .clk_main(clk_i),
        .rst_n(rst_ni)
    );

    wire axi_c2c_link_status_out;
    wire axi_c2c_multi_bit_error_out;

    chip_device chip_device_inst(
        .clk_i,
        .rst_ni,
        
        .tl_h2d,
        .tl_d2h,
        
        .idelay_ref_clk(clk_200mhz),
        .axi_c2c_selio_tx_diff_clk_out_p(axi_c2c_selio_tx_diff_clk_out_p),
        .axi_c2c_selio_tx_diff_clk_out_n(axi_c2c_selio_tx_diff_clk_out_n),
        .axi_c2c_selio_tx_data_out(axi_c2c_selio_tx_data_out),
        .axi_c2c_selio_rx_diff_clk_in_p(axi_c2c_selio_rx_diff_clk_in_p),
        .axi_c2c_selio_rx_diff_clk_in_n(axi_c2c_selio_rx_diff_clk_in_n),  
        .axi_c2c_selio_rx_data_in(axi_c2c_selio_rx_data_in),
        .axi_c2c_link_status_out(axi_c2c_link_status_out),
        .axi_c2c_multi_bit_error_out(axi_c2c_multi_bit_error_out)
    );

    assign led = {  
//                    1'b1, 
//                    1'b0, 
//                    1'b1,
//                    1'b0,
                    tl_d2h.d_valid,
                    tl_d2h.a_ready,
                    axi_c2c_multi_bit_error_out, 
                    axi_c2c_link_status_out};
    
    // Inter-module signals ======================================================

    // TODO: Better define what "idle" means -- only the core, or also the
    // register interface?
    //not busy and not start of each operation
    assign idle_o = ~hw2reg.gst_status.ddr2gb.d 
                    & ~(reg2hw.ddr2gb_ctrl.go.q &reg2hw.ddr2gb_ctrl.go.qe)
                    & ~hw2reg.gst_status.gb2lb.d
                    & ~(reg2hw.gb2lb_ctrl.go.q &reg2hw.gb2lb_ctrl.go.qe)
                    & ~hw2reg.gst_status.conv.d
                    & ~(reg2hw.comp_ctrl.go_conv.q &reg2hw.comp_ctrl.go_conv.qe)
                    & ~hw2reg.gst_status.fc.d
                    & ~(reg2hw.comp_ctrl.go_fc.q &reg2hw.comp_ctrl.go_fc.qe)
                    & ~hw2reg.gst_status.ppe.d
                    & ~(reg2hw.ppe_ctrl.go.q &reg2hw.ppe_ctrl.go.qe);

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
    assign dmem_access_core = ~reg2hw.cpu_rd.q;

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
    
    // Registers =================================================================
    dla_reg_top u_reg (
        .clk_i,
        .rst_ni,
        .tl_i(tl_h2d),
        .tl_o(tl_d2h),
        .tl_win_o (tl_win_h2d),
        .tl_win_i (tl_win_d2h),

        .reg2hw,
        .hw2reg,

        .devmode_i (1'b1)
    );

    assign hw2reg.init_calib_complete.d = init_calib_complete;
    assign hw2reg.init_calib_complete.de = 1'b1;
    assign hw2reg.rxf_ctrl.wptr.d = {'0, wptr};
    assign hw2reg.rxf_ctrl.wptr.de = 1'b1;  

    // MIG 7 ===================================================================
    logic            ui_clk;
    logic            ui_rst;

    logic    [28:0]  app_addr;
    logic    [2:0]   app_cmd;
    logic            app_en;
    logic            app_rdy;

    logic            app_wdf_rdy;
    logic    [255:0] app_wdf_data;
    logic    [31:0]  app_wdf_mask;
    logic            app_wdf_wren;
    logic            app_wdf_end;

    logic    [255:0] app_rd_data;
    logic            app_rd_data_valid;


    mig7 u_mig7 (
        // Memory interface ports
        .ddr3_addr                      (ddr3_addr),  // output [14:0]       ddr3_addr
        .ddr3_ba                        (ddr3_ba),  // output [2:0]      ddr3_ba
        .ddr3_cas_n                     (ddr3_cas_n),  // output         ddr3_cas_n
        .ddr3_ck_n                      (ddr3_ck_n),  // output [0:0]        ddr3_ck_n
        .ddr3_ck_p                      (ddr3_ck_p),  // output [0:0]        ddr3_ck_p
        .ddr3_cke                       (ddr3_cke),  // output [0:0]     ddr3_cke
        .ddr3_ras_n                     (ddr3_ras_n),  // output         ddr3_ras_n
        .ddr3_reset_n                   (ddr3_reset_n),  // output           ddr3_reset_n
        .ddr3_we_n                      (ddr3_we_n),  // output          ddr3_we_n
        .ddr3_dq                        (ddr3_dq),  // inout [31:0]        ddr3_dq
        .ddr3_dqs_n                     (ddr3_dqs_n),  // inout [3:0]      ddr3_dqs_n
        .ddr3_dqs_p                     (ddr3_dqs_p),  // inout [3:0]      ddr3_dqs_p
        .init_calib_complete            (init_calib_complete),  // output           init_calib_complete
          
        .ddr3_cs_n                      (ddr3_cs_n),  // output [0:0]        ddr3_cs_n
        .ddr3_dm                        (ddr3_dm),  // output [3:0]      ddr3_dm
        .ddr3_odt                       (ddr3_odt),  // output [0:0]     ddr3_odt
        // Application interface ports
        .app_addr                       (app_addr),  // input [28:0]        app_addr
        .app_cmd                        (app_cmd),  // input [2:0]      app_cmd
        .app_en                         (app_en),  // input             app_en
        .app_wdf_data                   (app_wdf_data),  // input [255:0]       app_wdf_data
        .app_wdf_end                    (app_wdf_end),  // input                app_wdf_end
        .app_wdf_wren                   (app_wdf_wren),  // input               app_wdf_wren
        .app_rd_data                    (app_rd_data),  // output [255:0]       app_rd_data
        //.app_rd_data_end                (app_rd_data_end),  // output         app_rd_data_end
        .app_rd_data_valid              (app_rd_data_valid),  // output         app_rd_data_valid
        .app_rdy                        (app_rdy),  // output           app_rdy
        .app_wdf_rdy                    (app_wdf_rdy),  // output           app_wdf_rdy
        .app_wdf_mask                   (app_wdf_mask),  // input [31:0]        app_wdf_mask
        .app_sr_req                     (1'b0),  // input           app_sr_req
        .app_ref_req                    (1'b0),  // input           app_ref_req
        .app_zq_req                     (1'b0),  // input           app_zq_req
        //.app_sr_active                  (app_sr_active),  // output           app_sr_active
        //.app_ref_ack                    (app_ref_ack),  // output         app_ref_ack
        //.app_zq_ack                     (app_zq_ack),  // output          app_zq_ack
        .ui_clk                         (ui_clk),  // output            ui_clk
        .ui_clk_sync_rst                (ui_rst),  // output            ui_clk_sync_rst
        
        // System Clock Ports
        .sys_clk_i                      (clk_200mhz),  // input              sys_clk_p
        .sys_rst                        (rst_ni) // input sys_rst
    );

    // ddr_mosi/miso* <-> DDR ==================================================
    logic [WLEN-1:0]                dla_ddr_mosi;
    logic [WLEN-1:0]                dla_ddr_miso;
    logic                           dla_ddr_mosi_valid;
    logic                           dla_ddr_miso_valid;

    logic [WLEN-1:0]                bus_ddr_mosi;
    logic [WLEN-1:0]                bus_ddr_miso;
    logic                           bus_ddr_mosi_valid;
    logic                           bus_ddr_miso_valid;

    logic [WLEN-1:0]                ddr_mosi;
    logic [WLEN-1:0]                ddr_miso;
    logic                           ddr_mosi_valid;
    logic                           ddr_miso_valid;

    logic                           ddrif_access_dla;

    assign bus_ddr_mosi = {reg2hw.ddr_mosi_u.q, reg2hw.ddr_mosi_l.q};
    assign bus_ddr_mosi_valid = reg2hw.ddr_mosi_valid.qe;
    assign hw2reg.ddr_miso_valid.d = bus_ddr_miso_valid;
    assign hw2reg.ddr_miso_valid.de = bus_ddr_miso_valid;

    //make sure host does not issue ddr2gb commands through registers
    //when host wants to read from ddr
    assign ddrif_access_dla = ~reg2hw.cpu_access_ddr.q;

    assign ddr_mosi             = ddrif_access_dla ? dla_ddr_mosi       : bus_ddr_mosi;
    assign ddr_mosi_valid       = ddrif_access_dla ? dla_ddr_mosi_valid : bus_ddr_mosi_valid;

    assign dla_ddr_miso         = ddrif_access_dla ? ddr_miso       : '0;
    assign dla_ddr_miso_valid   = ddrif_access_dla ? ddr_miso_valid : 1'b0;

    assign bus_ddr_miso         = ddrif_access_dla ? '0     : ddr_miso;
    assign bus_ddr_miso_valid   = ddrif_access_dla ? 1'b0   : ddr_miso_valid;

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
        
        .ddrs_mosi                  (ddr_mosi),
        .ddrs_miso                  (ddr_miso),
        .ddrs_mosi_valid            (ddr_mosi_valid),
        .ddrs_miso_valid            (ddr_miso_valid),

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

    // DDR miso to DMEM =========================================================
    ddr_ctrl_rxf_ctrl #(
        .WLEN(WLEN),
        .DmemSizeByte(DmemSizeByte)
    ) ddr_ctrl_rxf_ctrl_inst (
        .clk_i,
        .rst_ni,

        .wptr,

        .ddr_miso                           (bus_ddr_miso),
        .ddr_miso_valid                     (bus_ddr_miso_valid),

        .dmem_req_o                         (dmem_req_core),
        .dmem_write_o                       (dmem_write_core),
        .dmem_addr_o                        (dmem_addr_core),
        .dmem_wdata_o                       (dmem_wdata_core),
        .dmem_wmask_o                       (dmem_wmask_core),
        .dmem_rdata_i                       (dmem_rdata_core),
        .dmem_rvalid_i                      (dmem_rvalid_core),
        .dmem_rerror_i                      (dmem_rerror_core)
    );
    /*
    // DDR-RAM interface =========================================================
    logic [WLEN-1:0]                            ddr_mosi;
    logic [WLEN-1:0]                            ddr_miso;
    logic                                       ddr_mosi_valid;
    logic                                       ddr_miso_valid;
    
    ddr_ram_interface #(
        .WLEN(WLEN),
        .DmemSizeByte(DmemSizeByte)
    ) u_ddr_ram_if(
        .clk_i,
        .rst_ni,
        
        .dmem_req_o    (dmem_req_core),
        .dmem_write_o  (dmem_write_core),
        .dmem_addr_o   (dmem_addr_core),
        .dmem_wdata_o  (dmem_wdata_core),
        .dmem_wmask_o  (dmem_wmask_core),
        .dmem_rdata_i  (dmem_rdata_core),
        .dmem_rvalid_i (dmem_rvalid_core),
        .dmem_rerror_i (dmem_rerror_core),
        
        .ddr_miso,
        .ddr_mosi,
        .ddr_miso_valid,
        .ddr_mosi_valid
    );*/

    // DLA Core ==================================================================
    dla_core u_dla_core (
        .clk(clk_i),
        .rst(~rst_ni),
        
        .reg2hw,
        .hw2reg_gst_status(hw2reg.gst_status),
        .hw2reg_gst_intr(hw2reg.gst_intr),
        
        .ddr_miso_clk(clk_i),
        .ddr_mosi_clk(),
        .ddr_miso(dla_ddr_miso),
        .ddr_mosi(dla_ddr_mosi),
        .ddr_miso_valid(dla_ddr_miso_valid),
        .ddr_mosi_valid(dla_ddr_mosi_valid),
        .ddr_miso_en(),
        .ddr_mosi_en(),
        
        .interrupt(done)
    );
    
endmodule
