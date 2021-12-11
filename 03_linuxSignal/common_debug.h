/**
 * @copyright Copyright 2021 Li Auto Inc. All Rights Reserved.
 *
 * No duplications, whole or partial, manual or electronic, may be made
 * without express written permission. Any such copies, or revisions thereof,
 * must display this notice unaltered.
 *
 * @brief
 * @author Dash Zhou (zhoupeng@lixiang.com)
 * @date 2021-05-14
 */

#ifndef INCLUDE_LIDIAG_COMMON_DEBUG_H_
#define INCLUDE_LIDIAG_COMMON_DEBUG_H_

#include <inttypes.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#include <cctype>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>

#define LOGD(format, args...) \
  printf("DBG  : [%s:%d] " format "\r\n", __func__, __LINE__, ##args)

#define LOGI(format, args...) \
  printf("INFO : [%s:%d] " format "\r\n", __func__, __LINE__, ##args)

#define LOGW(format, args...) \
  printf("WARN : [%s:%d] " format "\r\n", __func__, __LINE__, ##args)

#define LOGE(format, args...) \
  printf("ERROR : [%s:%d] " format "\r\n", __func__, __LINE__, ##args)

#define LOGF(format, args...) \
  printf("FATAL : [%s:%d] " format "\r\n", __func__, __LINE__, ##args)

#if 0
#define LOGD(format, args...) \
  printf("DBG  : %s:[%s:%d:%lu] " format "\r\n", time(), __func__, __LINE__, syscall(__NR_gettid), ##args)
#define LOGI(format, args...) \
  printf("INFO : %s:[%s:%d:%lu] " format "\r\n", time(), __func__, __LINE__, syscall(__NR_gettid), ##args)
#define LOGW(format, args...) \
  printf(YELLOW "WARN : %s:[%s:%d:%lu] " format NON "\r\n", time(), __func__, __LINE__, syscall(__NR_gettid), ##args)
#define LOGE(format, args...) \
  printf(RED "ERROR: %s:[%s:%d:%lu] " format NON "\r\n", time(), __func__, __LINE__, syscall(__NR_gettid), ##args)
#define LOGF(format, args...)                                                                                 \
  do {                                                                                                        \
    printf("FATAL: %s:[%s:%d:%lu] " format "\r\n", time(), __func__, __LINE__, syscall(__NR_gettid), ##args); \
    assert(0);                                                                                                \
  } while (0);
#endif

#endif  // INCLUDE_LIDIAG_COMMON_DEBUG_H_
