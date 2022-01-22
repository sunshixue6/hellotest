#include "Identity.h"
#include <memory>
#include <iostream>

int main(int argc, char **argv) {

    std::shared_ptr<litest::Identity> identity_instance1 = std::make_shared<litest::Identity>();
    identity_instance1->dumpUUid();

    std::shared_ptr<litest::Identity> identity_instance2 = std::make_shared<litest::Identity>();
    identity_instance2->dumpUUid();

    std::shared_ptr<litest::Identity> identity_instance3 =  identity_instance1;

    std::cout << "identity_instance 1 and 2 equal check result is:"  << (*identity_instance1 == *identity_instance2) << std::endl;
    std::cout << "identity_instance 1 and 3 equal check result is:"  << (*identity_instance1 == *identity_instance3) << std::endl;
    return 0;
}
