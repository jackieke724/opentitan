// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "sw/device/lib/dif/dif_vec_dot.h"

#include "sw/device/lib/base/bitfield.h"

#include "vec_dot_regs.h"  // Generated.


dif_vec_dot_result_t dif_vec_dot_init(dif_vec_dot_params_t params,
                                dif_vec_dot_t *vec_dot) {
  if (vec_dot == NULL) {
    return kDifVecDotBadArg;
  }

  vec_dot->params = params;
  dif_vec_dot_reset(vec_dot);

  return kDifVecDotOk;
}


dif_vec_dot_result_t dif_vec_dot_reset(const dif_vec_dot_t *vec_dot) {
  if (vec_dot == NULL) {
    return kDifVecDotBadArg;
  }

  mmio_region_write32(vec_dot->params.base_addr, VEC_DOT_INTR_ENABLE_REG_OFFSET, 0);

  // Clear all pending interrupts.
  mmio_region_write32(vec_dot->params.base_addr, VEC_DOT_INTR_STATE_REG_OFFSET, 0xFFFFFFFF);

  return kDifVecDotOk;
}


dif_vec_dot_result_t dif_vec_dot_start(const dif_vec_dot_t *vec_dot) {
  if (vec_dot == NULL ) {
    return kDifVecDotBadArg;
  }
  
  uint32_t cmd_reg_val = 0x0u;
  cmd_reg_val = bitfield_bit32_write(cmd_reg_val, VEC_DOT_CMD_START_BIT, true);
  mmio_region_write32(vec_dot->params.base_addr, VEC_DOT_CMD_REG_OFFSET, cmd_reg_val);

  return kDifVecDotOk;
}


dif_vec_dot_result_t dif_vec_dot_send_vectors(const dif_vec_dot_t *vec_dot,
                                                                uint32_t transaction){
  for (uint32_t i=0; i<8; i++) {
    mmio_region_write32(vec_dot->params.base_addr, VEC_DOT_WDATA_REG_OFFSET, transaction+i);
  }
  return kDifVecDotOk;
}


dif_vec_dot_result_t dif_vec_dot_is_busy(const dif_vec_dot_t *vec_dot, bool *busy) {
  if (vec_dot == NULL ) {
    return kDifVecDotBadArg;
  }

  uint32_t status = mmio_region_read32(vec_dot->params.base_addr, VEC_DOT_STATUS_REG_OFFSET);
  *busy = bitfield_bit32_read(status, VEC_DOT_STATUS_BUSY_BIT);

  return kDifVecDotOk;
}

dif_vec_dot_result_t dif_vec_dot_read(const dif_vec_dot_t *vec_dot, uint32_t* result){
  if (vec_dot == NULL) {
    return kDifVecDotBadArg;
  }

  *result = mmio_region_read32(vec_dot->params.base_addr, VEC_DOT_DOTP_RESULT_REG_OFFSET);

  return kDifVecDotOk;

}


dif_vec_dot_result_t dif_vec_dot_irq_set_enabled(const dif_vec_dot_t *vec_dot,
                                                         dif_vec_dot_irq_t irq,
                                                         dif_vec_dot_enable_t enable){
  if (vec_dot == NULL) {
    return kDifVecDotBadArg;
  }
/*
  uint8_t bit_index;
  if (!irq_bit_index_get(irq_type, &bit_index)) {
    return kDifVecDotError;
  }
*/
  // Enable/Disable interrupt.
  uint32_t register_value =
      mmio_region_read32(vec_dot->params.base_addr, VEC_DOT_INTR_ENABLE_REG_OFFSET);
  register_value = bitfield_bit32_write(register_value, VEC_DOT_INTR_STATE_DONE_BIT,
                                        (enable == kDifVecDotToggleEnabled));
  mmio_region_write32(vec_dot->params.base_addr, VEC_DOT_INTR_ENABLE_REG_OFFSET,
                      register_value);

  return kDifVecDotOk;
}



