#ifndef OBSERVE_COMMON_H_
#define OBSERVE_COMMON_H_

namespace litest {

class ObserveCommon {
    public:
        virtual void updateInfo(int status) = 0;
};

} //namespace litest


#endif //OBSERVE_COMMON_H_