// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "sw/device/lib/dif/dif_ddr_ctrl.h"

#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/runtime/log.h" //debug use
#include "ddr_ctrl_regs.h"  // Generated.


dif_ddr_ctrl_result_t dif_ddr_ctrl_init(dif_ddr_ctrl_params_t params,
                                dif_ddr_ctrl_t *ddr_ctrl) {
  if (ddr_ctrl == NULL) {
    return kDifDdrCtrlBadArg;
  }

  ddr_ctrl->params = params;
  //dif_ddr_ctrl_reset(ddr_ctrl);

  return kDifDdrCtrlOk;
}



dif_ddr_ctrl_result_t dif_ddr_ctrl_init_calib(const dif_ddr_ctrl_t *ddr_ctrl) {
  if (ddr_ctrl == NULL ) {
    return kDifDdrCtrlBadArg;
  }
  
  uint32_t calib_complete = 0;
  while (calib_complete==0) {
    calib_complete = mmio_region_read32(ddr_ctrl->params.base_addr, DDR_CTRL_INIT_CALIB_COMPLETE_REG_OFFSET);
  }
  
  return kDifDdrCtrlOk;
}


dif_ddr_ctrl_result_t dif_ddr_ctrl_write(const dif_ddr_ctrl_t *ddr_ctrl, uint32_t data_u, uint32_t data_l) {
  if (ddr_ctrl == NULL ) {
    return kDifDdrCtrlBadArg;
  }
  
  //write command
  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_U_REG_OFFSET, 0xc1c00000); //{dir: write, 1'b1, len: 7, '0}
  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_L_REG_OFFSET, 0x0); //addr 0
  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_VALID_REG_OFFSET, 0x01);
  
  //write data
  uint32_t valid_reg_val = 0x0u;
  valid_reg_val = bitfield_bit32_write(valid_reg_val, DDR_CTRL_DDRS_MOSI_VALID_DDRS_MOSI_VALID_BIT, true);//0x01
  for (int i=0; i<8; i++) {
    //LOG_INFO("DDR write data %x %x.", data_u+i, data_l+i);
    mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_U_REG_OFFSET, data_u+i);
    mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_L_REG_OFFSET, data_l+i);
    mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_VALID_REG_OFFSET, valid_reg_val);
  }
  
  //wait for write ack
  valid_reg_val = 0x0u;
  while (valid_reg_val == 0) {
    valid_reg_val = mmio_region_read32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MISO_VALID_REG_OFFSET);
  }
  //clear valid reg
  valid_reg_val = 0x0u;
  valid_reg_val = bitfield_bit32_write(valid_reg_val, DDR_CTRL_DDRS_MISO_VALID_DDRS_MISO_VALID_BIT, false);
  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MISO_VALID_REG_OFFSET, valid_reg_val);
  
  return kDifDdrCtrlOk;
}


dif_ddr_ctrl_result_t dif_ddr_ctrl_write_buf(const dif_ddr_ctrl_t *ddr_ctrl, uint32_t ddr_start_addr, void *src, size_t len_bytes) {
  if (ddr_ctrl == NULL || src == NULL) {
    return kDifDdrCtrlBadArg;
  }
  
  //write command
  uint32_t ctrl = (1<<31) | (1<<30) | ((len_bytes/8-1)<<22);
  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_U_REG_OFFSET, ctrl); //{dir: write, 1'b1, len: 127, '0}
  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_L_REG_OFFSET, ddr_start_addr); //addr 0
  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_VALID_REG_OFFSET, 0x01);
  
  for (int i=0; i<(len_bytes>>3); i++) {
    //write data
    mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_U_REG_OFFSET, ((uint32_t*)src)[i*2]);
    mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_L_REG_OFFSET, ((uint32_t*)src)[i*2+1]);
    mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_VALID_REG_OFFSET, 0x01);
  }
  
  //wait for write ack
  uint32_t valid_reg_val = 0x0u;
  while (valid_reg_val == 0) {
    valid_reg_val = mmio_region_read32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MISO_VALID_REG_OFFSET);
  }
  //clear valid reg
  valid_reg_val = 0x0u;
  valid_reg_val = bitfield_bit32_write(valid_reg_val, DDR_CTRL_DDRS_MISO_VALID_DDRS_MISO_VALID_BIT, false);
  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MISO_VALID_REG_OFFSET, valid_reg_val);
  
  
  return kDifDdrCtrlOk;
}



dif_ddr_ctrl_result_t dif_ddr_ctrl_read(const dif_ddr_ctrl_t *ddr_ctrl, uint32_t ddr_start_addr,
                                      uint32_t* data_u, uint32_t* data_l, void *dest, size_t len_bytes, uint32_t* wptr) {
  if (ddr_ctrl == NULL ) {
    return kDifDdrCtrlBadArg;
  }
  
  uint32_t start_wptr = mmio_region_read32(ddr_ctrl->params.base_addr, DDR_CTRL_RXF_CTRL_REG_OFFSET);
  
  //cpu read
  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_CPU_RD_REG_OFFSET, 0x00);

  uint32_t ctrl = (0<<31) | (1<<30) | ((len_bytes/8-1)<<22);
  //LOG_INFO("DDR read ctrl %x, addr %d.", ctrl, ddr_start_addr);
  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_U_REG_OFFSET, ctrl); //{dir: read, 1'b1, len: 7, '0}
  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_L_REG_OFFSET, ddr_start_addr); //addr 0
  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_VALID_REG_OFFSET, 0x01);
  
  //wait for all DDR read to complete
  uint32_t cur_wptr, bytes_written;
  do {
    cur_wptr = mmio_region_read32(ddr_ctrl->params.base_addr, DDR_CTRL_RXF_CTRL_REG_OFFSET);
    LOG_INFO("DMEM write pointer is at %d.", cur_wptr);
    
    if ((cur_wptr&0x00001000) == (start_wptr&0x00001000)) bytes_written = cur_wptr-start_wptr; //same phase
    else bytes_written = DDR_CTRL_DMEM_SIZE_BYTES - ((start_wptr&0x00000fff) - (cur_wptr&0x00000fff)); //wrapped-around
    
  } while (bytes_written != len_bytes);
  *wptr = cur_wptr;
  
  uint32_t valid_reg_val = 0x0u;
  while (valid_reg_val == 0) {
    valid_reg_val = mmio_region_read32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MISO_VALID_REG_OFFSET);
  }
  *data_u = mmio_region_read32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MISO_U_REG_OFFSET);
  *data_l = mmio_region_read32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MISO_L_REG_OFFSET);
  
  //clear valid reg
  valid_reg_val = 0x0u;
  valid_reg_val = bitfield_bit32_write(valid_reg_val, DDR_CTRL_DDRS_MISO_VALID_DDRS_MISO_VALID_BIT, false);
  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MISO_VALID_REG_OFFSET, valid_reg_val);

  
  //cpu read
  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_CPU_RD_REG_OFFSET, 0x01);
  
  //read from dmem
  mmio_region_memcpy_from_mmio32(
      ddr_ctrl->params.base_addr, DDR_CTRL_DMEM_REG_OFFSET+(ddr_start_addr*8%DDR_CTRL_DMEM_SIZE_BYTES), dest, len_bytes);
      
  return kDifDdrCtrlOk;
}


