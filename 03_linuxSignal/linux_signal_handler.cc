
/**
 * @copyright Copyright 2021
 *
 * No duplications, whole or partial, manual or electronic, may be made
 * without express written permission. Any such copies, or revisions thereof,
 * must display this notice unaltered.
 *
 * @brief signal handler
 * @author
 * @date 2021-06-16
 */
#include "linux_signal_handler.h"

#include <dlfcn.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <linux/unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ucontext.h>
#include <unistd.h>

#include "common_debug.h"

namespace lidiag {

// NXP s32g /opt/xcu-fsl-auto/1.0/sysroots/aarch64-fsl-linux/usr/include/sys/ucontext-64.h
#if 0
/* Context to describe whole processor state.  This only describes
   the core registers; coprocessor registers get saved elsewhere
   (e.g. in uc_regspace, or somewhere unspecified on the stack
   during non-RT signal handlers).  */
typedef struct {
  unsigned long long int __ctx(fault_address); // NOLINT
  unsigned long long int __ctx(regs)[31]; // NOLINT
  unsigned long long int __ctx(sp); // NOLINT
  unsigned long long int __ctx(pc); // NOLINT
  unsigned long long int __ctx(pstate); // NOLINT
  /* This field contains extension records for additional processor
     state such as the FP/SIMD state.  It has to match the definition
     of the corresponding field in the sigcontext struct, see the
     arch/arm64/include/uapi/asm/sigcontext.h linux header for details.  */
  unsigned char __reserved[4096] __attribute__((__aligned__(16))); // NOLINT
} mcontext_t;

/* Userlevel context.  */
typedef struct ucontext_t {
  unsigned long __ctx(uc_flags); // NOLINT
  struct ucontext_t *uc_link;
  stack_t uc_stack;
  sigset_t uc_sigmask;
  mcontext_t uc_mcontext;
} ucontext_t;
#endif

#ifdef __aarch64__
int backtrace_arm64_AAPCS64(void **array, int size) {
  if (size <= 0) return 0;
  int cnt = 0;
  int ret = 0;
  unsigned long *backsp = 0;  // NOLINT
  asm volatile("MOV %x[result], x29\n\r" : [ result ] "=r"(backsp));
  while ((cnt <= size) && (0 != *backsp)) {
    array[cnt++] = (void *)(*(backsp + 1));  // NOLINT
    backsp = (unsigned long *)(*backsp);     // NOLINT
  }
  array[cnt++] = (void *)(*(backsp + 1));  // NOLINT
  ret = ((cnt <= size) ? cnt : size);
  return ret;
}

#define WORD_WIDTH 8
char **backtrace_symbols_ce123(void *const *array, int size) {
  Dl_info info[size];  // NOLINT
  int status[size];    // NOLINT
  int cnt;
  size_t total = 0;
  static char s_tmp[4096] = {0};
  char **result = (char **)s_tmp;  // NOLINT

  /* Fill in the information we can get from `dladdr'.  */
  for (cnt = 0; cnt < size; ++cnt) {
    status[cnt] = dladdr(array[cnt], &info[cnt]);
    if (status[cnt] && info[cnt].dli_fname && info[cnt].dli_fname[0] != '\0')
      /* We have some info, compute the length of the string which will be
      "<file-name>(<sym-name>) [+offset].  */
      total += (strlen(info[cnt].dli_fname ?: "") +
                (info[cnt].dli_sname ? strlen(info[cnt].dli_sname) + 3 + WORD_WIDTH + 3 : 1) + WORD_WIDTH + 5);
    else
      total += 5 + WORD_WIDTH;
  }
  /* Allocate memory for the result.  */
  if (size * sizeof(char *) + total < 4096) {
    char *last = (char *)(result + size);  // NOLINT

    for (cnt = 0; cnt < size; ++cnt) {
      result[cnt] = last;

      if (status[cnt] && info[cnt].dli_fname && info[cnt].dli_fname[0] != '\0') {
        char buf[20];

        if (array[cnt] >= (void *)info[cnt].dli_saddr) {                                   // NOLINT
          char *tmp = (char *)info[cnt].dli_saddr;                                         // NOLINT
          snprintf(buf, sizeof(buf), "+%#lx", (unsigned long)((char *)array[cnt] - tmp));  // NOLINT
        } else {
          char *tmp = (char *)info[cnt].dli_saddr;                                         // NOLINT
          snprintf(buf, sizeof(buf), "-%#lx", (unsigned long)(tmp - (char *)array[cnt]));  // NOLINT
        }

        last +=
            1 + sprintf(last, "%s%s%s%s%s[%p]", info[cnt].dli_fname ?: "", info[cnt].dli_sname ? "(" : "",  // NOLINT
                        info[cnt].dli_sname ?: "", info[cnt].dli_sname ? buf : "",                          // NOLINT
                        info[cnt].dli_sname ? ") " : " ",                                                   // NOLINT
                        array[cnt]);                                                                        // NOLINT
      } else                                                                                                // NOLINT
        last += 1 + sprintf(last, "[%p]", array[cnt]);                                                      // NOLINT
    }
    // assert (last <= (char *) result + size * sizeof (char *) + total);
  }
  return result;
}

#define SIZE 100
void signal_backtrace(siginfo_t *si, void *ptr) {
  LOGI("Get Signal (%d) At: %p\n", si->si_signo, si->si_addr);
  struct ucontext_t *ucontext = (struct ucontext_t *)ptr;
  void *pc_array[1];
  pc_array[0] = (void *)ucontext->uc_mcontext.pc;  // nxp s32g // NOLINT
  char **pc_name = backtrace_symbols_ce123(pc_array, 1);
  LOGI("%d: %s\n", 0, *pc_name);

  void *array[SIZE];
  int size, i;
  char **strings;
  size = backtrace_arm64_AAPCS64(array, SIZE);
  LOGI("Backstrace (%d deep)\n", size);

  strings = backtrace_symbols_ce123(array, size);
  for (i = 0; i < size; i++) {
    LOGI("%d: %s\n", i + 1, strings[i]);
  }
}
#endif

static void signal_action(int sig, siginfo_t *info, void *p) {
  int iRet = 0;
  Dl_info stDlInfo;
  void *pc = 0;
  static int s32SigFlag = 0;

  s32SigFlag++;
  if (s32SigFlag > 1) {
    if (s32SigFlag > 3 && SIGINT == sig) _exit(1);
    LOGI("Signal produce by signal action,return now sig [%d]!thread=%d\n", sig, getpid());
    return;
  }
  LOGI("Received signal %d ...\n", sig);

  FILE *mapfd = fopen("/proc/self/maps", "r");
  if (mapfd != NULL) {
    char buf[1024] = {0};
    while (fgets(buf, sizeof(buf), mapfd) != NULL) {
      LOGI("%s", buf);
    }
    fclose(mapfd);
  }

#ifdef __aarch64__
  ucontext_t *pstUcontext = (ucontext_t *)p;  // NOLINT
  LOGI("fault_address      : %llX\n", pstUcontext->uc_mcontext.fault_address);
  LOGI("sp                 : %llX\n", pstUcontext->uc_mcontext.sp);
  LOGI("pc                 : %llX\n", pstUcontext->uc_mcontext.pc);
  LOGI("pstate             : %llX\n", pstUcontext->uc_mcontext.pstate);
  for (int i = 0; i < 31; i++) {
    LOGI("regs[%d]            : %llX\n", i, pstUcontext->uc_mcontext.regs[i]);
  }
  pc = (void *)pstUcontext->uc_mcontext.pc;  // NOLINT
#endif

  iRet = dladdr(pc, &stDlInfo);
  if (iRet != 0) {
    LOGI("\nNO.%d;P-%d;GP-%d;T-%lu;PC-%p<>FNC-%s;FN-%s;RelativeAddr:%d--%d.time:%d\n", sig, info->si_pid,  // NOLINT
         getpid(),                                                                                         // NOLINT
         syscall(__NR_gettid), pc, stDlInfo.dli_sname, stDlInfo.dli_fname,                                 // NOLINT
         (unsigned)((char *)pc - (char *)stDlInfo.dli_saddr), iRet, (int)time(NULL));                      // NOLINT
  } else {
    LOGI("\nNO.%d;P-%d;GP-%d;T-%lu;PC-%p<add_er>FNC-%s;FN-%s;RelativeAddr:%d--%d.time:%d\n", sig,  // NOLINT
         info->si_pid,                                                                             // NOLINT
         getpid(), syscall(__NR_gettid), pc, stDlInfo.dli_sname, stDlInfo.dli_fname,               // NOLINT
         (unsigned)((char *)pc - (char *)stDlInfo.dli_saddr), iRet, (int)time(NULL));              // NOLINT
  }
  // if (SIGINT != sig) {
#ifdef __aarch64__
  signal_backtrace(info, p);
#else
  void *array[256];
  int cnt;
  char **messages;
  LOGI("Backtrace:\n");
  cnt = backtrace(array, 256);
  LOGI("backtrace() returned %d addresses\n", cnt);

  messages = backtrace_symbols(array, cnt);
  if (messages) {
    for (int i = 0; i < cnt && messages != NULL; ++i) {
      LOGI("[bt]: (%d) %s\n", i, messages[i]);
    }
    free(messages);
  }
#endif
  //}
  LOGI("_exit(1)\n");
  usleep(500 * 1000);
  _exit(1);
}

void register_sig_segfault_handler() {
  LOGI("register_sig_segfault_handler begin");
  struct sigaction act;

  sigset_t *mask = &act.sa_mask;
  act.sa_flags = SA_SIGINFO;
  act.sa_sigaction = signal_action;

  sigemptyset(mask);

#if 1  // ndef COMPILE_DEBUG
  sigaddset(mask, SIGABRT);
  sigaddset(mask, SIGSEGV);
#endif
  sigaddset(mask, SIGHUP);
  sigaddset(mask, SIGQUIT);
  sigaddset(mask, SIGILL);
  sigaddset(mask, SIGTRAP);
  sigaddset(mask, SIGIOT);
  sigaddset(mask, SIGBUS);
  sigaddset(mask, SIGFPE);

  sigaddset(mask, SIGINT);

#if 1  // ndef COMPILE_DEBUG
  sigaction(SIGABRT, &act, NULL);
  sigaction(SIGSEGV, &act, NULL);
#endif
  sigaction(SIGHUP, &act, NULL);
  sigaction(SIGQUIT, &act, NULL);
  sigaction(SIGILL, &act, NULL);
  sigaction(SIGTRAP, &act, NULL);
  sigaction(SIGIOT, &act, NULL);
  sigaction(SIGBUS, &act, NULL);
  sigaction(SIGFPE, &act, NULL);
  sigaction(SIGINT, &act, NULL);

  sigset_t signal_mask;
  sigemptyset(&signal_mask);
  sigaddset(&signal_mask, SIGPIPE);
  pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
}
}  // namespace lidiag
