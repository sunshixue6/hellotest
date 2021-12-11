#include "linux_signal_handler.h"
#include "common_debug.h"
#include <thread>

int main(int argc, char **argv) {

    LOGI("main begin test");
    lidiag::register_sig_segfault_handler();
    int test = 5;
    int num = 0;
    while (true) {
        LOGD("mainloop: %d\n", num);
        std::this_thread::sleep_for(std::chrono::seconds(5));
        num += 1;
        if (num == 2)
        {
            num = num / 0;
        }
        if (num == 3)
        {
            break;
        }
    }
    LOGI("main finish test");
    return 0;
}
