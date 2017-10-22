//
//*****************************************************************************
/// \brief This namespace collects methods that are useful for all capublic table processing.
//*****************************************************************************
//
#include "CAPublicTablesNS.h"
#include "DB_capublic.h"
#include "sqlite3.h"

#include <boost/weak_ptr.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

namespace CAPublicTablesNS {

   std::string ReadFile(const std::string& path) {
      std::ifstream is(path);
      std::stringstream ss;
      ss << is.rdbuf();
      return ss.str();
   }

   std::string TrimFirstLastSingleQuote(const std::string& source) { 
      if (*source.begin() == 0x60 && *(source.end()-1) == 0x60) return source.substr(1,source.length()-2);
      return source;
   }

   // Convert single quote to two single quotes
   std::string SQLQuote(const std::string& source) {  
      std::string result;
      std::for_each(source.begin(),source.end(), [&](const char& c) {
         result += c;
         if (c == '\'') result += c;
      });
      return result;
   }

   bool ClearTable(boost::weak_ptr<DB_capublic> database, const std::string& table_name) {
      boost::shared_ptr<DB_capublic> wp = database.lock();
      if (wp) {
         std::stringstream ss;
         ss << "Delete from " << table_name << ";";
         if (!wp->ExecuteSQL(ss.str())) {
            wp->Report(std::string("Failed SQL \n") + ss.str());
            return false;
         }
      }
      return true;
   }

   //
   //*****************************************************************************
   /// \brief Report exceptions
   //*****************************************************************************
   //
   void Report(const std::string& message) {
      std::cout << message << std::endl;
   }
   void ReportException(const std::string& announce, const std::exception& ex) {
      Report(announce);
      Report(reinterpret_cast<const char*>(ex.what()));        // Report error message
   }

}