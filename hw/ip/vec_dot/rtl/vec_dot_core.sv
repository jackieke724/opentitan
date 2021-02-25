module vec_dot_core #(
  parameter int VEC_LEN = 4,
  parameter int WLEN    = 64,
  
  // Size of the data memory, in bytes
  parameter int DmemSizeByte = 4096,
  localparam int DmemAddrWidth = prim_util_pkg::vbits(DmemSizeByte)
) (
  input clk_i,
  input rst_ni,
  input [31:0] data_i, 
  input wr_en_i, 
  input mode_i,
  input start_i,
  output logic [31:0] result_o,

  output logic done_o,
  
  // Data memory (DMEM)
  output logic                     dmem_req_o,
  output logic                     dmem_write_o,
  output logic [DmemAddrWidth-1:0] dmem_addr_o,
  output logic [WLEN-1:0]          dmem_wdata_o,
  output logic [WLEN-1:0]          dmem_wmask_o,
  input  logic [WLEN-1:0]          dmem_rdata_i,
  input  logic                     dmem_rvalid_i,
  input  logic                     dmem_rerror_i
);

  logic [31:0] vector1 [VEC_LEN-1:0];
  logic [31:0] vector2 [VEC_LEN-1:0];
  logic [31:0] addr_q, addr_d, vec_addr;
  logic [31:0] vec1_rd_data, vec2_rd_data;
  logic [31:0] sum, product;
	logic [31:0] result;
  logic done, mac_run, mac_reset, addr_clear, vec_req, vec1_wr, vec2_wr, write_result;
  
  typedef enum logic [2:0] {
      StIdle,
      StWriteV1,
      StWriteV2,
			StBusyWait,					//wait 1 cycle for the dmem access
      StRead,             //Comp as well
      StComp,             //Add the last product         
      StSave,
      StDone              
    } st_e ;
    
  st_e st_q, st_d;

  assign done_o = done;
  assign result_o = result;
  assign dmem_addr_o = addr_q;
  assign dmem_write_o = 1'b0;
  assign dmem_wdata_o = 0;
  assign dmem_wmask_o = 0;
  
  assign vec_addr = addr_q >> 2;//(32b) word-addresable
  //2 vector rams
  always_ff @(posedge clk_i) begin
    if (vec_req) begin
      if (vec1_wr)
        vector1[vec_addr] = data_i;
      else
        vec1_rd_data <= vector1[vec_addr];
    end
    
    if (vec_req) begin
      if (vec2_wr)
        vector2[vec_addr] = data_i;
      else
        vec2_rd_data <= vector2[vec_addr];
    end
  end
  
  always_ff @(posedge clk_i or negedge rst_ni) begin : state_ff
    if (!rst_ni) st_q <= StIdle;
    else         st_q <= st_d;
  end
  
  always_comb begin : next_state
    st_d = st_q;
    
    done = 1'b0;
    mac_run = 1'b0;
		mac_reset = 1'b0;
    addr_clear = 1'b0;
		addr_d = addr_q;
    dmem_req_o = 1'b0;
    vec_req = 1'b0;
    vec1_wr = 1'b0;
    vec2_wr = 1'b0;
    write_result = 1'b0;
    
    unique case (st_q)
      StIdle: begin
        if (wr_en_i) begin
          st_d = StWriteV1;
          vec_req = 1'b1;
          vec1_wr = 1'b1;
					addr_d = addr_q + 32'd4;
        end
        
        if (start_i) begin
            st_d = StBusyWait;
        end
      end  
      
      StWriteV1: begin
				if (wr_en_i) begin
					vec_req = 1'b1;
					vec1_wr = 1'b1;
					addr_d = addr_q + 32'd4;
					
					if (addr_q == (VEC_LEN-1)<<2) begin
						st_d = StWriteV2;
						addr_clear = 1'b1;
					end
				end
      end
      
      StWriteV2: begin
				if (wr_en_i) begin
					vec_req = 1'b1;
					vec2_wr = 1'b1;
					addr_d = addr_q + 32'd4;
					
					if (addr_q == (VEC_LEN-1)<<2) begin
						addr_clear = 1'b1;
						st_d = StIdle;
					end
				end
      end
			
			StBusyWait: begin
				vec_req = ~mode_i;
				dmem_req_o = mode_i;
				st_d = StRead;
				
				if (mode_i)
					addr_d = addr_q + 32'd8;
				else
					addr_d = addr_q + 32'd4;
			end
      
      StRead: begin
        if (~mode_i | dmem_rvalid_i) begin
          mac_run = 1'b1;
				  vec_req = ~mode_i;
				  dmem_req_o = mode_i;
					
					if (mode_i) begin
						addr_d = addr_q + 32'd8;
						if (addr_q == VEC_LEN<<3) begin
							vec_req = 1'b0;
							dmem_req_o = 1'b0;
							addr_clear = 1'b1;
							st_d = StComp;
						end
					end
					else begin
						addr_d = addr_q + 32'd4;
						if (addr_q == VEC_LEN<<2) begin
							vec_req = 1'b0;
							dmem_req_o = 1'b0;
							addr_clear = 1'b1;
							st_d = StComp;
						end
					end
          
        end
      end     
      
      StComp: begin
        mac_run = 1'b1;
        st_d = StSave;
      end
			
      StSave: begin
				write_result = 1'b1;
        st_d = StDone;
      end 
      
      StDone: begin
        done = 1'b1;
	      mac_reset = 1'b1;
        st_d = StIdle;
      end
    
      default: begin
        st_d = StIdle;
      end
    endcase
  end
  
  
  //Datapath
  //addr counter
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni | addr_clear) begin
      addr_q <= 0;
    end else begin
        addr_q <= addr_d; //byte-addressable
    end
  end

  logic [31:0]  mac_in1, mac_in2;
	assign mac_in1 = mode_i ?dmem_rdata_i[31:0]   :vec1_rd_data;
	assign mac_in2 = mode_i ?dmem_rdata_i[63:32]  :vec2_rd_data;
	
  //mac compute unit
  always_ff @(posedge clk_i or negedge rst_ni)
  begin
    if (!rst_ni | mac_reset) begin
      sum <= 0;
      product <= 0;
    end else if (mac_run) begin
      product <= mac_in1 * mac_in2;
      sum <= sum + product;
    end
  end
  
  //result reg
  always_ff @(posedge clk_i or negedge rst_ni)
  begin
    if (!rst_ni) begin
      result <= 0;
    end else if (write_result) begin
      result <= sum;
    end
  end

endmodule : vec_dot_core
