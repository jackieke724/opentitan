// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef OPENTITAN_SW_DEVICE_LIB_DIF_DIF_DLA_H_
#define OPENTITAN_SW_DEVICE_LIB_DIF_DIF_DLA_H_


/**
 * @file
 * @brief <a href="/hw/ip/dla/doc/">DLA</a> Device Interface Functions
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
typedef enum dif_dla_toggle {
  /*
   * The "enabled" state.
   */
  kDifDlaToggleEnabled,
  /**
   * The "disabled" state.
   */
  kDifDlaToggleDisabled,
} dif_dla_toggle_t;

/**
 * Hardware instantiation parameters for DLA.
 *
 * This struct describes information about the underlying hardware that is
 * not determined until the hardware design is used as part of a top-level
 * design.
 */
typedef struct dif_dla_params {
  /**
   * The base address for the DLA hardware registers.
   */
  mmio_region_t base_addr;

  // Other fields, if necessary.
} dif_dla_params_t;

/**
 * Runtime configuration for DLA.
 *
 * This struct describes runtime information for one-time configuration of the
 * hardware.
 */
typedef struct dif_dla_config {
  // Fields, if necessary.
  mmio_region_t base_addr;
} dif_dla_config_t;

/**
 * A handle to DLA.
 *
 * This type should be treated as opaque by users.
 */
typedef struct dif_dla {
  dif_dla_params_t params;

  // Other fields, if necessary.
} dif_dla_t;

/**
 * The result of a DLA operation.
 */
typedef enum dif_dla_result {
  /**
   * Indicates that the operation succeeded.
   */
  kDifDlaOk = 0,
  /**
   * Indicates some unspecified failure.
   */
  kDifDlaError = 1,
  /**
   * Indicates that some parameter passed into a function failed a
   * precondition.
   *
   * When this value is returned, no hardware operations occured.
   */
  kDifDlaBadArg = 2,
  /**
   * Indicates that this operation has been locked out, and can never
   * succeed until hardware reset.
   */
  // Remove this variant if you don't need it.
  kDifDlaLocked = 3,
} dif_dla_result_t;


/**
 * Parameters for a DLA transaction.
 *//*
typedef struct dif_dla_transaction {
  // Your fields here.
} dif_dla_transaction_t;
*/
/**
 * An output location for a DLA transaction.
 *//*
typedef struct dif_dla_output {
  // Your fields here.
} dif_dla_output_t;
*/
/**
 * A DLA interrupt request type.
 */
typedef enum dif_dla_irq {
  kDifVecDotInterruptDone = 0,
} dif_dla_irq_t;

/**
 * A snapshot of the enablement state of the interrupts for DLA.
 *
 * This is an opaque type, to be used with the `dif_dla_irq_disable_all()` and
 * `dif_dla_irq_restore_all()` functions.
 */
typedef uint32_t dif_dla_irq_snapshot_t;

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
uint32_t dif_dla_get_size(dif_dla_params_t params);

/**
 * Creates a new handle for DLA.
 *
 * This function does not actuate the hardware.
 *
 * @param params Hardware instantiation parameters.
 * @param[out] handle Out param for the initialized handle.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_dla_result_t dif_dla_init(dif_dla_params_t params,
                                              dif_dla_t *handle);


//user-defined
dif_dla_result_t dif_dla_init_ddr(const dif_dla_t *dla);
dif_dla_result_t dif_dla_wait_for_done(const dif_dla_t *dla);
dif_dla_result_t dif_dla_set_ppe(const dif_dla_t *dla, uint32_t row_en, uint32_t col_en);
dif_dla_result_t dif_dla_ddr2gb(const dif_dla_t *dla, uint32_t len, uint32_t addr0, uint32_t addr1,
                    uint32_t gb_addr, uint32_t gb_idx, uint32_t gb_mux, uint32_t direction);
dif_dla_result_t dif_dla_fbuf2lbuf(const dif_dla_t *dla, uint32_t src_addr, uint32_t dest_addr,
                    uint32_t skip, uint32_t iter, uint32_t len, uint32_t dila, uint32_t mode);
dif_dla_result_t dif_dla_ppe_reset(const dif_dla_t *dla);
dif_dla_result_t dif_dla_conv(const dif_dla_t *dla, 
                uint32_t mode_spar, uint32_t k_scale, uint32_t k_size,
                uint32_t if_len, uint32_t of_len, uint32_t if_chl, uint32_t of_chl, 
                uint32_t pad_left, uint32_t pad_right, uint32_t pad_num, uint32_t sub_col,
                uint32_t sub_row, uint32_t lbuf_addr, uint32_t wbuf_addr, uint32_t ibuf_addr);
dif_dla_result_t dif_dla_ppe_psum_accum(const dif_dla_t *dla, uint32_t acc_len, uint32_t row_num);
dif_dla_result_t dif_dla_ppe_comp(const dif_dla_t *dla, uint32_t elem, uint32_t bias, 
                    uint32_t pass, uint32_t row_num, uint32_t len, uint32_t iter, uint32_t post,
                    uint32_t mode, uint32_t oper, uint32_t fbuf_src, uint32_t fbuf_dest,
                    uint32_t abuf_src, uint32_t src_dila, uint32_t dest_dila, 
                    uint32_t src_skip, uint32_t dest_skip);
dif_dla_result_t dif_dla_read_ddr(const dif_dla_t *dla,
                                  uint32_t offset_bytes, void *dest, size_t len_bytes );

//ddr ctrl
dif_dla_result_t dif_dla_ddr_ctrl_init_calib(const dif_dla_t *dla);
dif_dla_result_t dif_dla_ddr_ctrl_write_buf(const dif_dla_t *dla, uint32_t ddr_start_addr, void *src, size_t len_bytes);
dif_dla_result_t dif_dla_ddr_ctrl_read(const dif_dla_t *dla, uint32_t ddr_start_addr, void *dest, size_t len_bytes, uint32_t* wptr);
dif_dla_result_t dif_dla_ddr_ctrl_wptr(const dif_dla_t *dla, uint32_t* wptr);


/**
 * Configures DLA with runtime information.
 *
 * This function should need to be called once for the lifetime of `handle`.
 *
 * @param handle A DLA handle.
 * @param config Runtime configuration parameters.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_dla_result_t dif_dla_configure(const dif_dla_t *handle,
                                                   dif_dla_config_t config);

/**
 * Begins a DLA transaction.
 *
 * Each call to this function should be sequenced with a call to
 * `dif_dla_end()`.
 *
 * @param handle A DLA handle.
 * @param transaction Transaction configuration parameters.
 * @return The result of the operation.
 */
/*
DIF_WARN_UNUSED_RESULT
dif_dla_result_t dif_dla_start(const dif_dla_t *handle,
                                               dif_dla_transaction_t transaction);
*/
/** Ends a DLA transaction, writing the results to the given output..
 *
 * @param handle A DLA handle.
 * @param output Transaction output parameters.
 * @return The result of the operation.
 *//*
DIF_WARN_UNUSED_RESULT
dif_dla_result_t dif_dla_end(const dif_dla_t *handle,
                                             dif_dla_output_t output);
*/
/**
 * Locks out DLA functionality.
 *
 * This function is reentrant: calling it while functionality is locked will
 * have no effect and return `kDifDlaOk`.
 *
 * @param handle A DLA handle.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_dla_result_t dif_dla_lock(const dif_dla_t *handle);

/**
 * Checks whether this DLA is locked.
 *
 * @param handle A DLA handle.
 * @param[out] is_locked Out-param for the locked state.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_dla_result_t dif_dla_is_locked(const dif_dla_t *handle,
                                                   bool *is_locked);

/**
 * Returns whether a particular interrupt is currently pending.
 *
 * @param handle A DLA handle.
 * @param irq An interrupt type.
 * @param[out] is_pending Out-param for whether the interrupt is pending.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_dla_result_t dif_dla_irq_is_pending(const dif_dla_t *handle,
                                                        dif_dla_irq_t irq,
                                                        bool *is_pending);

/**
 * Acknowledges a particular interrupt, indicating to the hardware that it has
 * been successfully serviced.
 *
 * @param handle A DLA handle.
 * @param irq An interrupt type.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_dla_result_t dif_dla_irq_acknowledge(const dif_dla_t *handle,
                                                         dif_dla_irq_t irq);

/**
 * Checks whether a particular interrupt is currently enabled or disabled.
 *
 * @param handle A DLA handle.
 * @param irq An interrupt type.
 * @param[out] state Out-param toggle state of the interrupt.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_dla_result_t dif_dla_irq_get_enabled(const dif_dla_t *handle,
                                                         dif_dla_irq_t irq,
                                                         dif_dla_toggle_t *state);

/**
 * Sets whether a particular interrupt is currently enabled or disabled.
 *
 * @param handle A DLA handle.
 * @param irq An interrupt type.
 * @param state The new toggle state for the interrupt.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_dla_result_t dif_dla_irq_set_enabled(const dif_dla_t *handle,
                                                         dif_dla_irq_t irq,
                                                         dif_dla_toggle_t state);

/**
 * Forces a particular interrupt, causing it to be serviced as if hardware had
 * asserted it.
 *
 * @param handle A DLA handle.
 * @param irq An interrupt type.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_dla_result_t dif_dla_irq_force(const dif_dla_t *handle,
                                                   dif_dla_irq_t irq);

/**
 * Disables all interrupts, optionally snapshotting all toggle state for later
 * restoration.
 *
 * @param handle A DLA handle.
 * @param[out] snapshot Out-param for the snapshot; may be `NULL`.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_dla_result_t dif_dla_irq_disable_all(const dif_dla_t *handle,
                                                         dif_dla_irq_snapshot_t *snapshot);

/**
 * Restores interrupts from the given snapshot.
 *
 * This function can be used with `dif_dla_irq_disable_all()` to temporary
 * interrupt save-and-restore.
 *
 * @param handle A DLA handle.
 * @param snapshot A snapshot to restore from.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_dla_result_t dif_dla_irq_restore_all(const dif_dla_t *handle,
                                                         const dif_dla_irq_snapshot_t *snapshot);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // OPENTITAN_SW_DEVICE_LIB_DIF_DIF_DLA_H_
