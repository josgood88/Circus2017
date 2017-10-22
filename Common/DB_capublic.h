#ifndef DB_capublic_h
#define DB_capublic_h

#include "sqlite3.h"
#include <regex>
#include <sstream>
#include <string>

//
//*****************************************************************************
/// \brief DB provides database encapsulation.  
///        This version is used for the database which receives data from the California Legislature website.
//*****************************************************************************
//

class DB_capublic {
public:
   DB_capublic();
   DB_capublic(const std::string& databaseName);
   ~DB_capublic();

   void Report(const std::string& message);
   void ReportException(const std::string& reporter);
   sqlite3* db;                                 // Database connection

   // Actions on any table
   bool ExecuteSQL(const std::string& command);
   unsigned long Count(const std::string& tableName, const std::string& whereClause);

   // Static
   static std::string EnQuote(std::string s) {
      const std::regex re("'");
      const std::string replacement("''");
      return std::regex_replace(s,re,replacement);
   }

   static unsigned int ToUInt(const std::string& s) { std::stringstream ss; ss << s; unsigned int i(0); ss >> i;  return i; }
   static std::string  FrUInt(unsigned int       i) { std::stringstream ss; ss << i; std::string s;     ss >> s;  return s; }

   static std::string Quote (const std::string& s) { return std::string("'") + EnQuote(s) + "'"; }
   static std::string QuoteC(const std::string& s) { return Quote(s) + ", "; }

   static std::string IComma(unsigned int i) { return FrUInt(i) + ","; }

private:
   bool Initialize(const std::string& databaseName);
};

#endif
