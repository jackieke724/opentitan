// +FHDR========================================================================
//  License:
//      Copyright (c) 2019 Authors and BCRC. All rights reserved.
// =============================================================================
//  File Name:      ddrif.v
//                  Shiwei Liu (18212020026@fudan.edu.com)
//  Organization:   Brain-Chip Research Center (BCRC), Fudan University
//  Description:
//      DDR Interface
// -FHDR========================================================================

`resetall

//`include "INC_global.v"

module ddrif #(
    parameter   APP_AW = 28,
    parameter   APP_DW = 512,
    parameter   APP_MW = 64,
    parameter   DLA_DW = 64,
    parameter   DDR_DW = 64
)(
    // Global Signals
    input               ddrs_mosi_clk,
    input               rst,
    input               ui_clk,
    input               ui_rst,

    // Interface - DLA <-> DDR
    input       [DLA_DW-1:0]    ddrs_mosi,       // DLA -> DDR
    output      [DLA_DW-1:0]    ddrs_miso,       // DLA <- DDR
    input                       ddrs_mosi_valid, // DLA -> DDR, DDR WDATA Valid
    output                      ddrs_miso_valid, // DLA <- DDR, DDR RDATA Valid

    // Interface - MIG UI
    // Command Channel
    output      [APP_AW-1:0]    app_addr,
    output      [2:0]           app_cmd,
    output                      app_en,
    input                       app_rdy,

    // Write Channel
    input                       app_wdf_rdy,
    output      [APP_DW-1:0]    app_wdf_data,
    output      [APP_MW-1:0]    app_wdf_mask,
    output                      app_wdf_wren,
    output                      app_wdf_end,

    // Read Channel
    input       [APP_DW-1:0]    app_rd_data,
    input                       app_rd_data_valid
);

// =============================================================================
// UI -> DLA FIFO

    wire    [APP_DW-1:0]    u2d_rdata;
    wire                    u2d_ren;
    wire                    u2d_rempty;
    wire    [APP_DW-1:0]    u2d_wdata;
    wire                    u2d_wen;
    wire                    u2d_wfull;

    ddrif_fifo_u2h u2d_fifo_inst (
        .wr_clk (ui_clk),
        .wr_en  (u2d_wen),
        .din    (u2d_wdata),
        .full   (u2d_wfull),

        .rd_clk (ddrs_mosi_clk),
        .rd_en  (u2d_ren),
        .dout   (u2d_rdata),
        .empty  (u2d_rempty)
    );

// =============================================================================
// UI <- DLA FIFO

    wire    [APP_DW-1:0]    d2u_rdata;
    wire                    d2u_ren;
    wire                    d2u_rempty;
    wire    [APP_DW-1:0]    d2u_wdata;
    wire                    d2u_wen;
    wire                    d2u_wfull;

    ddrif_fifo_h2u d2u_fifo_inst (
        .wr_clk (ddrs_mosi_clk),
        .wr_en  (d2u_wen),
        .din    (d2u_wdata),
        .full   (d2u_wfull),

        .rd_clk (ui_clk),
        .rd_en  (d2u_ren),
        .dout   (d2u_rdata),
        .empty  (d2u_rempty)
    );

// =============================================================================
// DLA <-> UI Interface - DLA

    ddrif_dla #(
        .DLA_DW (DLA_DW),
        .APP_DW (APP_DW)
    ) ddrif_dla_inst (
        .clk            (ddrs_mosi_clk),
        .rst            (rst),

        .ddr_mosi       (ddrs_mosi),
        .ddr_miso       (ddrs_miso),
        .ddr_mosi_valid (ddrs_mosi_valid),
        .ddr_miso_valid (ddrs_miso_valid),

        .u2d_rdata      (u2d_rdata),
        .u2d_ren        (u2d_ren),
        .u2d_rempty     (u2d_rempty),

        .d2u_wdata      (d2u_wdata),
        .d2u_wen        (d2u_wen),
        .d2u_wfull      (d2u_wfull)
    );

// =============================================================================
// DLA <-> UI Interface - UI

    ddrif_ui #(
        .APP_AW     (APP_AW),
        .APP_DW     (APP_DW),
        .APP_MW     (APP_MW),
        .DLA_DW     (DLA_DW),
        .DDR_DW     (DDR_DW)
    ) ddrif_ui_inst (
        .ui_clk         (ui_clk),
        .ui_rst         (ui_rst),

        .app_addr       (app_addr),
        .app_cmd        (app_cmd),
        .app_en         (app_en),
        .app_rdy        (app_rdy),

        .app_wdf_rdy    (app_wdf_rdy),
        .app_wdf_data   (app_wdf_data),
        .app_wdf_mask   (app_wdf_mask),
        .app_wdf_wren   (app_wdf_wren),
        .app_wdf_end    (app_wdf_end),

        .app_rd_data        (app_rd_data),
        .app_rd_data_valid  (app_rd_data_valid),

        .d2u_rdata      (d2u_rdata),
        .d2u_ren        (d2u_ren),
        .d2u_rempty     (d2u_rempty),

        .u2d_wdata      (u2d_wdata),
        .u2d_wen        (u2d_wen),
        .u2d_wfull      (u2d_wfull)
    );

endmodule
