#ifndef MYOBSERVE_IMPL_H_
#define MYOBSERVE_IMPL_H_
#include "observe_common.h"

namespace litest {
class MyObseveImpl: ObserveCommon {
public:    
    MyObseveImpl(int id);
    void updateInfo(int status) override;
private:
    int idNum;
};


} //namespace litest

#endif //MYOBSERVE_IMPL_H_