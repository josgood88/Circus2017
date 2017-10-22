#include <boost/interprocess/sync/named_mutex.hpp>
//
//    Manage the lifetime of an interprocess mutex
//
class ScopedInterprocessNamedMutex {
public:
   ScopedInterprocessNamedMutex(std::string aName) : name_(aName), mutex_qm(boost::interprocess::create_only,aName.c_str()) { }
   ~ScopedInterprocessNamedMutex() { boost::interprocess::named_mutex::remove(name_.c_str()); }
private:
   boost::interprocess::named_mutex mutex_qm;
   std::string name_;
};

#define mutex_qmLOGGER_INPUT_QUEUE "Logger.Input.Queue"
