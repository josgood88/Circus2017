#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include "Logger.h"
#include <sstream>

class ScopedElapsedTime {
public:
   ScopedElapsedTime(const std::string first, const std::string second) : m_prefix(second), now1(boost::posix_time::microsec_clock::universal_time()) { 
      LoggerNS::Logger::Log(first);
   }
   ~ScopedElapsedTime() {
      const boost::posix_time::ptime now2 = boost::posix_time::microsec_clock::universal_time();
      const boost::posix_time::time_duration duration = now2 - now1;
      std::stringstream ss;
      ss << m_prefix << duration;
      LoggerNS::Logger::Log(ss.str());
   }
private:
   std::string m_prefix;
   boost::posix_time::ptime now1;
};