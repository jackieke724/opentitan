// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "sw/device/lib/dif/dif_ddr_ctrl.h"

#include "sw/device/lib/base/bitfield.h"

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
  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_U_REG_OFFSET, data_u);
  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_L_REG_OFFSET, data_l);
  uint32_t valid_reg_val = 0x0u;
  valid_reg_val = bitfield_bit32_write(valid_reg_val, DDR_CTRL_DDRS_MOSI_VALID_DDRS_MOSI_VALID_BIT, true);//0x01
  for (int i=0; i<8; i++) {
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


dif_ddr_ctrl_result_t dif_ddr_ctrl_read(const dif_ddr_ctrl_t *ddr_ctrl, uint32_t* data_u, uint32_t* data_l) {
  if (ddr_ctrl == NULL ) {
    return kDifDdrCtrlBadArg;
  }
  uint32_t valid_reg_val = 0x0u;

  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_U_REG_OFFSET, 0x41c00000); //{dir: read, 1'b1, len: 7, '0}
  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_L_REG_OFFSET, 0x0); //addr 0
  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MOSI_VALID_REG_OFFSET, 0x01);
  
  valid_reg_val = 0x0u;
  while (valid_reg_val == 0) {
    valid_reg_val = mmio_region_read32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MISO_VALID_REG_OFFSET);
  }
  *data_u = mmio_region_read32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MISO_U_REG_OFFSET);
  *data_l = mmio_region_read32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MISO_L_REG_OFFSET);
  
  //clear valid reg
  valid_reg_val = 0x0u;
  valid_reg_val = bitfield_bit32_write(valid_reg_val, DDR_CTRL_DDRS_MISO_VALID_DDRS_MISO_VALID_BIT, false);
  mmio_region_write32(ddr_ctrl->params.base_addr, DDR_CTRL_DDRS_MISO_VALID_REG_OFFSET, valid_reg_val);

  return kDifDdrCtrlOk;
}


