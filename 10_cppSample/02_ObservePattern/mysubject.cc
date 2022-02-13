#include "mysubject.h"
#include <iostream>
namespace litest {

MySubject::MySubject()
{
    mStatus = MYState::end;
    mObserveList.clear();
}

void MySubject::attach(MyObseveImpl& observeCom)
{
    mObserveList.push_back(observeCom);
} 

void MySubject::notifyInfo( int status)
{
    for(auto& it : mObserveList) 
    {
        //std::cout << "notifyInfo" << std::endl;
        it.updateInfo( status );
    } 

}

void MySubject::changeStatus(const MYState& refStatus)
{
    notifyInfo( 1 );
    //notifyInfo( static_cast<int>(refStatus) );
}
   

} //namespace litest