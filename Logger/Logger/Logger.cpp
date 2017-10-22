// Logger.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "Logger.h"
#include <boost/date_time.hpp>
#include <iostream>
#include <mutex>
#include <sstream>


namespace {
   static std::ofstream logFile;
   const std::string default_log_file_location("D:/CCHR/Projects/Circus2017/Logs/CircusLogFile.txt");
   std::string logFileLocation;
   static std::mutex m_mutex;

   static std::string Timestamped(const std::string& input) {
      boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
      std::stringstream ss;
      ss << now << " " << input;
      return ss.str();
   }

   static void LogThis(const std::string& msg) {
      const std::string timed(Timestamped(msg));
      logFile   << timed << std::endl;
      std::cout << timed << std::endl;
   }
}

namespace LoggerNS {
   void Logger::LogFileLocation(const std::string str) { logFileLocation = str; }

   void Logger::Log(const std::string& msg) {
      std::lock_guard<std::mutex> lock(m_mutex);
      if (logFileLocation.length() == 0) logFileLocation = default_log_file_location;
      if (!logFile.is_open()) logFile = std::ofstream(logFileLocation);
      LogThis(msg);
   }
}

