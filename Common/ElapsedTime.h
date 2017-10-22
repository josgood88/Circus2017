#ifndef ElapsedTime_h
#define ElapsedTime_h

#include "boost/date_time/posix_time/posix_time.hpp"

namespace {

   boost::posix_time::ptime StartingTime() { return boost::posix_time::microsec_clock::universal_time(); }

   boost::posix_time::time_duration ElapsedTime(const boost::posix_time::ptime& start_time) {
      const boost::posix_time::ptime end_time = boost::posix_time::microsec_clock::universal_time();
      return end_time - start_time;
   }

}

#endif
