#include <uuid/uuid.h>  // NOLINT

#include <cstdint>
#include <cstring>
#include <string>
#include <iostream>

#include "Identity.h"

namespace litest {

Identity::Identity(bool need_generate) {
  std::memset(data_, 0, ID_SIZE);
  if (need_generate) {
    uuid_t uuid;
    uuid_generate(uuid);
    std::memcpy(data_, uuid, ID_SIZE);
  }
}
Identity::~Identity() {}

bool Identity::operator ==(const Identity& refIdentity) const
{
  return ((std::memcmp(data_, refIdentity.data_, ID_SIZE) == 0));
}

void Identity::dumpUUid()
{
    std::cout<<"UUid is:";
    int i = 0;
    for(i = 0; i < ID_SIZE; i++)
    {
        std::cout<< (data_[i] & 0xff);
    }
    std::cout<<std::endl;
}


} //namespace litest
