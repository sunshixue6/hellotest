#ifndef MYSUBJECT_IMPL_H_
#define MYSUBJECT_IMPL_H_

#include "observe_common.h"
#include "myobserve_impl.h"
#include <vector>

namespace litest {

enum class MYState
{
    connect,    // 0
    disconnect, // 1
    end = 100,  // = 100   
    defaultVal  /* = 101 */
};

class MySubject {
public:
    MySubject();
    void notifyInfo( int status);
    void changeStatus(const MYState& refStatus);
    void attach(MyObseveImpl& observeCom);

private:
    MYState mStatus;
    std::vector<MyObseveImpl> mObserveList;

};

} //namespace litest

#endif