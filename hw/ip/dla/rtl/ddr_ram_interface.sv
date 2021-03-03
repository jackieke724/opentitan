module ddr_ram_interface#(
  parameter int WLEN    = 64,
  // Size of the data memory, in bytes
  parameter int DmemSizeByte = 4096,
  localparam int DmemAddrWidth = prim_util_pkg::vbits(DmemSizeByte)
) (
  input clk_i,
  input rst_ni,
	
	output logic                     dmem_req_o,
  output logic                     dmem_write_o,
  output logic [DmemAddrWidth-1:0] dmem_addr_o,
  output logic [WLEN-1:0]          dmem_wdata_o,
  output logic [WLEN-1:0]          dmem_wmask_o,
  input  logic [WLEN-1:0]          dmem_rdata_i,
  input  logic                     dmem_rvalid_i,
  input  logic                     dmem_rerror_i,
  
  input  logic [63:0]  						 ddr_mosi,
	output logic [63:0]  						 ddr_miso,
	input  logic             				 ddr_mosi_valid,
	output logic         						 ddr_miso_valid
);

	typedef enum logic [1:0] {
      StIdle,
      StMISO_send,
      StMISO,
      StMOSI         
    } st_e ;
    
  st_e st_q, st_d;
	
	logic [7:0] burst_cnt_d, burst_cnt_q;
	logic [7:0] rvalid_cnt_d, rvalid_cnt_q;
	logic [53:0] ram_addr_d, ram_addr_q;
	
	
	//Control
	always_ff @(posedge clk_i or negedge rst_ni) begin : state_ff
    if (!rst_ni) st_q <= StIdle;
    else         st_q <= st_d;
  end
  
  always_comb begin : next_state
    st_d = st_q;
		
		dmem_req_o = 0;
		dmem_write_o = 0;
		
		ddr_miso_valid = 0;
		
		burst_cnt_d = burst_cnt_q;
		ram_addr_d =  ram_addr_q;
		rvalid_cnt_d = rvalid_cnt_q; 
  
		unique case (st_q)
			StIdle: begin
				if (ddr_mosi_valid) begin
					burst_cnt_d = ddr_mosi[61:54];
					ram_addr_d = ddr_mosi[53:0];
					rvalid_cnt_d = ddr_mosi[61:54];
					
					if (ddr_mosi[63])
						st_d = StMOSI;
					else 
						st_d = StMISO_send;
				end
			end
			
			StMISO_send: begin
				st_d = StMISO;
				dmem_req_o = 1'b1;
				ram_addr_d  = ram_addr_q + 1'b1;
			end
				
			StMISO: begin
				if (burst_cnt_q == 8'b0) begin
					dmem_req_o = 1'b0;
				end
				else begin
					dmem_req_o = 1'b1;
					burst_cnt_d = burst_cnt_q - 1'b1;
					ram_addr_d  = ram_addr_q + 1'b1;
				end
				
				if (dmem_rvalid_i) begin
					ddr_miso_valid = 1'b1;
					rvalid_cnt_d = rvalid_cnt_q - 1'b1;
					if (rvalid_cnt_q == 8'd0) begin
						st_d = StIdle;
					end
				end
			end
			
				
			StMOSI: begin
				if (ddr_mosi_valid) begin
					dmem_req_o = 1'b1;
					dmem_write_o = 1'b1;
					ram_addr_d = ram_addr_q + 1'b1;
					if (burst_cnt_q == 8'b0) begin
						st_d = StIdle;
						ddr_miso_valid = 1'b1;
					end
					else
						burst_cnt_d = burst_cnt_q - 1'b1;
				end
			end
			
			default: begin
        st_d = StIdle;
      end
    endcase
  end
	
	
	//Datapath
	assign dmem_addr_o = ram_addr_q << 3; //double-word-addressable
	assign dmem_wdata_o = ddr_mosi;
	assign dmem_wmask_o = {WLEN{1'b1}}; //mask not used
	assign ddr_miso = dmem_rdata_i;
	
	
	always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      rvalid_cnt_q <= 8'b0;
      burst_cnt_q <= 8'b0;
      ram_addr_q <= 54'b0;
    end else begin
      rvalid_cnt_q <= rvalid_cnt_d;
      burst_cnt_q <= burst_cnt_d;
      ram_addr_q <= ram_addr_d;
    end
  end

	
endmodule : ddr_ram_interface
