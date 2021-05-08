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
  //LOG_INFO("DDR read ctrl %x, addr %d.", ctrl, ddr_start_addr);
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
  mmio_region_memcpy_from_mmio32(
      dla->params.base_addr, DLA_DMEM_REG_OFFSET+(ddr_start_addr*8%DLA_DMEM_SIZE_BYTES), dest, len_bytes);
      
  return kDifDlaOk;
}


dif_dla_result_t dif_dla_ddr_ctrl_wptr(const dif_dla_t *dla, uint32_t* wptr) {
  if (dla == NULL ) {
    return kDifDlaBadArg;
  }
  
  *wptr = mmio_region_read32(dla->params.base_addr, DLA_RXF_CTRL_REG_OFFSET);

  return kDifDlaOk;
}
