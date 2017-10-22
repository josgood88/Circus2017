#ifndef queuemap_h
#define queuemap_h

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/sync/interprocess_recursive_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/scoped_ptr.hpp>
#include <map>
#include <string>
#include <vector>

class QueueMap {
public:
   QueueMap() { }
   ~QueueMap() { }

   void Add(std::string name_, boost::shared_ptr<boost::interprocess::message_queue> queue_) { 
      boost::interprocess::scoped_lock<boost::interprocess::interprocess_recursive_mutex> lock(mutex);
      internalMap[name_] = queue_;
   }
   bool Exists(const std::string& name_) { 
      boost::interprocess::scoped_lock<boost::interprocess::interprocess_recursive_mutex> lock(mutex);
      return internalMap.find(name_) != internalMap.end(); 
   }
   boost::shared_ptr<boost::interprocess::message_queue> operator[] (const std::string& name_) { 
      boost::interprocess::scoped_lock<boost::interprocess::interprocess_recursive_mutex> lock(mutex);
      return internalMap[name_];
   }
   std::vector<std::string> Contents() { 
      boost::interprocess::scoped_lock<boost::interprocess::interprocess_recursive_mutex> lock(mutex);
      std::vector<std::string> result;
      for (auto itr = internalMap.begin(); itr != internalMap.end(); itr++) {
         result.push_back(itr->first);
      }
      return result; 
   }


private:
   QueueMap(QueueMap const&);
   void operator= (QueueMap const&);

   std::map<std::string, boost::shared_ptr<boost::interprocess::message_queue>> internalMap;
// TODO: Consider using a separate mutex for each message queue.  Program architecture doesn't promote deadlocks.
   boost::interprocess::interprocess_recursive_mutex mutex;
};

#endif
