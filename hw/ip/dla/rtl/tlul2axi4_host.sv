module tlul2axi4_host import tlul_pkg::*;
(
  input clk_i,
  input rst_ni,

  input   tlul_pkg::tl_h2d_t  tl_h2d,
  output  tlul_pkg::tl_d2h_t  tl_d2h,

  output  logic   [7:0]               s_axi_awid,
  output  logic   [31:0]              s_axi_awaddr,
  output  logic   [7:0]               s_axi_awlen,
  output  logic   [2:0]               s_axi_awsize,
  output  logic   [1:0]               s_axi_awburst,
  output  logic                       s_axi_awvalid,
  input   logic                       s_axi_awready,
  output  logic   [31:0]              s_axi_wdata,
  output  logic   [3:0]               s_axi_wstrb,
  output  logic                       s_axi_wlast,
  output  logic                       s_axi_wvalid,
  input   logic                       s_axi_wready,
  input   logic   [7:0]               s_axi_bid,
  input   logic   [1:0]               s_axi_bresp,
  input   logic                       s_axi_bvalid,
  output  logic                       s_axi_bready,
  output  logic   [7:0]               s_axi_arid,
  output  logic   [31:0]              s_axi_araddr,
  output  logic   [7:0]               s_axi_arlen,
  output  logic   [2:0]               s_axi_arsize,
  output  logic   [1:0]               s_axi_arburst,
  output  logic                       s_axi_arvalid,
  input   logic                       s_axi_arready,
  input   logic   [7:0]               s_axi_rid,
  input   logic   [31:0]              s_axi_rdata,
  input   logic   [1:0]               s_axi_rresp,
  input   logic                       s_axi_rlast,
  input   logic                       s_axi_rvalid,
  output  logic                       s_axi_rready
);

  logic write, read;

  always_comb begin
    write = 1'b0;
    read  = 1'b0;
    case (tl_h2d.a_opcode)
      PutFullData: begin
        write = tl_h2d.a_valid;
      end

      PutPartialData: begin
        write = tl_h2d.a_valid;
      end

      Get: begin
        read = tl_h2d.a_valid;
      end

      default: begin
        write = 1'b0;
        read  = 1'b0;
      end
    endcase
  end
  
  //axi write address channel
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      s_axi_awid    <= '0;
      s_axi_awaddr  <= '0;
      s_axi_awlen   <= '0;
      s_axi_awsize  <= '0;
      s_axi_awburst <= '0;
      s_axi_awvalid <= '0;
    end else begin
      if (write & s_axi_awready & s_axi_wready) begin
        s_axi_awid    <= tl_h2d.a_source;
        s_axi_awaddr  <= tl_h2d.a_address;
        s_axi_awlen   <= {'0, 1'b1};
        s_axi_awsize  <= {'0, tl_h2d.a_size};
        s_axi_awburst <= '0;
        s_axi_awvalid <= 1'b1;
      end
      else
        s_axi_awvalid <= 1'b0;
    end
  end

  /*logic [31:0] a_data_bitmask = { {8{tl_h2d.a_mask[3]}},
                                  {8{tl_h2d.a_mask[2]}},
                                  {8{tl_h2d.a_mask[1]}},
                                  {8{tl_h2d.a_mask[0]}} };*/

  //axi write data channel
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      s_axi_wdata   <= '0;
      s_axi_wstrb   <= '0;
      s_axi_wlast   <= '0;
      s_axi_wvalid  <= '0;
    end
    else begin
      if (write & s_axi_awready & s_axi_wready) begin
        s_axi_wdata   <= tl_h2d.a_data;
        s_axi_wstrb   <= tl_h2d.a_mask;
        s_axi_wlast   <= 1'b1;
        s_axi_wvalid  <= 1'b1;
      end
      else
        s_axi_wvalid  <= 1'b0;
    end
  end

  //axi write response channel
  //assign s_axi_bready = tl_h2d.d_ready;


  //axi read address channel
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      s_axi_arid    <= '0;
      s_axi_araddr  <= '0;
      s_axi_arlen   <= '0;
      s_axi_arsize  <= '0;
      s_axi_arburst <= '0;
      s_axi_arvalid <= '0;
    end else begin
      if (read & s_axi_arready) begin
        s_axi_arid    <= tl_h2d.a_source;
        s_axi_araddr  <= tl_h2d.a_address;
        s_axi_arlen   <= {'0, 1'b1};;
        s_axi_arsize  <= {'0, tl_h2d.a_size};
        s_axi_arburst <= '0;
        s_axi_arvalid <= 1'b1;
      end
      else
        s_axi_arvalid <= 1'b0;
    end
  end

  //axi read data channel
  //prioritize write ack over read data when they happen together
  //assign s_axi_rready = tl_h2d.d_ready & ~s_axi_bvalid; 


  //tl_d2h
  typedef enum logic [1:0] {
    StIdle,
    StAccAck,
    StAccData
  } st_e ;
    
  st_e st_q, st_d;
  //Control
  always_ff @(posedge clk_i or negedge rst_ni) begin : state_ff
    if (!rst_ni) st_q <= StIdle;
    else         st_q <= st_d;
  end
  
  always_comb begin : next_state
    st_d = st_q;

    s_axi_bready      = 1'b0;
    s_axi_rready      = 1'b0;

    tl_d2h.d_valid    = '0;
    tl_d2h.d_opcode   = AccessAck;
    tl_d2h.d_size     = '0;
    tl_d2h.d_source   = '0;
    tl_d2h.d_sink     = '0;
    tl_d2h.d_data     = '0;
    tl_d2h.d_error    = '0;

    unique case (st_q)
      StIdle: begin
        if (s_axi_bvalid) begin
          st_d = StAccAck;
        end

        //prioritize write over read when they happen together
        else if (s_axi_rvalid) begin
          st_d = StAccData;
        end
      end

      StAccAck: begin
        if (tl_h2d.d_ready) begin
          st_d = StIdle;
          s_axi_bready    = 1'b1;
        end
        tl_d2h.d_valid    = 1'b1;
        tl_d2h.d_opcode   = AccessAck;
        tl_d2h.d_size     = 2'h2;
        tl_d2h.d_source   = s_axi_bid;
        tl_d2h.d_sink     = '0;
        tl_d2h.d_data     = '0;
        tl_d2h.d_error    = s_axi_bresp[1];
      end

      StAccData: begin
        if (tl_h2d.d_ready) begin
          st_d = StIdle;
          s_axi_rready    = 1'b1;
        end
        tl_d2h.d_valid    = 1'b1;
        tl_d2h.d_opcode   = AccessAckData;
        tl_d2h.d_size     = 2'h2;
        tl_d2h.d_source   = s_axi_rid;
        tl_d2h.d_sink     = '0;
        tl_d2h.d_data     = s_axi_rdata;
        tl_d2h.d_error    = s_axi_rresp[1];
      end

      default: begin
        st_d = StIdle;
      end
    endcase
  end

  assign tl_d2h.d_param = '0;
  assign tl_d2h.d_user  = '0;
  assign tl_d2h.a_ready = (write & s_axi_awready & s_axi_wready) | (read & s_axi_arready);

  /*
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      tl_d2h.d_valid  <= '0;
      tl_d2h.d_opcode <= AccessAck;
      tl_d2h.d_size   <= '0;
      tl_d2h.d_source <= '0;
      tl_d2h.d_sink   <= '0;
      tl_d2h.d_data   <= '0;
      tl_d2h.d_error  <= '0;
    end
    else begin
      //prioritize write ack over read data when they happen together
      if (s_axi_bvalid & tl_h2d.d_ready) begin
        tl_d2h.d_valid  <= 1'b1;
        tl_d2h.d_opcode <= AccessAck;
        tl_d2h.d_size   <= 2'h2;
        tl_d2h.d_source <= s_axi_bid;
        tl_d2h.d_sink   <= '0;
        tl_d2h.d_data   <= '0;
        tl_d2h.d_error  <= s_axi_bresp[1];
      end

      else if (s_axi_rvalid & tl_h2d.d_ready) begin
        tl_d2h.d_valid <= 1'b1;
        tl_d2h.d_opcode <= AccessAckData;
        tl_d2h.d_size <= 2'h2;
        tl_d2h.d_source <= s_axi_rid;
        tl_d2h.d_sink <= '0;
        tl_d2h.d_data <= s_axi_rdata;
        tl_d2h.d_error <= s_axi_rresp[1];
      end

      else begin
        tl_d2h.d_valid <= '0;
      end
    end
  end
  */

endmodule