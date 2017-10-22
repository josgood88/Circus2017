#pragma once  

#include <string>

#ifdef LOGGER_EXPORTS  
   #define Logger_API __declspec(dllexport)
#else  
   #define Logger_API __declspec(dllimport)
#endif  

namespace LoggerNS {
   class Logger {
   public:
      static Logger_API void LogFileLocation(const std::string str);
      static Logger_API void Log(const std::string& msg);
   };
}