/**
 * @copyright Copyright 2022 Li . All Rights Reserved.
 *
 * No duplications, whole or partial, manual or electronic, may be made
 * without express written permission. Any such copies, or revisions thereof,
 * must display this notice unaltered.
 *
 * @brief test
 * @author ssx
 * @date 2022-02-12
 */
#include <iostream>
#include "myobserve_impl.h"

namespace litest {

MyObseveImpl::MyObseveImpl(int id)
{
    this->idNum = id;
}

void MyObseveImpl::updateInfo(int status)
{
    std::cout << "MyObseve id:" << this->idNum << " update status: " << status << std::endl;
}


} //namespace litest