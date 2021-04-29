// +FHDR========================================================================
//  License:
//      Copyright (c) 2019 Authors and BCRC. All rights reserved.
// =============================================================================
//  File Name:      ddrif_dla.v
//                  Shiwei Liu (18212020026@fudan.edu.com)
//  Organization:   Brain-Chip Research Center (BCRC), Fudan University
//  Description:
//      DDR Interface - DLA to UI FIFO.
// -FHDR========================================================================

`resetall

//`include "INC_global.v"

module ddrif_dla #(
    parameter DLA_DW = 64,
    parameter APP_DW = 512 
)(
    // Global Signals
    input               clk,
    input               rst,

    // Interface - DLA <-> DDR
    input       [DLA_DW-1:0]    ddr_mosi,       // DLA -> DDR
    output      [DLA_DW-1:0]    ddr_miso,       // DLA <- DDR
    input                       ddr_mosi_valid, // DLA -> DDR, DDR WDATA Valid
    output  reg                 ddr_miso_valid, // DLA <- DDR, DDR RDATA Valid

    // Interface - UI FIFO -> DLA
    input       [APP_DW-1:0]    u2d_rdata,
    output                      u2d_ren,
    input                       u2d_rempty,

    // Interface - UI FIFO <- DLA
    output  reg [APP_DW-1:0]    d2u_wdata,
    output  reg                 d2u_wen,
    input                       d2u_wfull
);
    // APP_DW = 512, DLA_DW = 64, DLA2APP_CNT_DW = 3
    localparam  DLA2APP_CNT_DW = $clog2(APP_DW/DLA_DW); // needs to be power of 2

    reg     [DLA2APP_CNT_DW-1:0]    dla2app_cnt;

// =============================================================================
// DDR Command Recieve

    reg     [DLA_DW-1:0]    mosi_buf;
    reg                     mosi_valid_buf;

    wire            mosi_cmd_dir;
    wire    [7:0]   mosi_cmd_len;

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            mosi_buf       <= 64'b0;
            mosi_valid_buf <= 1'b0;
        end
        else begin
            mosi_buf       <= ddr_mosi;
            mosi_valid_buf <= ddr_mosi_valid;
        end
    end

    assign  mosi_cmd_dir  = mosi_buf[DLA_DW-1];
    assign  mosi_cmd_len  = mosi_buf[DLA_DW-3-:8];

// =============================================================================
// FSM - DLA <-> UI FIFO

    parameter       DDRS_IDLE   = 5'b00001,
                    DDRS_RD_FCH = 5'b00010, // Read  DDR - Fetch Data
                    DDRS_RD_ISS = 5'b00100, // Read  DDR - Issue
                    DDRS_WR_RCV = 5'b01000, // Write DDR - Recieve
                    DDRS_WR_ACK = 5'b10000; // Write DDR - Acknowledge
    reg     [4:0]   ddrs_state;             // DDR Slave State

    reg     [7:0]   burst_len;
    reg             miso_wack_valid;

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            ddrs_state      <= DDRS_IDLE;
            burst_len       <= 8'b0;
            d2u_wen         <= 1'b0;
            d2u_wdata       <= {APP_DW{1'b0}};
            dla2app_cnt     <= {DLA2APP_CNT_DW{1'b0}};
            miso_wack_valid <= 1'b0;
        end
        else begin
            case (ddrs_state)
                DDRS_IDLE: begin
                    miso_wack_valid <= 1'b0;
                    if (mosi_valid_buf) begin
                        ddrs_state            <= mosi_cmd_dir ? DDRS_WR_RCV : DDRS_RD_FCH;
                        burst_len             <= mosi_cmd_len;
                        d2u_wen               <= 1'b1;
                        d2u_wdata[DLA_DW-1:0] <= mosi_buf;
                        dla2app_cnt           <= {DLA2APP_CNT_DW{1'b0}};
                    end
                end
                DDRS_RD_FCH: begin
                    d2u_wen <= 1'b0;
                    if (~u2d_rempty) begin
                        burst_len   <= burst_len - 1'b1;
                        dla2app_cnt <= {{DLA2APP_CNT_DW-1{1'b0}}, 1'b1};
                        if (APP_DW == DLA_DW) begin
                            ddrs_state <= burst_len == 8'b0 ? DDRS_IDLE : DDRS_RD_FCH;
                        end
                        else begin
                            ddrs_state <= DDRS_RD_ISS;
                        end
                    end
                end
                DDRS_RD_ISS: begin
                    dla2app_cnt <= dla2app_cnt + 1'b1;
                    burst_len   <= burst_len - 1'b1;
                    if (&dla2app_cnt) begin
                        ddrs_state <= burst_len == 8'b0 ? DDRS_IDLE : DDRS_RD_FCH;
                    end
                end
                DDRS_WR_RCV: begin
                    if (mosi_valid_buf) begin
                        if (burst_len == 8'b0) begin
                            ddrs_state <= DDRS_WR_ACK;
                        end
                        else begin
                            burst_len <= burst_len - 1'b1;
                        end
                        dla2app_cnt <= dla2app_cnt + 1'b1;
                        d2u_wen     <= & dla2app_cnt;
                        if (APP_DW != DLA_DW) begin
                            d2u_wdata   <= {mosi_buf, d2u_wdata[APP_DW-1:DLA_DW]};
                        end
                        else begin
                            d2u_wen   <= 1'b1;
                            d2u_wdata <= mosi_buf;
                        end
                    end
                    else begin
                        d2u_wen <= 1'b0;
                    end
                end
                DDRS_WR_ACK: begin
                    d2u_wen <= 1'b0;
                    if (~u2d_rempty) begin
                        ddrs_state      <= DDRS_IDLE;
                        miso_wack_valid <= 1'b1;
                    end
                end
                default: begin
                    ddrs_state <= DDRS_IDLE;
                end
            endcase
        end
    end

// =============================================================================
// DDR Read Channel

    reg     [APP_DW-1:0]    miso_rdata;
    reg                     miso_rdata_valid;
    reg                     u2d_rvalid;

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            miso_rdata       <= {APP_DW{1'b0}};
            miso_rdata_valid <= 1'b0;
            u2d_rvalid       <= 1'b0;
        end
        else begin
            u2d_rvalid <= u2d_ren;
            miso_rdata_valid <= u2d_ren || ddrs_state == DDRS_RD_ISS;
            if (miso_rdata_valid) begin
                if (u2d_rvalid) begin
                    miso_rdata <= u2d_rdata;
                end
                else begin
                    miso_rdata <= miso_rdata >> DLA_DW;
                end
            end
        end
    end

    assign  ddr_miso = miso_rdata[DLA_DW-1:0];

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            ddr_miso_valid <= 1'b0;
        end
        else begin
            ddr_miso_valid <= miso_rdata_valid || miso_wack_valid;
        end
    end

    assign  u2d_ren = ~u2d_rempty && (ddrs_state == DDRS_WR_ACK || ddrs_state == DDRS_RD_FCH);

endmodule
