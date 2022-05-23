/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2017-2018 Intel Corporation
 */

#ifndef __DLB_OPS_H__
#define __DLB_OPS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* CPU feature enumeration macros */
#define CPUID_DIRSTR_BIT 27
#define CPUID_DIRSTR64B_BIT 28
#define CPUID_UMWAIT_BIT 5

static bool movdir64b_supported(void) {
  unsigned int rax, rbx, rcx, rdx;

  asm volatile("cpuid\t\n" : "=a"(rax), "=b"(rbx), "=c"(rcx), "=d"(rdx) : "a"(7), "c"(0));

  return rcx & (1 << CPUID_DIRSTR64B_BIT);
}

static void dlb_movntdq(struct dlb_enqueue_qe* qe4, uint64_t* pp_addr) {
  /* Move entire 64B cache line of QEs, 128 bits (16B) at a time. */
  long long* _qe = (long long*)qe4;
  __v2di srcData0 = (__v2di){_qe[0], _qe[1]};
  __v2di srcData1 = (__v2di){_qe[2], _qe[3]};
  __v2di srcData2 = (__v2di){_qe[4], _qe[5]};
  __v2di srcData3 = (__v2di){_qe[6], _qe[7]};
  __builtin_ia32_movntdq((__v2di*)pp_addr, (__v2di)srcData0);
  _mm_sfence();
  __builtin_ia32_movntdq((__v2di*)pp_addr, (__v2di)srcData1);
  _mm_sfence();
  __builtin_ia32_movntdq((__v2di*)pp_addr, (__v2di)srcData2);
  _mm_sfence();
  __builtin_ia32_movntdq((__v2di*)pp_addr, (__v2di)srcData3);
  /* movntdq requires an sfence between writes to the PP MMIO address */
  _mm_sfence();
}

static void dlb_movdir64b(struct dlb_enqueue_qe* qe4, uint64_t* pp_addr) {
  /* TODO: Change to proper assembly when compiler support available */
  asm volatile(".byte 0x66, 0x0f, 0x38, 0xf8, 0x02" : : "a"(pp_addr), "d"(qe4));
}

/* Faster wakeup, smaller power savings */
#define DLB_UMWAIT_CTRL_STATE_CO1 1
/* Slower wakeup, larger power savings */
#define DLB_UMWAIT_CTRL_STATE_CO2 0

static bool umwait_supported(void) {
  unsigned int rax, rbx, rcx, rdx;

  asm volatile("cpuid\t\n" : "=a"(rax), "=b"(rbx), "=c"(rcx), "=d"(rdx) : "a"(7), "c"(0));

  return rcx & (1 << CPUID_UMWAIT_BIT);
}

static inline void dlb_umonitor(volatile void* addr) {
  /* TODO: Change to proper assembly when compiler support available */
  asm volatile(".byte 0xf3, 0x0f, 0xae, 0xf7\t\n" : : "D"(addr));
}

static inline void dlb_umwait(int state, uint64_t timeout) {
  uint32_t eax = timeout & UINT32_MAX;
  uint32_t edx = timeout >> 32;

  /* TODO: Change to proper assembly when compiler support available */
  asm volatile(".byte 0xf2, 0x0f, 0xae, 0xf7\t\n" : : "D"(state), "a"(eax), "d"(edx));
}

static unsigned int cpuid_max(void) {
  unsigned int rax, rbx, rcx, rdx;

  asm volatile("cpuid\t\n" : "=a"(rax), "=b"(rbx), "=c"(rcx), "=d"(rdx) : "a"(0));

  return rax;
}

static unsigned int cpuid_tsc_freq(void) {
  unsigned int rax, rbx, rcx, rdx;

  asm volatile("cpuid\t\n" : "=a"(rax), "=b"(rbx), "=c"(rcx), "=d"(rdx) : "a"(0x15));

  return (rbx && rcx) ? rcx * (rbx / rax) : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* __DLB_OPS_H__ */
