// +FHDR========================================================================
//  License:
//      Copyright (c) 2019 Authors and BCRC. All rights reserved.
// =============================================================================
//  File Name:      ddrif_ui.v
//                  Shiwei Liu (18212020026@fudan.edu.com)
//  Organization:   Brain-Chip Research Center (BCRC), Fudan University
//  Description:
//      DDR Interface - User Interface. Refer UI document for more information.
// -FHDR========================================================================

`resetall

//`include "INC_global.v"

module ddrif_ui #(
    parameter   APP_AW = 28,
    parameter   APP_DW = 512,
    parameter   APP_MW = 64,
    parameter   DLA_DW = 64,
    parameter   DDR_DW = 64
)(
    // Global Signals
    input               ui_clk,
    input               ui_rst,

    // Interface - MIG UI
    // Command Channel
    output  reg [APP_AW-1:0]    app_addr,
    output  reg [2:0]           app_cmd,
    output  reg                 app_en,
    input                       app_rdy,

    // Write Channel
    input                       app_wdf_rdy,
    output  reg [APP_DW-1:0]    app_wdf_data,
    output      [APP_MW-1:0]    app_wdf_mask,
    output  reg                 app_wdf_wren,
    output  reg                 app_wdf_end,

    // Read Channel
    input       [APP_DW-1:0]    app_rd_data,
    input                       app_rd_data_valid,

    // Interface - DLA -> UI FIFO
    input       [APP_DW-1:0]    d2u_rdata,
    output  reg                 d2u_ren,
    input                       d2u_rempty,

    // Interface - DLA <- UI FIFO
    output  reg [APP_DW-1:0]    u2d_wdata,
    output  reg                 u2d_wen,
    input                       u2d_wfull
);

// =============================================================================
// Command

    assign  app_wdf_mask = {APP_MW{1'b0}};

    wire                    app_cmd_dir;
    wire    [7:0]           app_cmd_len;
    wire    [APP_AW-1:0]    app_cmd_addr;

    assign  app_cmd_dir  = d2u_rdata[DLA_DW-1];
    assign  app_cmd_len  = d2u_rdata[DLA_DW-3-:8] >> ($clog2(APP_DW/DLA_DW));
    assign  app_cmd_addr = d2u_rdata[APP_AW-1:0] << ($clog2(DLA_DW/DDR_DW));

// =============================================================================
// FSM - UI

    parameter       UI_IDLE = 4'b0001,
                    UI_WR_A = 4'b0010,
                    UI_WR_B = 4'b0100,
                    UI_RD   = 4'b1000;
    reg     [3:0]   ui_state;

    localparam  APP_CMD_READ  = 3'b1;
    localparam  APP_CMD_WRITE = 3'b0;

    reg     [7:0]   app_burst_len;
    reg     [7:0]   app_rvalid_cnt;

    always @(posedge ui_clk or posedge ui_rst) begin
        if (ui_rst) begin
            ui_state       <= UI_IDLE;
            app_addr       <= {APP_AW{1'b0}};
            app_cmd        <= APP_CMD_WRITE;
            app_en         <= 1'b0;
            app_wdf_data   <= {APP_DW{1'b0}};
            app_wdf_wren   <= 1'b0;
            app_wdf_end    <= 1'b0;
            u2d_wdata      <= {APP_DW{1'b0}};
            u2d_wen        <= 1'b0;
            app_burst_len  <= 8'b0;
            app_rvalid_cnt <= 8'b0;
        end
        else begin
            case (ui_state)
                UI_IDLE: begin
                    app_en       <= 1'b0;
                    app_wdf_wren <= 1'b0;
                    app_wdf_end  <= 1'b0;
                    u2d_wen      <= 1'b0;
                    if (~d2u_rempty) begin
                        app_addr       <= app_cmd_addr;
                        app_burst_len  <= app_cmd_len;
                        app_rvalid_cnt <= app_cmd_len;
                        if (app_cmd_dir) begin
                            ui_state <= UI_WR_A;
                            app_cmd  <= APP_CMD_WRITE;
                        end
                        else begin
                            ui_state <= UI_RD;
                            app_cmd  <= APP_CMD_READ;
                            app_en   <= 1'b1;
                        end
                    end
                end
                UI_WR_A: begin
                    if (~d2u_rempty) begin
                        ui_state     <= UI_WR_B;
                        app_en       <= 1'b1;
                        app_wdf_data <= d2u_rdata;
                        app_wdf_wren <= 1'b1;
                        app_wdf_end  <= 1'b1;
                    end
                end
                UI_WR_B: begin
                    if (app_rdy && app_wdf_rdy) begin
                        app_addr      <= app_addr + {{APP_AW-5{1'b0}}, 5'd8};    // BL8 Mode
                        app_burst_len <= app_burst_len - 1'b1;
                        if (app_burst_len == 8'b0) begin
                            ui_state     <= UI_IDLE;
                            app_en       <= 1'b0;
                            app_wdf_wren <= 1'b0;
                            app_wdf_end  <= 1'b0;
                            u2d_wen      <= 1'b1;
                            u2d_wdata    <= {{APP_DW{1'b0}}, 1'b1};
                        end
                        else if (~d2u_rempty) begin
                            app_en       <= 1'b1;
                            app_wdf_data <= d2u_rdata;
                            app_wdf_wren <= 1'b1;
                            app_wdf_end  <= 1'b1;
                        end
                        else begin
                            ui_state     <= UI_WR_A;
                            app_en       <= 1'b0;
                            app_wdf_wren <= 1'b0;
                            app_wdf_end  <= 1'b0;
                        end
                    end
                end
                UI_RD: begin
                    if (app_rdy) begin
                        app_addr <= app_addr + {{APP_AW-5{1'b0}}, 5'd8};   // BL8 Mode
                        if (app_burst_len == 8'b0) begin
                            app_en <= 1'b0;
                        end
                        else begin
                            app_burst_len <= app_burst_len - 8'b1;
                        end
                    end
                    if (app_rd_data_valid) begin
                        u2d_wdata      <= app_rd_data;
                        u2d_wen        <= 1'b1;
                        app_rvalid_cnt <= app_rvalid_cnt - 1'b1;
                        if (app_rvalid_cnt == 8'b0) begin
                            ui_state <= UI_IDLE;
                        end
                    end
                    else begin
                        u2d_wen <= 1'b0;
                    end
                end
                default: begin
                    ui_state <= UI_IDLE;
                end
            endcase
        end 
    end

    always @(*) begin
        if (~d2u_rempty) begin
            d2u_ren = ui_state == UI_IDLE || ui_state == UI_WR_A || 
                     (ui_state == UI_WR_B && app_rdy && app_wdf_rdy && app_burst_len != 8'b0);
        end
        else begin
            d2u_ren = 1'b0;
        end
    end

endmodule
