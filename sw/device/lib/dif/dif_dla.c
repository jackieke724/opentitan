#include "sw/device/lib/dif/dif_dla.h"
#include "sw/device/lib/runtime/log.h" //for debugging only

//#include "sw/device/lib/base/bitfield.h"

#include "dla_regs.h"  // Generated.


dif_dla_result_t dif_dla_reset(const dif_dla_t *dla) {
  if (dla == NULL) {
    return kDifDlaBadArg;
  }

  mmio_region_write32(dla->params.base_addr, DLA_INTR_ENABLE_REG_OFFSET, 0);

  // Clear all pending interrupts.
  mmio_region_write32(dla->params.base_addr, DLA_INTR_STATE_REG_OFFSET, 0xFFFFFFFF);

  return kDifDlaOk;
}


dif_dla_result_t dif_dla_init(dif_dla_params_t params,
                                dif_dla_t *dla) {
  if (dla == NULL) {
    return kDifDlaBadArg;
  }

  dla->params = params;
  dif_dla_reset(dla);

  return kDifDlaOk;
}


dif_dla_result_t dif_dla_init_ddr(const dif_dla_t *dla) {
  if (dla == NULL ) {
    return kDifDlaBadArg;
  }
  
  //initialize DMEM
  uint32_t src[66];
  //feature
  for (uint32_t i=0; i<16; i++) { //0-31
    src[i*2] = i;
    src[i*2+1] = 0;
  }
  //weight
  for (uint32_t i=0; i<32; i++) { //32-63
    src[32+i] = 0;
  }
  src[32]=1;
  src[33]=0;
  src[34]=1;
  src[35]=0;
  src[36]=0;
  src[37]=0;
  //bias
  src[64]=0x45a6;
  src[65]=0;
  
  mmio_region_memcpy_to_mmio32(dla->params.base_addr, 
                                  DLA_DMEM_REG_OFFSET,
                                  (void*)src, 
                                  4*66);
  
  return kDifDlaOk;
}


dif_dla_result_t dif_dla_wait_for_done(const dif_dla_t *dla) {
  if (dla == NULL ) {
    return kDifDlaBadArg;
  }

  uint32_t done = 0;
  while (done==0){
    done = mmio_region_read32(dla->params.base_addr, DLA_GST_INTR_REG_OFFSET);
  }
  mmio_region_write32(dla->params.base_addr, DLA_GST_INTR_REG_OFFSET, 0);
 
  return kDifDlaOk;
}


dif_dla_result_t dif_dla_set_ppe(const dif_dla_t *dla, uint32_t row_en, uint32_t col_en) {
  if (dla == NULL ) {
    return kDifDlaBadArg;
  }

  mmio_region_write32(dla->params.base_addr, DLA_GST_ENABLE_ROW_REG_OFFSET, row_en);
  mmio_region_write32(dla->params.base_addr, DLA_GST_ENABLE_COL_REG_OFFSET, col_en);
  return kDifDlaOk;
}


dif_dla_result_t dif_dla_ddr2gb(const dif_dla_t *dla, uint32_t len, uint32_t addr0, uint32_t addr1,
                    uint32_t gb_addr, uint32_t gb_idx, uint32_t gb_mux, uint32_t direction) {
  if (dla == NULL ) {
    return kDifDlaBadArg;
  }

  mmio_region_write32(dla->params.base_addr, DLA_CPU_ACCESS_DDR_REG_OFFSET, 0x00); 

  uint32_t reg;
  mmio_region_write32(dla->params.base_addr, DLA_DDR2GB_DDR_ADDR0_REG_OFFSET, addr0);
  mmio_region_write32(dla->params.base_addr, DLA_DDR2GB_DDR_ADDR1_REG_OFFSET, addr1);
  reg = (len<<24)+(gb_idx<<16)+(gb_mux<<12)+gb_addr;
  mmio_region_write32(dla->params.base_addr, DLA_DDR2GB_GB_ADDR_REG_OFFSET, reg);
  reg = (1<<31) + direction;
  mmio_region_write32(dla->params.base_addr, DLA_DDR2GB_CTRL_REG_OFFSET, reg);
  return kDifDlaOk;
}


dif_dla_result_t dif_dla_fbuf2lbuf(const dif_dla_t *dla, uint32_t src_addr, uint32_t dest_addr,
                    uint32_t skip, uint32_t iter, uint32_t len, uint32_t dila, uint32_t mode) {
  if (dla == NULL ) {
    return kDifDlaBadArg;
  }

  uint32_t reg;
  reg = (dest_addr<<16) + src_addr;
  mmio_region_write32(dla->params.base_addr, DLA_GB2LB_ADDR_REG_OFFSET, reg);
  reg = (iter << 16) + skip;
  mmio_region_write32(dla->params.base_addr, DLA_GB2LB_SRC0_REG_OFFSET, reg);
  reg = (dila << 16) + len;
  mmio_region_write32(dla->params.base_addr, DLA_GB2LB_SRC1_REG_OFFSET, reg);
  reg = (1<<31) + mode;
  mmio_region_write32(dla->params.base_addr, DLA_GB2LB_CTRL_REG_OFFSET, reg);
  return kDifDlaOk;
}


dif_dla_result_t dif_dla_ppe_reset(const dif_dla_t *dla) {
  if (dla == NULL ) {
    return kDifDlaBadArg;
  }

  uint32_t reg;
  reg = 2;
  mmio_region_write32(dla->params.base_addr, DLA_GST_COMP_STATE_REG_OFFSET, reg);
  reg = (1 << 16) + 63;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_SIZE_REG_OFFSET, reg);
  reg = (1 << 31) + (15 << 24) + (0 << 21) + (0 << 18) + (0 << 17) + (0 << 16) 
          + (1 << 5) + (0 << 4) + 1;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_CTRL_REG_OFFSET, reg);
  return kDifDlaOk;
}


dif_dla_result_t dif_dla_conv(const dif_dla_t *dla, 
                uint32_t mode_spar, uint32_t k_scale, uint32_t k_size,
                uint32_t if_len, uint32_t of_len, uint32_t if_chl, uint32_t of_chl, 
                uint32_t pad_left, uint32_t pad_right, uint32_t pad_num, uint32_t sub_col,
                uint32_t sub_row, uint32_t lbuf_addr, uint32_t wbuf_addr, uint32_t ibuf_addr) {
  if (dla == NULL ) {
    return kDifDlaBadArg;
  }

  uint32_t reg;
  mmio_region_write32(dla->params.base_addr, DLA_GST_COMP_STATE_REG_OFFSET, 1);
  reg = (k_scale << 16) + k_size;
  mmio_region_write32(dla->params.base_addr, DLA_COMP_K_SIZE_REG_OFFSET, reg);
  reg = (of_len  << 16) + if_len;
  mmio_region_write32(dla->params.base_addr, DLA_COMP_F_SIZE_REG_OFFSET, reg);
  reg = (of_chl  << 16) + if_chl;
  mmio_region_write32(dla->params.base_addr, DLA_COMP_C_SIZE_REG_OFFSET, reg);
  reg = (pad_num << 16) + (pad_right << 8) + pad_left;
  mmio_region_write32(dla->params.base_addr, DLA_COMP_P_SIZE_REG_OFFSET, reg);
  reg = (sub_row << 8)  + sub_col;
  mmio_region_write32(dla->params.base_addr, DLA_COMP_FBLOAD_REG_OFFSET, reg);
  reg = (ibuf_addr << 24) + (wbuf_addr << 12) + (lbuf_addr);
  mmio_region_write32(dla->params.base_addr, DLA_COMP_ADDR_REG_OFFSET, reg);
  reg = (1<<30) + (mode_spar << 2) + 0;
  mmio_region_write32(dla->params.base_addr, DLA_COMP_CTRL_REG_OFFSET, reg);
  return kDifDlaOk;
}


dif_dla_result_t dif_dla_ppe_psum_accum(const dif_dla_t *dla, uint32_t acc_len, uint32_t row_num) {
  if (dla == NULL ) {
    return kDifDlaBadArg;
  }

  uint32_t reg;
  reg = 2;
  mmio_region_write32(dla->params.base_addr, DLA_GST_COMP_STATE_REG_OFFSET, reg);
  reg = (1 << 16) + acc_len;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_SIZE_REG_OFFSET, reg);
  reg = (1 << 31) + (row_num << 24) + 1;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_CTRL_REG_OFFSET, reg);
  return kDifDlaOk;
}


dif_dla_result_t dif_dla_ppe_comp(const dif_dla_t *dla, uint32_t elem, uint32_t bias, 
                    uint32_t pass, uint32_t row_num, uint32_t len, uint32_t iter, uint32_t post,
                    uint32_t mode, uint32_t oper, uint32_t fbuf_src, uint32_t fbuf_dest,
                    uint32_t abuf_src, uint32_t src_dila, uint32_t dest_dila, 
                    uint32_t src_skip, uint32_t dest_skip) {
  if (dla == NULL ) {
    return kDifDlaBadArg;
  }

  uint32_t reg;
  reg = 2;
  mmio_region_write32(dla->params.base_addr, DLA_GST_COMP_STATE_REG_OFFSET, reg);
  reg = (fbuf_dest << 16) + fbuf_src;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_FBUF_ADDR_REG_OFFSET, reg);
  reg = abuf_src;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_ABUF_ADDR_REG_OFFSET, reg);
  reg = (iter       << 16) + len;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_SIZE_REG_OFFSET, reg);
  reg = (dest_dila << 16) + src_dila;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_DILA_REG_OFFSET, reg);
  reg = (dest_skip << 16) + src_skip;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_SKIP_REG_OFFSET, reg);
  
  
  reg = (1 << 31) + (row_num << 24) + (oper << 21) + (pass << 20) + 
          (bias << 18) + (elem << 17) + (0 << 16) + (mode << 4) + (post << 2) + 2;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_CTRL_REG_OFFSET, reg);
  return kDifDlaOk;
}


dif_dla_result_t dif_dla_read_ddr(const dif_dla_t *dla,
                                  uint32_t offset_bytes, void *dest, size_t len_bytes ) {

  // Only 32b-aligned 32b word accesses are allowed.
  if (dla == NULL || dest == NULL || len_bytes % 4 != 0 ||
      offset_bytes % 4 != 0 ||
      offset_bytes + len_bytes > DLA_DMEM_SIZE_BYTES) {
    return kDifDlaBadArg;
  }
  
  mmio_region_memcpy_from_mmio32(
      dla->params.base_addr, DLA_DMEM_REG_OFFSET + offset_bytes, dest, len_bytes);

  
  return kDifDlaOk;
}


//ddr ctrl

dif_dla_result_t dif_dla_ddr_ctrl_init_calib(const dif_dla_t *dla) {
  if (dla == NULL ) {
    return kDifDlaBadArg;
  }
  
  uint32_t calib_complete = 0;
  while (calib_complete==0) {
    calib_complete = mmio_region_read32(dla->params.base_addr, DLA_INIT_CALIB_COMPLETE_REG_OFFSET);
  }
  
  return kDifDlaOk;
}


dif_dla_result_t dif_dla_ddr_ctrl_write_buf(const dif_dla_t *dla, uint32_t ddr_start_addr, void *src, size_t len_bytes) {
  if (dla == NULL || src == NULL) {
    return kDifDlaBadArg;
  }


  mmio_region_write32(dla->params.base_addr, DLA_CPU_ACCESS_DDR_REG_OFFSET, 0x01); 
  
  //write command
  uint32_t ctrl = (1<<31) | (1<<30) | ((len_bytes/8-1)<<22);
  mmio_region_write32(dla->params.base_addr, DLA_DDR_MOSI_U_REG_OFFSET, ctrl); //{dir: write, 1'b1, len: 127, '0}
  mmio_region_write32(dla->params.base_addr, DLA_DDR_MOSI_L_REG_OFFSET, ddr_start_addr); //addr 0
  mmio_region_write32(dla->params.base_addr, DLA_DDR_MOSI_VALID_REG_OFFSET, 0x01);

  for (int i=0; i<(len_bytes>>3); i++) {
    //write data
    mmio_region_write32(dla->params.base_addr, DLA_DDR_MOSI_U_REG_OFFSET, ((uint32_t*)src)[i*2]);
    mmio_region_write32(dla->params.base_addr, DLA_DDR_MOSI_L_REG_OFFSET, ((uint32_t*)src)[i*2+1]);
    mmio_region_write32(dla->params.base_addr, DLA_DDR_MOSI_VALID_REG_OFFSET, 0x01);
  }

  //wait for write ack
  uint32_t valid_reg_val = 0x0u;
  while (valid_reg_val == 0) {
    valid_reg_val = mmio_region_read32(dla->params.base_addr, DLA_DDR_MISO_VALID_REG_OFFSET);
  }

  //clear valid reg
  valid_reg_val = 0x0u;
  valid_reg_val = bitfield_bit32_write(valid_reg_val, DLA_DDR_MISO_VALID_DDR_MISO_VALID_BIT, false);
  mmio_region_write32(dla->params.base_addr, DLA_DDR_MISO_VALID_REG_OFFSET, valid_reg_val);
  
  
  return kDifDlaOk;
}


dif_dla_result_t dif_dla_ddr_ctrl_read(const dif_dla_t *dla, uint32_t ddr_start_addr,
                                      void *dest, size_t len_bytes, uint32_t* wptr) {
  if (dla == NULL ) {
    return kDifDlaBadArg;
  }
  
  uint32_t start_wptr = mmio_region_read32(dla->params.base_addr, DLA_RXF_CTRL_REG_OFFSET);
  
  mmio_region_write32(dla->params.base_addr, DLA_CPU_ACCESS_DDR_REG_OFFSET, 0x01); 

  //cpu read
  mmio_region_write32(dla->params.base_addr, DLA_CPU_RD_REG_OFFSET, 0x00);

  uint32_t ctrl = (0<<31) | (1<<30) | ((len_bytes/8-1)<<22);
  // LOG_INFO("DDR read ctrl %x, addr %d.", ctrl, ddr_start_addr);
  mmio_region_write32(dla->params.base_addr, DLA_DDR_MOSI_U_REG_OFFSET, ctrl); //{dir: read, 1'b1, len: 7, '0}
  mmio_region_write32(dla->params.base_addr, DLA_DDR_MOSI_L_REG_OFFSET, ddr_start_addr); //addr 0
  mmio_region_write32(dla->params.base_addr, DLA_DDR_MOSI_VALID_REG_OFFSET, 0x01);
  
  //wait for all DDR read to complete
  uint32_t cur_wptr, bytes_written;
  do {
    cur_wptr = mmio_region_read32(dla->params.base_addr, DLA_RXF_CTRL_REG_OFFSET);
    //LOG_INFO("DMEM write pointer is at %d.", cur_wptr);
    
    if ((cur_wptr&0x00001000) == (start_wptr&0x00001000)) bytes_written = cur_wptr-start_wptr; //same phase
    else bytes_written = DLA_DMEM_SIZE_BYTES - ((start_wptr&0x00000fff) - (cur_wptr&0x00000fff)); //wrapped-around
    
  } while (bytes_written != len_bytes);
  *wptr = cur_wptr;
  
  ///*can remove ============================================
  uint32_t valid_reg_val = 0x0u;
  while (valid_reg_val == 0) {
    valid_reg_val = mmio_region_read32(dla->params.base_addr, DLA_DDR_MISO_VALID_REG_OFFSET);
  }
  
  //clear valid reg
  valid_reg_val = 0x0u;
  valid_reg_val = bitfield_bit32_write(valid_reg_val, DLA_DDR_MISO_VALID_DDR_MISO_VALID_BIT, false);
  mmio_region_write32(dla->params.base_addr, DLA_DDR_MISO_VALID_REG_OFFSET, valid_reg_val);
  // can remove ============================================*/
  
  //cpu read
  mmio_region_write32(dla->params.base_addr, DLA_CPU_RD_REG_OFFSET, 0x01);
  
  //read from dmem
  if ((cur_wptr&0x00001000) == (start_wptr&0x00001000)){
    // LOG_INFO("start_wptr %d, cur_wptr %d, len_bytes %d", 
    //   (start_wptr&0x00000fff), (cur_wptr&0x00000fff), len_bytes);
    mmio_region_memcpy_from_mmio32(
        dla->params.base_addr, DLA_DMEM_REG_OFFSET+(start_wptr&0x00000fff), dest, len_bytes);
  }
  else {
    //wrapped-around, read 2 times
    uint32_t first_read_bytes = DLA_DMEM_SIZE_BYTES - (start_wptr&0x00000fff);
    uint32_t second_read_bytes = cur_wptr&0x00000fff;

    // LOG_INFO("start_wptr %d, cur_wptr %d, first_read_bytes %d, second_read_bytes %d", 
    //   (start_wptr&0x00000fff), (cur_wptr&0x00000fff), first_read_bytes, second_read_bytes);
    mmio_region_memcpy_from_mmio32(
        dla->params.base_addr, DLA_DMEM_REG_OFFSET+(start_wptr&0x00000fff), dest, first_read_bytes);
    mmio_region_memcpy_from_mmio32(
        dla->params.base_addr, DLA_DMEM_REG_OFFSET, (uint8_t *)dest+first_read_bytes/*dest_ddr2*/, second_read_bytes);
  }
  return kDifDlaOk;
}


dif_dla_result_t dif_dla_ddr_ctrl_wptr(const dif_dla_t *dla, uint32_t* wptr) {
  if (dla == NULL ) {
    return kDifDlaBadArg;
  }
  
  *wptr = mmio_region_read32(dla->params.base_addr, DLA_RXF_CTRL_REG_OFFSET);

  return kDifDlaOk;
}




// following is adapted from dla_src.cpp for microblaze
dif_dla_result_t dif_dla_pbuf_rst(const dif_dla_t *dla, uint32_t len) {
  if (dla == NULL ) {
    return kDifDlaBadArg;
  }

  uint32_t reg;
  reg = 2;
  mmio_region_write32(dla->params.base_addr, DLA_GST_COMP_STATE_REG_OFFSET, reg);
  reg = (1 << 16) + 63;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_SIZE_REG_OFFSET, reg);
  reg = (1 << 31) + (15 << 24) + (0 << 21) + (0 << 18) + (0 << 17) + (0 << 16) 
          + (1 << 5) + (0 << 4) + 1;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_CTRL_REG_OFFSET, reg);

  dif_dla_wait_for_done(dla);

  return kDifDlaOk;
}

dif_dla_result_t dif_dla_move_ddr2gb(const dif_dla_t *dla, uint32_t len, uint64_t ddr_addr,
                    uint32_t gb_addr, uint32_t gb_idx, uint32_t gb_mux, uint32_t direction) {
  if (dla == NULL ) {
    return kDifDlaBadArg;
  }

  mmio_region_write32(dla->params.base_addr, DLA_CPU_ACCESS_DDR_REG_OFFSET, 0x00); 

  uint32_t reg;
  uint32_t len_left = len;

  while (len_left != 0) {
    uint32_t len_todo;
    len_todo = (len_left > 256) ? 256 : len_left;

    mmio_region_write32(dla->params.base_addr, DLA_DDR2GB_DDR_ADDR0_REG_OFFSET, ddr_addr);
    mmio_region_write32(dla->params.base_addr, DLA_DDR2GB_DDR_ADDR1_REG_OFFSET, ddr_addr>>32);
    reg = ((len_todo-1)<<24)+(gb_idx<<16)+(gb_mux<<12)+gb_addr;
    mmio_region_write32(dla->params.base_addr, DLA_DDR2GB_GB_ADDR_REG_OFFSET, reg);
    reg = (1<<31) + direction;
    mmio_region_write32(dla->params.base_addr, DLA_DDR2GB_CTRL_REG_OFFSET, reg);

    len_left  -= len_todo;
    gb_addr   += len_todo;
    ddr_addr  += len_todo;

    // LOG_INFO("ddr2gb sent, len_left %d, len_todo %d", 
    //   len_left, len_todo);
    dif_dla_wait_for_done(dla);
    // LOG_INFO("ddr2gb command done");
  }

  return kDifDlaOk;
}

dif_dla_result_t dif_dla_move_fbuf2lbuf(const dif_dla_t *dla, uint32_t src_addr, uint32_t dest_addr,
                    uint32_t skip, uint32_t iter, uint32_t len, uint32_t dila, uint32_t mode) {
  if (dla == NULL ) {
    return kDifDlaBadArg;
  }

  uint32_t reg;
  reg = (dest_addr<<16) + src_addr;
  mmio_region_write32(dla->params.base_addr, DLA_GB2LB_ADDR_REG_OFFSET, reg);
  reg = (iter << 16) + skip;
  mmio_region_write32(dla->params.base_addr, DLA_GB2LB_SRC0_REG_OFFSET, reg);
  reg = (dila << 16) + len;
  mmio_region_write32(dla->params.base_addr, DLA_GB2LB_SRC1_REG_OFFSET, reg);
  reg = (1<<31) + mode;
  mmio_region_write32(dla->params.base_addr, DLA_GB2LB_CTRL_REG_OFFSET, reg);

  dif_dla_wait_for_done(dla);

  return kDifDlaOk;
}

dif_dla_result_t dif_dla_fc_comp(const dif_dla_t *dla, 
                uint32_t mode_spar, uint32_t k_scale,
                uint32_t if_chl, uint32_t of_chl, 
                uint32_t sub_col, uint32_t sub_row,
                uint32_t lbuf_addr, uint32_t wbuf_addr, uint32_t ibuf_addr,
                uint32_t row_num, uint32_t acc_len ) {
  if (dla == NULL ) {
    return kDifDlaBadArg;
  }

  uint32_t reg;

  // FC Stage Configuration
  mmio_region_write32(dla->params.base_addr, DLA_GST_COMP_STATE_REG_OFFSET, 1);
  reg = (k_scale << 16) + 1;
  mmio_region_write32(dla->params.base_addr, DLA_COMP_K_SIZE_REG_OFFSET, reg);
  reg = (of_chl  << 16) + if_chl;
  mmio_region_write32(dla->params.base_addr, DLA_COMP_C_SIZE_REG_OFFSET, reg);
  reg = (sub_row << 8)  + sub_col;
  mmio_region_write32(dla->params.base_addr, DLA_COMP_FBLOAD_REG_OFFSET, reg);
  reg = (ibuf_addr << 24) + (wbuf_addr << 12) + (lbuf_addr);
  mmio_region_write32(dla->params.base_addr, DLA_COMP_ADDR_REG_OFFSET, reg);
  reg = (1<<31) + (mode_spar << 2) + 1;
  mmio_region_write32(dla->params.base_addr, DLA_COMP_CTRL_REG_OFFSET, reg);

  dif_dla_wait_for_done(dla);

  // Psum Accumulation Stage Configuration
  mmio_region_write32(dla->params.base_addr, DLA_GST_COMP_STATE_REG_OFFSET, 2);
  reg = (1 << 16) + acc_len;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_SIZE_REG_OFFSET, reg);
  reg = (1 << 31) + (row_num << 24) + 1;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_CTRL_REG_OFFSET, reg);

  dif_dla_wait_for_done(dla);

  return kDifDlaOk;
}


dif_dla_result_t dif_dla_conv_comp(const dif_dla_t *dla, 
                uint32_t mode_spar, uint32_t k_size, uint32_t k_scale,
                uint32_t if_len, uint32_t of_len, uint32_t if_chl, uint32_t of_chl, 
                uint32_t pad_left, uint32_t pad_right, uint32_t pad_num, uint32_t sub_col,
                uint32_t sub_row, uint32_t lbuf_addr, uint32_t wbuf_addr, uint32_t ibuf_addr,
                uint32_t row_num, uint32_t acc_len) {
  if (dla == NULL ) {
    return kDifDlaBadArg;
  }

  uint32_t reg, lbuf_inc, wbuf_inc, ibuf_inc;

  lbuf_inc = (if_len - pad_left - pad_right + 1) * sub_col;
  wbuf_inc = k_size * of_chl;
  if (mode_spar == 2) {
    ibuf_inc = k_size * of_chl;
  }
  else {
    ibuf_inc = k_size;
  }

  mmio_region_write32(dla->params.base_addr, DLA_GST_COMP_STATE_REG_OFFSET, 1);
  reg = (k_scale << 16) + k_size;
  mmio_region_write32(dla->params.base_addr, DLA_COMP_K_SIZE_REG_OFFSET, reg);
  reg = (of_len  << 16) + if_len;
  mmio_region_write32(dla->params.base_addr, DLA_COMP_F_SIZE_REG_OFFSET, reg);
  reg = (of_chl  << 16) + if_chl;
  mmio_region_write32(dla->params.base_addr, DLA_COMP_C_SIZE_REG_OFFSET, reg);
  reg = (pad_num << 16) + (pad_right << 8) + pad_left;
  mmio_region_write32(dla->params.base_addr, DLA_COMP_P_SIZE_REG_OFFSET, reg);
  reg = (sub_row << 8)  + sub_col;
  mmio_region_write32(dla->params.base_addr, DLA_COMP_FBLOAD_REG_OFFSET, reg);

  for (uint32_t i = 0; i < if_chl; i++) {
    reg = ((ibuf_addr + ibuf_inc * i) << 24) + 
          ((wbuf_addr + wbuf_inc * i) << 12) + 
          ( lbuf_addr + lbuf_inc * i);
    mmio_region_write32(dla->params.base_addr, DLA_COMP_ADDR_REG_OFFSET, reg);
    
    // Convolution Computation Start
    reg = (1 << 30) + (mode_spar << 2)  + 0;
    mmio_region_write32(dla->params.base_addr, DLA_COMP_CTRL_REG_OFFSET, reg);

    // Check Finish Flag
    dif_dla_wait_for_done(dla);
  }

  // Psum Accumulation Stage Configuration
  mmio_region_write32(dla->params.base_addr, DLA_GST_COMP_STATE_REG_OFFSET, 2);
  reg = (1 << 16) + acc_len;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_SIZE_REG_OFFSET, reg);
  reg = (1 << 31) + (row_num << 24) + 1;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_CTRL_REG_OFFSET, reg);
  dif_dla_wait_for_done(dla);

  return kDifDlaOk;
}


dif_dla_result_t dif_dla_ppe_comp_full(const dif_dla_t *dla, 
                    uint32_t act, uint32_t elem, uint32_t bias, 
                    uint32_t pass, uint32_t row, uint32_t len, uint32_t iter, uint32_t post,
                    uint32_t mode, uint32_t oper, uint32_t fbuf_src, uint32_t fbuf_dest,
                    uint32_t abuf_src, uint32_t src_dila, uint32_t dest_dila, 
                    uint32_t src_skip, uint32_t dest_skip,
                    uint32_t *act_k, uint32_t *act_b, uint32_t *act_x ) {
  if (dla == NULL ) {
    return kDifDlaBadArg;
  }

  uint32_t reg;

  if (act       == 11111)    act       = 0;
  if (elem      == 11111)    elem      = 0;
  if (bias      == 11111)    bias      = 0;
  if (pass      == 11111)    pass      = 0;
  if (row       == 11111)    row       = 15;
  if (len       == 11111)    len       = 0;
  if (iter      == 11111)    iter      = 1;
  if (post      == 11111)    post      = 0;
  if (mode      == 11111)    mode      = 1;
  if (oper      == 11111)    oper      = 0;
  if (fbuf_src  == 11111)    fbuf_src  = 0;
  if (fbuf_dest == 11111)    fbuf_dest = 0;
  if (abuf_src  == 11111)    abuf_src  = 0;
  if (src_dila  == 11111)    src_dila  = 1;
  if (dest_dila == 11111)    dest_dila = 1;
  if (src_skip  == 11111)    src_skip  = 1;
  if (dest_skip == 11111)    dest_skip = 1;

  if (act) {
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_K0_REG_OFFSET, (act_k[1] << 16) + act_k[0]   );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_K1_REG_OFFSET, (act_k[3] << 16) + act_k[2]   );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_K2_REG_OFFSET, (act_k[5] << 16) + act_k[4]   );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_K3_REG_OFFSET, (act_k[7] << 16) + act_k[6]   );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_K4_REG_OFFSET, (act_k[9] << 16) + act_k[8]   );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_K5_REG_OFFSET, (act_k[11] << 16) + act_k[10] );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_K6_REG_OFFSET, (act_k[13] << 16) + act_k[12] );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_K7_REG_OFFSET, (act_k[15] << 16) + act_k[14] );
    
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_B0_REG_OFFSET, (act_b[1] << 16) + act_b[0]   );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_B1_REG_OFFSET, (act_b[3] << 16) + act_b[2]   );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_B2_REG_OFFSET, (act_b[5] << 16) + act_b[4]   );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_B3_REG_OFFSET, (act_b[7] << 16) + act_b[6]   );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_B4_REG_OFFSET, (act_b[9] << 16) + act_b[8]   );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_B5_REG_OFFSET, (act_b[11] << 16) + act_b[10] );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_B6_REG_OFFSET, (act_b[13] << 16) + act_b[12] );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_B7_REG_OFFSET, (act_b[15] << 16) + act_b[14] );

    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_X0_REG_OFFSET, (act_x[1] << 16) + act_x[0]   );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_X1_REG_OFFSET, (act_x[3] << 16) + act_x[2]   );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_X2_REG_OFFSET, (act_x[5] << 16) + act_x[4]   );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_X3_REG_OFFSET, (act_x[7] << 16) + act_x[6]   );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_X4_REG_OFFSET, (act_x[9] << 16) + act_x[8]   );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_X5_REG_OFFSET, (act_x[11] << 16) + act_x[10] );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_X6_REG_OFFSET, (act_x[13] << 16) + act_x[12] );
    mmio_region_write32(dla->params.base_addr, DLA_PPE_ACT_X7_REG_OFFSET, act_x[14]                     );
  }

  reg = 2;
  mmio_region_write32(dla->params.base_addr, DLA_GST_COMP_STATE_REG_OFFSET, reg);
  reg = (fbuf_dest << 16) + fbuf_src;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_FBUF_ADDR_REG_OFFSET, reg);
  reg = abuf_src;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_ABUF_ADDR_REG_OFFSET, reg);
  reg = (iter       << 16) + len;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_SIZE_REG_OFFSET, reg);
  reg = (dest_dila << 16) + src_dila;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_DILA_REG_OFFSET, reg);
  reg = (dest_skip << 16) + src_skip;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_SKIP_REG_OFFSET, reg);
  
  
  reg = (1 << 31) + (row << 24) + (oper << 21) + (pass << 20) + 
          (bias << 18) + (elem << 17) + (act << 16) + (mode << 4) + (post << 2) + 2;
  mmio_region_write32(dla->params.base_addr, DLA_PPE_CTRL_REG_OFFSET, reg);

  dif_dla_wait_for_done(dla);

  return kDifDlaOk;
}