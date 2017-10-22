//
/// \page databaseInterface databaseInterface
/// \remark databaseInterface localizes all the information necessary to working with the Circus database.
//
// Any project including DB.cpp must define OTL_STL in the project property sheet, to enable std::string <==> varchar/char
//
#pragma warning (disable:4996)      // 'std::_Copy_backward': Function call with parameters that may be unsafe
#include "DB.h"

// OTL_STL is defined in the project property sheet, to enable std::string <==> varchar/char
#include <OTL/otlv4.h>     // http://otl.sourceforge.net/otl3.htm
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include <string>
#include <sstream>

namespace {
   const std::string default_connect_string("UID=Joe;PWD=sher8lock;DSN=ODBC_SQL_Server");
}

//
//*****************************************************************************
/// \brief Report exceptions to std::cout
//*****************************************************************************
//
void Report(const std::string& message) {
   std::cout << message << std::endl;
}
//
//*****************************************************************************
/// Initialize database connection.  Used during construction.
//*****************************************************************************
//
void Initialize(otl_connect& db, const std::string& connectString, const std::string& databaseName) {
   try {
      db.rlogon(connectString.c_str());                     // Connect to ODBC
      std::string useStr = std::string("Use ") + databaseName;
      db.direct_exec(useStr.c_str());                       // Use this database
   } catch (const otl_exception& ex) {
      Report(reinterpret_cast<const char*>(ex.msg));        // Report error message
      Report(ex.stm_text);                                  // Report SQL that caused the error
      Report(ex.var_info);                                  // Report the variable that caused the error
      throw;
   }
}
//
//*****************************************************************************
// Start of class definition.  Constructors and destructors first.
//*****************************************************************************
//
DB::DB() {
   Initialize(db, default_connect_string, "Circus");
}

DB::DB(const std::string& databaseName) {
   Initialize(db, default_connect_string, databaseName);
}

DB::DB(const std::string& connectString, const std::string& databaseName) {
   Initialize(db, connectString, databaseName);
}

DB::~DB() {
   db.logoff();
}
//
//*****************************************************************************
/// \brief Perform a non-query action on a table.
/// \param[in] query The query that returns the row
/// \returns -1 on error, >=0, if the SQL command is succesful. 
///          In case of INSERT, UPDATE or DELETE statements, returns count of rows affected.
//*****************************************************************************
//
long DB::NonQuery(const std::string& nonQuery) {
   return db.direct_exec(nonQuery.c_str(), otl_exception::disabled);
}
//
//*****************************************************************************
/// \brief Count number of table rows
/// \param[in] tableName Count rows in this table
//*****************************************************************************
//
long DB::CountRows(const std::string& tableName, const std::string& whereClause) {
   std::stringstream ss;
   ss << "Select Count(*) from " << tableName << whereClause;
   long result(-1);
   try {
      otl_stream is(1, ss.str().c_str(), db);                  // Count has a single result, 1 row required
      is >> result;
   } catch (const otl_exception& ex) {
      std::stringstream ss;
      ss << "OTL exception in DB::CountRows" << ex.msg;
      Report(ss.str());
   }
   return result;
}
