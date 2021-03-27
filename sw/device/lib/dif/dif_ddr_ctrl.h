// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef OPENTITAN_SW_DEVICE_LIB_DIF_DIF_DDR_CTRL_H_
#define OPENTITAN_SW_DEVICE_LIB_DIF_DIF_DDR_CTRL_H_

/**
 * @file
 * @brief <a href="/hw/ip/ddr_ctrl/doc/">DDR CONTROLLER</a> Device Interface Functions
 */

#include <stdint.h>

#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/dif/dif_warn_unused_result.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * A toggle state: enabled, or disabled.
 *
 * This enum may be used instead of a `bool` when describing an enabled/disabled
 * state.
 */
typedef enum dif_ddr_ctrl_toggle {
  /*
   * The "enabled" state.
   */
  kDifDdrCtrlToggleEnabled,
  /**
   * The "disabled" state.
   */
  kDifDdrCtrlToggleDisabled,
} dif_ddr_ctrl_toggle_t;

/**
 * Hardware instantiation parameters for DDR CONTROLLER.
 *
 * This struct describes information about the underlying hardware that is
 * not determined until the hardware design is used as part of a top-level
 * design.
 */
typedef struct dif_ddr_ctrl_params {
  /**
   * The base address for the DDR CONTROLLER hardware registers.
   */
  mmio_region_t base_addr;

  // Other fields, if necessary.
} dif_ddr_ctrl_params_t;

/**
 * Runtime configuration for DDR CONTROLLER.
 *
 * This struct describes runtime information for one-time configuration of the
 * hardware.
 */
typedef struct dif_ddr_ctrl_config {
  // Fields, if necessary.
  mmio_region_t base_addr;
} dif_ddr_ctrl_config_t;

/**
 * A handle to DDR CONTROLLER.
 *
 * This type should be treated as opaque by users.
 */
typedef struct dif_ddr_ctrl {
  dif_ddr_ctrl_params_t params;

  // Other fields, if necessary.
} dif_ddr_ctrl_t;

/**
 * The result of a DDR CONTROLLER operation.
 */
typedef enum dif_ddr_ctrl_result {
  /**
   * Indicates that the operation succeeded.
   */
  kDifDdrCtrlOk = 0,
  /**
   * Indicates some unspecified failure.
   */
  kDifDdrCtrlError = 1,
  /**
   * Indicates that some parameter passed into a function failed a
   * precondition.
   *
   * When this value is returned, no hardware operations occured.
   */
  kDifDdrCtrlBadArg = 2,
  /**
   * Indicates that this operation has been locked out, and can never
   * succeed until hardware reset.
   */
  // Remove this variant if you don't need it.
  kDifDdrCtrlLocked = 3,
} dif_ddr_ctrl_result_t;



/**
 * Calculates information needed to safely call a DIF. Functions like this
 * should be used instead of global variables or #defines.
 *
 * This function does not actuate the hardware.
 *
 * @param params Hardware instantiation parameters.
 * @return The information required.
 */
DIF_WARN_UNUSED_RESULT
uint32_t dif_ddr_ctrl_get_size(dif_ddr_ctrl_params_t params);

/**
 * Creates a new handle for DDR CONTROLLER.
 *
 * This function does not actuate the hardware.
 *
 * @param params Hardware instantiation parameters.
 * @param[out] handle Out param for the initialized handle.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_ddr_ctrl_result_t dif_ddr_ctrl_init(dif_ddr_ctrl_params_t params,
                                              dif_ddr_ctrl_t *ddr_ctrl);

//dif_ddr_ctrl_result_t dif_ddr_ctrl_reset(const dif_ddr_ctrl_t *ddr_ctrl);
dif_ddr_ctrl_result_t dif_ddr_ctrl_init_calib(const dif_ddr_ctrl_t *ddr_ctrl);
dif_ddr_ctrl_result_t dif_ddr_ctrl_write(const dif_ddr_ctrl_t *ddr_ctrl, uint32_t data_u, uint32_t data_l);
dif_ddr_ctrl_result_t dif_ddr_ctrl_read(const dif_ddr_ctrl_t *ddr_ctrl, uint32_t* data_u, uint32_t* data_l);

/**
 * Configures DDR CONTROLLER with runtime information.
 *
 * This function should need to be called once for the lifetime of `handle`.
 *
 * @param handle A DDR CONTROLLER handle.
 * @param config Runtime configuration parameters.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_ddr_ctrl_result_t dif_ddr_ctrl_configure(const dif_ddr_ctrl_t *handle,
                                                   dif_ddr_ctrl_config_t config);

/**
 * Locks out DDR CONTROLLER functionality.
 *
 * This function is reentrant: calling it while functionality is locked will
 * have no effect and return `kDifDdrCtrlOk`.
 *
 * @param handle A DDR CONTROLLER handle.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_ddr_ctrl_result_t dif_ddr_ctrl_lock(const dif_ddr_ctrl_t *handle);

/**
 * Checks whether this DDR CONTROLLER is locked.
 *
 * @param handle A DDR CONTROLLER handle.
 * @param[out] is_locked Out-param for the locked state.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_ddr_ctrl_result_t dif_ddr_ctrl_is_locked(const dif_ddr_ctrl_t *handle,
                                                   bool *is_locked);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // OPENTITAN_SW_DEVICE_LIB_DIF_DIF_DDR_CTRL_H_
