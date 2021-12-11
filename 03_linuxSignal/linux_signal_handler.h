
/**
 * @copyright Copyright
 *
 * No duplications, whole or partial, manual or electronic, may be made
 * without express written permission. Any such copies, or revisions thereof,
 * must display this notice unaltered.
 *
 * @brief signal handler
 * @author
 * @date 2021-06-16
 */
#ifndef SRC_COMMON_LINUX_SIGNAL_HANDLER_H_
#define SRC_COMMON_LINUX_SIGNAL_HANDLER_H_

namespace lidiag {
void register_sig_segfault_handler(void);
}  // namespace lidiag

#endif  // SRC_COMMON_LINUX_SIGNAL_HANDLER_H_
