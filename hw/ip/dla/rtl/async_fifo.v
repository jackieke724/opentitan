`resetall

`include "INC_global.v"

module async_fifo
#(
    parameter DW = 64,
    parameter AW = 3,
    parameter FWFT = 0 // First Word Fall Through
)(
    input                   wclk,
    input                   wrst,
    input                    wen,
    input        [DW-1:0]   wdata,
    output  reg             wfull,

    input                   rclk,
    input                   rrst,
    input                   ren,
    output  reg  [DW-1:0]   rdata,
    output  reg             rempty
);

    localparam DEPTH = 1 << AW;

    reg     [DW-1:0]    mem [0:DEPTH-1];

    wire    [AW-1:0]    raddr;
    wire    [AW-1:0]    waddr;
    reg     [AW:0]      rptr, rq1_wptr, rq2_wptr;
    reg     [AW:0]      wptr, wq1_rptr, wq2_rptr;

    reg     [AW:0]      rbin;
    wire    [AW:0]      rgraynext, rbinnext;
    reg     [AW:0]      wbin;
    wire    [AW:0]      wgraynext, wbinnext;

    wire                wfull_val;

    always @ (posedge wclk or posedge wrst) begin
        if (wrst) {wq2_rptr, wq1_rptr} <= 8'b0;
        else      {wq2_rptr, wq1_rptr} <= {wq1_rptr, rptr};
    end

    always @ (posedge rclk or posedge rrst) begin
        if (rrst) {rq2_wptr, rq1_wptr} <= 8'b0;
        else      {rq2_wptr, rq1_wptr} <= {rq1_wptr, wptr};
    end

    always @ (posedge rclk or posedge rrst) begin
        if (rrst) rdata <= 64'b0;
        else if (ren && ! rempty) rdata <= mem[raddr];
    end

    always @ (posedge wclk) begin
        if (wen && ! wfull) mem[waddr] <= wdata;
    end

    // gray code pointer
    always @ (posedge rclk or posedge rrst) begin
        if (rrst) {rbin, rptr} <= 8'b0;
        else      {rbin, rptr} <= {rbinnext, rgraynext};
    end

    // Memory read-address pointer (okay to use binary to address memory)
    assign raddr     = rbin[AW-1:0];
    assign rbinnext  = rbin + (ren & ~ rempty);
    assign rgraynext = (rbinnext >> 1) ^ rbinnext;

    always @ (posedge rclk or posedge rrst) begin
        if (rrst) rempty <= 1'b1;
        else      rempty <= rgraynext == rq2_wptr;
    end

    // gray code pointer
    always @ (posedge wclk or posedge wrst) begin
        if (wrst) {wbin, wptr} <= 8'b0;
        else      {wbin, wptr} <= {wbinnext, wgraynext};
    end

    // Memory write-address pointer (okay to use binary to address memory)
    assign waddr     = wbin[AW-1:0];
    assign wbinnext  = wbin + (wen & ~ wfull);
    assign wgraynext = (wbinnext >> 1) ^ wbinnext;

    //------------------------------------------------------------------
    // Simplified version of the three necessary full-tests:
    // assign wfull_val = ((wgnext[AW]     != wq2_rptr[AW])
    //                  && (wgnext[AW-1]   != wq2_rptr[AW-1])
    //                  && (wgnext[AW-2:0] == wq2_rptr[AW-2:0]));
    //------------------------------------------------------------------
    assign wfull_val = wgraynext == {~ wq2_rptr[AW:AW-1], wq2_rptr[AW-2:0]};

    always @ (posedge wclk or posedge wrst) begin
        if (wrst) wfull <= 1'b0;
        else      wfull <= wfull_val;
    end

endmodule
