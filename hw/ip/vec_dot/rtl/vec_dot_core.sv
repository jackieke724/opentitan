module vec_dot_core #(
  parameter int VEC_LEN = 4
) (
  input clk_i,
  input rst_ni,
  input [31:0] data_i, 
  input wr_en_i, 
  input start_i,
  output logic busy_o,
  output logic [31:0] result_o,

  output logic done_o    
);

  logic [31:0] vector1 [VEC_LEN-1:0];
  logic [31:0] vector2 [VEC_LEN-1:0];
  logic [31:0] addr;
  logic [31:0] vec1_rd_data, vec2_rd_data;
  logic [31:0] sum, product;
	logic [31:0] result;
  logic busy, done, mac_run, mac_reset, addr_add, addr_clear, vec_req, vec1_wr, vec2_wr, rd_en, write_result;
  
  typedef enum logic [2:0] {
      StIdle,
      StWriteV1,
      StWriteV2,           
      StRead,             //Comp as well
      StComp,             //Remaining computations         
      StComp2,             //Remaining computations         
      StSave,
      StDone              
    } st_e ;
    
  st_e st_q, st_d;

	assign busy_o = busy;
  assign done_o = done;
  assign result_o = result;
   
  
  //2 vector rams
  always_ff @(posedge clk_i) begin
    if (vec_req) begin
      if (vec1_wr)
        vector1[addr] = data_i;
      else
        vec1_rd_data <= vector1[addr];
    end
    
    if (vec_req) begin
      if (vec2_wr)
        vector2[addr] = data_i;
      else
        vec2_rd_data <= vector2[addr];
    end
  end
  
  always_ff @(posedge clk_i or negedge rst_ni) begin : state_ff
    if (!rst_ni) st_q <= StIdle;
    else         st_q <= st_d;
  end
  
  always_comb begin : next_state
    st_d = st_q;
    
    busy = 1'b0;
    done = 1'b0;
    mac_run = 1'b0;
		mac_reset = 1'b0;
    addr_add = 1'b0;
    addr_clear = 1'b0;
    vec_req = 1'b0;
    vec1_wr = 1'b0;
    vec2_wr = 1'b0;
    rd_en = 1'b0;
    write_result = 1'b0;
    
    unique case (st_q)
      StIdle: begin
        if (wr_en_i) begin
          st_d = StWriteV1;
          vec_req = 1'b1;
          vec1_wr = 1'b1;
          addr_add = 1'b1;
        end
        
        if (start_i) begin
          st_d = StRead;
					vec_req = 1'b1;
					rd_en = 1'b1;
          addr_add = 1'b1;
        end
      end  
      
      StWriteV1: begin
        busy = 1'b1;
        
				if (wr_en_i) begin
					vec_req = 1'b1;
					vec1_wr = 1'b1;
					addr_add = 1'b1;
					
					if (addr == VEC_LEN-1) begin
						st_d = StWriteV2;
						addr_clear = 1'b1;
					end
				end
      end
      
      StWriteV2: begin
        busy = 1'b1;
				if (wr_en_i) begin
					vec_req = 1'b1;
					vec2_wr = 1'b1;
					addr_add = 1'b1;
					
					if (addr == VEC_LEN-1) begin
						addr_clear = 1'b1;
						st_d = StIdle;
					end
				end
      end
      
      StRead: begin
        busy = 1'b1;
        mac_run = 1'b1;
				vec_req = 1'b1;
        rd_en = 1'b1;
        addr_add = 1'b1;
        if (addr == VEC_LEN-1) begin
          addr_clear = 1'b1;
          st_d = StComp;
        end
      end     
      
      StComp: begin
        busy = 1'b1;
        mac_run = 1'b1;
        st_d = StComp2;
      end
			
			StComp2: begin
        busy = 1'b1;
        mac_run = 1'b1;
        st_d = StSave;
      end
      
      StSave: begin
				busy = 1'b1;
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
    if (!rst_ni) begin
      addr <= 0;
    end else begin
    
      if (addr_clear)
        addr <= 0;
      else if (addr_add)
        addr <= addr + 1'b1;
    end
  end

  //mac compute unit
  always_ff @(posedge clk_i or negedge rst_ni)
  begin
    if (!rst_ni | mac_reset) begin
      sum <= 0;
      product <= 0;
    end else if (mac_run) begin
      product <= vec1_rd_data * vec2_rd_data;
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
