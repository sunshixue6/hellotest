#include "myobserve_impl.h"
#include "mysubject.h"
#include <memory>
#include <iostream>

int main(int argc, char **argv) {

    litest::MyObseveImpl myObseveImpl(1);
    litest::MyObseveImpl myObseveImpl2(2);

    std::shared_ptr<litest::MySubject> mySubject = std::make_shared<litest::MySubject>();
    mySubject->attach(myObseveImpl);
    mySubject->attach(myObseveImpl2);
    mySubject->changeStatus(litest::MYState::disconnect);

    return 0;
}