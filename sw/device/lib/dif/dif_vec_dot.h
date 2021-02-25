// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef OPENTITAN_SW_DEVICE_LIB_DIF_DIF_VEC_DOT_H_
#define OPENTITAN_SW_DEVICE_LIB_DIF_DIF_VEC_DOT_H_


/**
 * @file
 * @brief <a href="/hw/ip/vec_dot/doc/">Vector Dot Product</a> Device Interface Functions
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
typedef enum dif_vec_dot_enable {
  /*
   * The "enabled" state.
   */
  kDifVecDotToggleEnabled,
  /**
   * The "disabled" state.
   */
  kDifVecDotToggleDisabled,
} dif_vec_dot_enable_t;

/**
 * Hardware instantiation parameters for Vector Dot Product.
 *
 * This struct describes information about the underlying hardware that is
 * not determined until the hardware design is used as part of a top-level
 * design.
 */
typedef struct dif_vec_dot_params {
  /**
   * The base address for the Vector Dot Product hardware registers.
   */
  mmio_region_t base_addr;

  // Other fields, if necessary.
} dif_vec_dot_params_t;

/**
 * Runtime configuration for Vector Dot Product.
 *
 * This struct describes runtime information for one-time configuration of the
 * hardware.
 */
typedef struct dif_vec_dot_config {
  /** Base address of the Vector Dot Product device in the system. */
  mmio_region_t base_addr;
} dif_vec_dot_config_t;

/**
 * A handle to Vector Dot Product.
 *
 * This type should be treated as opaque by users.
 */
typedef struct dif_vec_dot {
  dif_vec_dot_params_t params;

  // Other fields, if necessary.
} dif_vec_dot_t;

/**
 * The result of a Vector Dot Product operation.
 */
typedef enum dif_vec_dot_result {
  /**
   * Indicates that the operation succeeded.
   */
  kDifVecDotOk = 0,
  /**
   * Indicates some unspecified failure.
   */
  kDifVecDotError = 1,
  /**
   * Indicates that some parameter passed into a function failed a
   * precondition.
   *
   * When this value is returned, no hardware operations occured.
   */
  kDifVecDotBadArg = 2,
  /**
   * Indicates that this operation has been locked out, and can never
   * succeed until hardware reset.
   */
  // Remove this variant if you don't need it.
  kDifVecDotLocked = 3,
} dif_vec_dot_result_t;


/**
 * A Vector Dot Product interrupt request type.
 */
typedef enum dif_vec_dot_irq {
  /**
   * Vector Dot Product is done, it has run the application to completion.
   *
   * Associated with the `vec_dot.INTR_STATE.done` hardware interrupt.
   */
	kDifVecDotInterruptDone = 0,
} dif_vec_dot_irq_t;

/**
 * A snapshot of the enablement state of the interrupts for Vector Dot Product.
 *
 * This is an opaque type, to be used with the `dif_vec_dot_irq_disable_all()` and
 * `dif_vec_dot_irq_restore_all()` functions.
 */
typedef uint32_t dif_vec_dot_irq_snapshot_t;

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
uint32_t dif_vec_dot_get_size(dif_vec_dot_params_t params);

/**
 * Creates a new handle for Vector Dot Product.
 *
 * This function does not actuate the hardware.
 *
 * @param params Hardware instantiation parameters.
 * @param[out] vec_dot Out param for the initialized handle.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_vec_dot_result_t dif_vec_dot_init(dif_vec_dot_params_t params,
                                              dif_vec_dot_t *vec_dot);

/**
 * Reset Vector Dot Product device.
 *
 * Resets the given Vector Dot Product  device by setting its configuration registers to
 * reset values. Disables interrupts, output, and input filter.
 *
 * @param vec_dot Vector Dot Product instance
 * @return `kDifVecDotBadArg` if `vec_dot` is `NULL`, `kDifVecDotOk` otherwise.
 */
dif_vec_dot_result_t dif_vec_dot_reset(const dif_vec_dot_t *vec_dot);


/**
 * Configures Vector Dot Product with runtime information.
 *
 * This function should need to be called once for the lifetime of `handle`.
 *
 * @param vec_dot A Vector Dot Product handle.
 * @param config Runtime configuration parameters.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_vec_dot_result_t dif_vec_dot_configure(const dif_vec_dot_t *vec_dot,
                                                   dif_vec_dot_config_t config);


/**
 * Send Vector Dot Product vectors through the registers.
 *
 * send vector data in serial with the values also in serial
 * starting at transaction.
 * i.e. transaction = 0, 
 * vector1 = [0,1,2,3] vector2 = [4,5,6,7]
 *
 * @param vec_dot A Vector Dot Product handle.
 * @param transaction Starting value of the vector values.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_vec_dot_result_t dif_vec_dot_send_vectors_reg(const dif_vec_dot_t *vec_dot,
                                                            uint32_t transaction);


/**
 * Send Vector Dot Product vectors through the buffer ram.
 *
 * send vector data in serial
 *
 * @param vec_dot A Vector Dot Product handle.
 * @param transaction Transaction configuration parameters.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_vec_dot_result_t dif_vec_dot_send_vectors_ram(const dif_vec_dot_t *vec_dot,
                                                            uint32_t transaction);


/**
 * Read from Vector Dot Product buffer ram.
 *
 * @param vec_dot A Vector Dot Product handle.
 * @param transaction Transaction configuration parameters.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_vec_dot_result_t dif_vec_dot_dmem_read(const dif_vec_dot_t *vec_dot,
                                            uint32_t offset_bytes, void *dest,
                                            size_t len_bytes);


/**
 * Selects a Vector Dot Product operation mode.
 *
 * Each call to this function should be sequenced with a call to
 * `dif_vec_dot_end()`.
 *
 * @param vec_dot A Vector Dot Product handle.
 * @param transaction Transaction configuration parameters.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_vec_dot_result_t dif_vec_dot_mode(const dif_vec_dot_t *vec_dot, uint32_t mode);


/**
 * Begins a Vector Dot Product transaction.
 *
 * @param vec_dot A Vector Dot Product handle.
 * @param transaction Transaction configuration parameters.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_vec_dot_result_t dif_vec_dot_start(const dif_vec_dot_t *vec_dot);


/**
 * Is Vector Dot Product busy executing an application?
 *
 * @param vec_dot vec_dot instance
 * @param[out] busy vec_dot is busy
 * @return `kDifVecdotBadArg` if `vec_dot` or `busy` is `NULL`,
 *         `kDifVecdotOk` otherwise.
 */
dif_vec_dot_result_t dif_vec_dot_is_busy(const dif_vec_dot_t *vec_dot, bool *busy);


/**
 * Send Vector Dot Product vectors.
 *
 * read the computed inner product
 *
 * @param vec_dot A Vector Dot Product handle.
 * @param transaction Transaction configuration parameters.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_vec_dot_result_t dif_vec_dot_read(const dif_vec_dot_t *vec_dot, uint32_t* result);



/**
 * Locks out Vector Dot Product functionality.
 *
 * This function is reentrant: calling it while functionality is locked will
 * have no effect and return `kDifVecDotOk`.
 *
 * @param handle A Vector Dot Product handle.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_vec_dot_result_t dif_vec_dot_lock(const dif_vec_dot_t *handle);

/**
 * Checks whether this Vector Dot Product is locked.
 *
 * @param handle A Vector Dot Product handle.
 * @param[out] is_locked Out-param for the locked state.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_vec_dot_result_t dif_vec_dot_is_locked(const dif_vec_dot_t *handle,
                                                   bool *is_locked);

/**
 * Returns whether a particular interrupt is currently pending.
 *
 * @param handle A Vector Dot Product handle.
 * @param irq An interrupt type.
 * @param[out] is_pending Out-param for whether the interrupt is pending.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_vec_dot_result_t dif_vec_dot_irq_is_pending(const dif_vec_dot_t *handle,
                                                        dif_vec_dot_irq_t irq,
                                                        bool *is_pending);

/**
 * Acknowledges a particular interrupt, indicating to the hardware that it has
 * been successfully serviced.
 *
 * @param handle A Vector Dot Product handle.
 * @param irq An interrupt type.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_vec_dot_result_t dif_vec_dot_irq_acknowledge(const dif_vec_dot_t *handle,
                                                         dif_vec_dot_irq_t irq);

/**
 * Checks whether a particular interrupt is currently enabled or disabled.
 *
 * @param handle A Vector Dot Product handle.
 * @param irq An interrupt type.
 * @param[out] state Out-param toggle state of the interrupt.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_vec_dot_result_t dif_vec_dot_irq_get_enabled(const dif_vec_dot_t *handle,
                                                         dif_vec_dot_irq_t irq,
                                                         dif_vec_dot_enable_t *state);

/**
 * Sets whether a particular interrupt is currently enabled or disabled.
 *
 * @param handle A Vector Dot Product handle.
 * @param irq An interrupt type.
 * @param state The new toggle state for the interrupt.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_vec_dot_result_t dif_vec_dot_irq_set_enabled(const dif_vec_dot_t *handle,
                                                         dif_vec_dot_irq_t irq,
                                                         dif_vec_dot_enable_t enable);

/**
 * Forces a particular interrupt, causing it to be serviced as if hardware had
 * asserted it.
 *
 * @param handle A Vector Dot Product handle.
 * @param irq An interrupt type.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_vec_dot_result_t dif_vec_dot_irq_force(const dif_vec_dot_t *handle,
                                                   dif_vec_dot_irq_t irq);

/**
 * Disables all interrupts, optionally snapshotting all toggle state for later
 * restoration.
 *
 * @param handle A Vector Dot Product handle.
 * @param[out] snapshot Out-param for the snapshot; may be `NULL`.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_vec_dot_result_t dif_vec_dot_irq_disable_all(const dif_vec_dot_t *handle,
                                                         dif_vec_dot_irq_snapshot_t *snapshot);

/**
 * Restores interrupts from the given snapshot.
 *
 * This function can be used with `dif_vec_dot_irq_disable_all()` to temporary
 * interrupt save-and-restore.
 *
 * @param handle A Vector Dot Product handle.
 * @param snapshot A snapshot to restore from.
 * @return The result of the operation.
 */
DIF_WARN_UNUSED_RESULT
dif_vec_dot_result_t dif_vec_dot_irq_restore_all(const dif_vec_dot_t *handle,
                                                         const dif_vec_dot_irq_snapshot_t *snapshot);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // OPENTITAN_SW_DEVICE_LIB_DIF_DIF_VEC_DOT_H_
