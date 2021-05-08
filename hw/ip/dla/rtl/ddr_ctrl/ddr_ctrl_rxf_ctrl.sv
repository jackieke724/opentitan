module ddr_ctrl_rxf_ctrl #(
  parameter int WLEN    = 64,
  // Size of the data memory, in bytes
  parameter int DmemSizeByte = 4096,
  localparam int DmemAddrWidth = prim_util_pkg::vbits(DmemSizeByte)
) (
  input clk_i,
  input rst_ni,
	
	output logic [DmemAddrWidth:0]		wptr,
	
	input logic [63:0]  							ddr_miso,
	input logic         							ddr_miso_valid,
	
	// Data memory (DMEM)
  output logic                     	dmem_req_o,
  output logic                     	dmem_write_o,
  output logic [DmemAddrWidth-1:0] 	dmem_addr_o, //byte-addressable
  output logic [WLEN-1:0]          	dmem_wdata_o,
  output logic [WLEN-1:0]          	dmem_wmask_o,
  input  logic [WLEN-1:0]          	dmem_rdata_i,
  input  logic                     	dmem_rvalid_i,
  input  logic                     	dmem_rerror_i
);

	//logic [DmemAddrWidth:0] wptr; //MSB is phase--wrapped around relative to rptr

	assign dmem_req_o = ddr_miso_valid;
	assign dmem_write_o = 1'b1;
	assign dmem_addr_o = wptr[DmemAddrWidth-1:0];
	assign dmem_wdata_o = ddr_miso;
	assign dmem_wmask_o = '1;
	
	// Write pointer update
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      wptr <= '0;
    end else if (ddr_miso_valid) begin
			
			/*
			if (wptr[DmemAddrWidth-1:0] == DmemSizeByte) begin
				wptr[DmemAddrWidth] <= ~wptr[DmemAddrWidth-1];
				wptr[DmemAddrWidth-1:0] <= '0;
			end else begin
				wptr[DmemAddrWidth-1:3] <= wptr[DmemAddrWidth-1:3] + 1'b1;
			end
			*/
			wptr[DmemAddrWidth:3] <= wptr[DmemAddrWidth:3] + 1'b1;
    end
  end
	
endmodule
