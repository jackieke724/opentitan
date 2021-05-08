module chip_host(
  input clk_i,
  input rst_ni,

  input   tlul_pkg::tl_h2d_t  tl_h2d,
  output  tlul_pkg::tl_d2h_t  tl_d2h,

  input idelay_ref_clk,
  input wire axi_c2c_phy_clk,
  output wire axi_c2c_selio_tx_diff_clk_out_p,
  output wire axi_c2c_selio_tx_diff_clk_out_n,
  output wire [30 : 0] axi_c2c_selio_tx_data_out,
  input wire axi_c2c_selio_rx_diff_clk_in_p,
  input wire axi_c2c_selio_rx_diff_clk_in_n,
  input wire [30 : 0] axi_c2c_selio_rx_data_in,
  output wire axi_c2c_link_status_out,
  output wire axi_c2c_multi_bit_error_out,
  output wire axi_c2c_link_error_out
);

logic   [7:0]               s_axi_awid;
logic   [31:0]              s_axi_awaddr;
logic   [7:0]               s_axi_awlen;
logic   [2:0]               s_axi_awsize;
logic   [1:0]               s_axi_awburst;
logic                       s_axi_awvalid;
logic                       s_axi_awready;
logic   [31:0]              s_axi_wdata;
logic   [3:0]               s_axi_wstrb;
logic                       s_axi_wlast;
logic                       s_axi_wvalid;
logic                       s_axi_wready;
logic   [7:0]               s_axi_bid;
logic   [1:0]               s_axi_bresp;
logic                       s_axi_bvalid;
logic                       s_axi_bready;
logic   [7:0]               s_axi_arid;
logic   [31:0]              s_axi_araddr;
logic   [7:0]               s_axi_arlen;
logic   [2:0]               s_axi_arsize;
logic   [1:0]               s_axi_arburst;
logic                       s_axi_arvalid;
logic                       s_axi_arready;
logic   [7:0]               s_axi_rid;
logic   [31:0]              s_axi_rdata;
logic   [1:0]               s_axi_rresp;
logic                       s_axi_rlast;
logic                       s_axi_rvalid;
logic                       s_axi_rready;

wire [3 : 0] axi_c2c_m2s_intr_in;
wire [3 : 0] axi_c2c_s2m_intr_out;

assign axi_c2c_m2s_intr_in = 4'b0;

axi_chip2chip_host axi_chip2chip_host_inst (
  .s_aclk(clk_i),                                            // input wire s_aclk
  .s_aresetn(rst_ni),                                      // input wire s_aresetn
  .s_axi_awid(s_axi_awid),                                    // input wire [7 : 0] s_axi_awid
  .s_axi_awaddr(s_axi_awaddr),                                // input wire [31 : 0] s_axi_awaddr
  .s_axi_awlen(s_axi_awlen),                                  // input wire [7 : 0] s_axi_awlen
  .s_axi_awsize(s_axi_awsize),                                // input wire [2 : 0] s_axi_awsize
  .s_axi_awburst(s_axi_awburst),                              // input wire [1 : 0] s_axi_awburst
  .s_axi_awvalid(s_axi_awvalid),                              // input wire s_axi_awvalid
  .s_axi_awready(s_axi_awready),                              // output wire s_axi_awready
  .s_axi_wdata(s_axi_wdata),                                  // input wire [31 : 0] s_axi_wdata
  .s_axi_wstrb(s_axi_wstrb),                                  // input wire [3 : 0] s_axi_wstrb
  .s_axi_wlast(s_axi_wlast),                                  // input wire s_axi_wlast
  .s_axi_wvalid(s_axi_wvalid),                                // input wire s_axi_wvalid
  .s_axi_wready(s_axi_wready),                                // output wire s_axi_wready
  .s_axi_bid(s_axi_bid),                                      // output wire [7 : 0] s_axi_bid
  .s_axi_bresp(s_axi_bresp),                                  // output wire [1 : 0] s_axi_bresp
  .s_axi_bvalid(s_axi_bvalid),                                // output wire s_axi_bvalid
  .s_axi_bready(s_axi_bready),                                // input wire s_axi_bready
  .s_axi_arid(s_axi_arid),                                    // input wire [7 : 0] s_axi_arid
  .s_axi_araddr(s_axi_araddr),                                // input wire [31 : 0] s_axi_araddr
  .s_axi_arlen(s_axi_arlen),                                  // input wire [7 : 0] s_axi_arlen
  .s_axi_arsize(s_axi_arsize),                                // input wire [2 : 0] s_axi_arsize
  .s_axi_arburst(s_axi_arburst),                              // input wire [1 : 0] s_axi_arburst
  .s_axi_arvalid(s_axi_arvalid),                              // input wire s_axi_arvalid
  .s_axi_arready(s_axi_arready),                              // output wire s_axi_arready
  .s_axi_rid(s_axi_rid),                                      // output wire [7 : 0] s_axi_rid
  .s_axi_rdata(s_axi_rdata),                                  // output wire [31 : 0] s_axi_rdata
  .s_axi_rresp(s_axi_rresp),                                  // output wire [1 : 0] s_axi_rresp
  .s_axi_rlast(s_axi_rlast),                                  // output wire s_axi_rlast
  .s_axi_rvalid(s_axi_rvalid),                                // output wire s_axi_rvalid
  .s_axi_rready(s_axi_rready),                                // input wire s_axi_rready
  .axi_c2c_m2s_intr_in(axi_c2c_m2s_intr_in),                  // input wire [3 : 0] axi_c2c_m2s_intr_in
  .axi_c2c_s2m_intr_out(axi_c2c_s2m_intr_out),                // output wire [3 : 0] axi_c2c_s2m_intr_out
  .idelay_ref_clk(idelay_ref_clk),                            // input wire idelay_ref_clk
  .axi_c2c_phy_clk(axi_c2c_phy_clk),                          // input wire axi_c2c_phy_clk
  .axi_c2c_selio_tx_data_out(axi_c2c_selio_tx_data_out),      // output wire [30 : 0] axi_c2c_selio_tx_data_out
  .axi_c2c_selio_rx_data_in(axi_c2c_selio_rx_data_in),        // input wire [30 : 0] axi_c2c_selio_rx_data_in
  .axi_c2c_selio_tx_diff_clk_out_p(axi_c2c_selio_tx_diff_clk_out_p),  // output wire axi_c2c_selio_tx_diff_clk_out_p
  .axi_c2c_selio_tx_diff_clk_out_n(axi_c2c_selio_tx_diff_clk_out_n),  // output wire axi_c2c_selio_tx_diff_clk_out_n
  .axi_c2c_selio_rx_diff_clk_in_p(axi_c2c_selio_rx_diff_clk_in_p),    // input wire axi_c2c_selio_rx_diff_clk_in_p
  .axi_c2c_selio_rx_diff_clk_in_n(axi_c2c_selio_rx_diff_clk_in_n),    // input wire axi_c2c_selio_rx_diff_clk_in_n
  .axi_c2c_link_status_out(axi_c2c_link_status_out),          // output wire axi_c2c_link_status_out
  .axi_c2c_multi_bit_error_out(axi_c2c_multi_bit_error_out),  // output wire axi_c2c_multi_bit_error_out
  .axi_c2c_link_error_out(axi_c2c_link_error_out)            // output wire axi_c2c_link_error_out
);


tlul2axi4_host tlul2axi4_host_inst(
  .clk_i,
  .rst_ni,

  .tl_h2d,
  .tl_d2h,

  .s_axi_awid,
  .s_axi_awaddr,
  .s_axi_awlen,
  .s_axi_awsize,
  .s_axi_awburst,
  .s_axi_awvalid,
  .s_axi_awready,
  .s_axi_wdata,
  .s_axi_wstrb,
  .s_axi_wlast,
  .s_axi_wvalid,
  .s_axi_wready,
  .s_axi_bid,
  .s_axi_bresp,
  .s_axi_bvalid,
  .s_axi_bready,
  .s_axi_arid,
  .s_axi_araddr,
  .s_axi_arlen,
  .s_axi_arsize,
  .s_axi_arburst,
  .s_axi_arvalid,
  .s_axi_arready,
  .s_axi_rid,
  .s_axi_rdata,
  .s_axi_rresp,
  .s_axi_rlast,
  .s_axi_rvalid,
  .s_axi_rready
);


endmodule