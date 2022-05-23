/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2017-2018 Intel Corporation
 */

/*! @file      dlb_perf.h
 *
 *  @brief     DLB Performance Monitoring Client API
 */

#ifndef __DLB_PERF_H__
#define __DLB_PERF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "dlb_common.h"

/** Performance monitoring measurement group 0 */
struct dlb_perf_mon_group_0 {
  /** Number of transactions enqueued at the IOSF-System interface. */
  uint32_t dlb_iosf_to_sys_enq_count;
  /** Number of transactions dequeued at the IOSF-System interface. */
  uint32_t dlb_sys_to_iosf_deq_count;
  /** Number of transactions enqueued at the System-DLB interface. */
  uint32_t dlb_sys_to_dlb_enq_count;
  /** Number of transactions dequeud at the System-DLB interface. */
  uint32_t dlb_dlb_to_sys_deq_count;
};

/** Performance monitoring measurement group 1 */
struct dlb_perf_mon_group_1 {
  /** Number of credit push pointer update operations performed. */
  uint32_t dlb_push_ptr_update_count;
};

/** Performance monitoring measurement group 2 */
struct dlb_perf_mon_group_2 {
  /** Average depth of all enabled load-balanced CQ's history lists (which
   * contain the load-balanced event state entries).
   */
  uint32_t dlb_avg_hist_list_depth;
};

/** Performance monitoring measurement group 3 */
struct dlb_perf_mon_group_3 {
  /** Average load-balanced queue depth. Note that atomic QEs are located
   * in a smaller, separate storage area before being scheduled, and that
   * storage is not accounted for here.
   */
  uint32_t dlb_avg_qed_depth;
};

/** Performance monitoring measurement group 4 */
struct dlb_perf_mon_group_4 {
  /** Average directed queue depth. */
  uint32_t dlb_avg_dqed_depth;
};

/** Performance monitoring measurement group 5 */
struct dlb_perf_mon_group_5 {
  /** Number of NOOP HCWs. */
  uint32_t dlb_noop_hcw_count;
  /** Number of batch token return HCWs. */
  uint32_t dlb_bat_t_hcw_count;
};

/** Performance monitoring measurement group 6 */
struct dlb_perf_mon_group_6 {
  /** Number of release HCWs. */
  uint32_t dlb_comp_hcw_count;
  /** Number of release + CQ pop HCWs. */
  uint32_t dlb_comp_t_hcw_count;
};

/** Performance monitoring measurement group 7 */
struct dlb_perf_mon_group_7 {
  /** Number of new HCWs. */
  uint32_t dlb_enq_hcw_count;
  /** Number of new + CQ pop HCWs. */
  uint32_t dlb_enq_t_hcw_count;
};

/** Performance monitoring measurement group 8 */
struct dlb_perf_mon_group_8 {
  /** Number of forward HCWs. */
  uint32_t dlb_renq_hcw_count;
  /** Number of forward + CQ pop HCWs. */
  uint32_t dlb_renq_t_hcw_count;
};

/** Performance monitoring measurement group 9 */
struct dlb_perf_mon_group_9 {
  /** Number of QID inflight release HCWs (note: this HCW is not used by this
   * library).
   */
  uint32_t dlb_rel_hcw_count;
};

/** Performance monitoring measurement group 10 */
struct dlb_perf_mon_group_10 {
  /** Number of fragment HCWs (note: this HCW is not used by this library). */
  uint32_t dlb_frag_hcw_count;
  /** Number of fragment + CQ pop HCWs. (note: this HCW is not used by this
   * library).
   */
  uint32_t dlb_frag_t_hcw_count;
};

/**
 * Performance monitoring measurement structure
 */
union dlb_perf_mon_group {
  /** Performance monitoring measurement group 0 */
  struct dlb_perf_mon_group_0 group_0;
  /** Performance monitoring measurement group 1 */
  struct dlb_perf_mon_group_1 group_1;
  /** Performance monitoring measurement group 2 */
  struct dlb_perf_mon_group_2 group_2;
  /** Performance monitoring measurement group 3 */
  struct dlb_perf_mon_group_3 group_3;
  /** Performance monitoring measurement group 4 */
  struct dlb_perf_mon_group_4 group_4;
  /** Performance monitoring measurement group 5 */
  struct dlb_perf_mon_group_5 group_5;
  /** Performance monitoring measurement group 6 */
  struct dlb_perf_mon_group_6 group_6;
  /** Performance monitoring measurement group 7 */
  struct dlb_perf_mon_group_7 group_7;
  /** Performance monitoring measurement group 8 */
  struct dlb_perf_mon_group_8 group_8;
  /** Performance monitoring measurement group 9 */
  struct dlb_perf_mon_group_9 group_9;
  /** Performance monitoring measurement group 10 */
  struct dlb_perf_mon_group_10 group_10;
};

/*!
 * @fn int dlb_monitor_perf(
 *          dlb_hdl_t hdl,
 *          int perf_mon_group_id,
 *          union dlb_perf_mon_group *data,
 *          uint32_t duration_us,
 *          uint32_t *elapsed_us);
 *
 * @brief   Measure a set of performance metrics for a user-specified duration
 *          while the DLB is operating. Certain metrics cannot be measured
 *          concurrently; hence, the metrics are put into different measurement
 *          groups.
 *
 * @note    This function is blocking (the thread sleeps in the kernel driver
 *          for the duration of the measurement). Also the kernel driver
 *          serializes performance monitoring requests, such that only one
 *          group can be measured at any time.
 *
 * @note    VF devices cannot measure device-global performance counters, and
 *          will cause the function to return an error.
 *
 * @note    The measurement counter saturates at 2^32-1; if measurement
 *          duration is large enough that the counter reaches its limit,
 *          elapsed_us will not reflect the point at which the counter stopped
 *          and shouldn't be used to derive a rate.
 *
 * @param[in] hdl: Handle returned by a successful call to dlb_open().
 * @param[in] perf_mon_group_id: ID of the set of performance metrics the user
 *                  wants to measure.
 * @param[out] data: Pointer to perfomance monitoring data, populated by the
 *                  library.
 * @param[in] duration_us: Measurement duration, in microseconds. Maximum
 *                  allowed duration is 60 seconds.
 * @param[out] elapsed_us: Pointer to the actual measurement duration, in
 *                  microseconds, populated by the library. Thread scheduling,
 *                  interrupts, and other kernel interface can cause the actual
 *                  duration to differ from the requested duration. This value
 *                  should be used when calculating rates.
 *
 * @retval  0 Success
 * @retval -1 Failure, and errno is set according to the exceptions listed below
 * @exception EINVAL Invalid DLB handle, data pointer, or elapsed_us pointer
 * @exception EINVAL duration_us is 0 or greater than 60000000 (60 seconds)
 * @exception EINVAL Attempted to call function on a VF device
 * @exception EBUSY  The measurement hardware is currently in use
 */
int dlb_monitor_perf(dlb_hdl_t hdl, int perf_mon_group_id, union dlb_perf_mon_group* data,
                     uint32_t duration_us, uint32_t* elapsed_us);

/** DLB port scheduling counts */
struct dlb_perf_sched_counts {
  /** Number of events scheduled to load-balanced ports. */
  uint64_t ldb_sched_count;
  /** Number of events scheduled to directed ports. */
  uint64_t dir_sched_count;
  /** Number of events scheduled to individual load-balanced ports. */
  uint64_t ldb_cq_sched_count[64];
  /** Number of events scheduled to individual directed ports. */
  uint64_t dir_cq_sched_count[128];
};

/*!
 * @fn int dlb_measure_sched_counts(
 *          dlb_hdl_t hdl,
 *          struct dlb_perf_sched_counts *data,
 *          uint32_t duration_us,
 *          uint32_t *elapsed_us);
 *
 * @brief   Measure the device's scheduling counts over a user-specified
 *          duration.
 *
 * @note    This function is blocking (the thread sleeps in the kernel driver
 *          for the duration of the measurement).
 *
 * @note    VF devices cannot measure global scheduling counts ldb_sched_count
 *          and dir_sched_count, and their values will be set to zero.
 *
 * @param[in] hdl: Handle returned by a successful call to dlb_open().
 * @param[out] data: Pointer to perfomance monitoring data, populated by the
 *                  library.
 * @param[in] duration_us: Measurement duration, in microseconds. Maximum
 *                  allowed duration is 60 seconds.
 * @param[out] elapsed_us: Pointer to the actual measurement duration, in
 *                  microseconds, populated by the library. Thread scheduling,
 *                  interrupts, and other kernel interface can cause the actual
 *                  duration to differ from the requested duration. This value
 *                  should be used when calculating rates.
 *
 * @retval  0 Success
 * @retval -1 Failure, and errno is set according to the exceptions listed below
 * @exception EINVAL Invalid DLB handle, data pointer, or elapsed_us pointer
 * @exception EINVAL duration_us is 0 or greater than 60000000 (60 seconds)
 */
int dlb_measure_sched_counts(dlb_hdl_t hdl, struct dlb_perf_sched_counts* data,
                             uint32_t duration_us, uint32_t* elapsed_us);

#ifdef __cplusplus
}
#endif

#endif /* __DLB_PERF_H__ */
