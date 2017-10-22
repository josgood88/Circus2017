//
/// \page databaseInterface databaseInterface
/// \remark databaseInterface localizes all the information necessary to working with the Circus database.
//
// Any project including DB.cpp must define OTL_STL in the project property sheet, to enable std::string <==> varchar/char
//
#pragma warning (disable:4996)      // 'std::_Copy_backward': Function call with parameters that may be unsafe
#include "DB_capublic.h"

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
// Start of class definition.  Constructors and destructors first.
//*****************************************************************************
//
DB_capublic::DB_capublic() {
   Report("DB_capublic creation, default");
   if (default_connect_string.length() > 0) Initialize(default_connect_string, "capublic");
}

DB_capublic::DB_capublic(const std::string& databaseName) {
   Report("DB_capublic creation, db name");
   Initialize(default_connect_string, databaseName);
}

DB_capublic::DB_capublic(const std::string& connectString, const std::string& databaseName) {
   Report("DB_capublic creation, connect string, db name");
   Initialize(connectString, databaseName);
}

DB_capublic::~DB_capublic() {
   db.logoff();
}
//
//*****************************************************************************
/// Initialize database connection.  Used during construction.  Issues USE command.
//*****************************************************************************
//
void DB_capublic::Initialize(const std::string& connectString, const std::string& databaseName) {
   try {
      db.rlogon(connectString.c_str());                     // Connect to ODBC
      std::string useStr = std::string("Use ") + databaseName;
      Report(std::string("\t") + useStr);
      db.direct_exec(useStr.c_str());                       // Use this database
   } catch (const otl_exception& ) {
      ReportException("DB_capublic::Initialize");
      throw;
   }
}
//
//*****************************************************************************
/// \brief Perform a non-query action on a table.
/// \param[in] query The query that returns the row
/// \returns -1 on error, >=0, if the SQL command is succesful. 
///          In case of INSERT, UPDATE or DELETE statements, returns count of rows affected.
//*****************************************************************************
//
bool DB_capublic::ExecuteSQL(const std::string& command) {
long x;
   try {
      Report(command);
      x = db.direct_exec(command.c_str(),otl_exception::enabled);
   } catch (const otl_exception& ) {
      ReportException("EnsureLegislativeDatabase::ExecuteSQL");
      return false;
   }
   return true;
}
//
//*****************************************************************************
/// \brief Perform a non-query action on a table.
/// \param[in] query The query that returns the row
/// \returns -1 on error, >=0, if the SQL command is succesful. 
///          In case of INSERT, UPDATE or DELETE statements, returns count of rows affected.
//*****************************************************************************
//
long DB_capublic::NonQuery(const std::string& nonQuery) {
   return db.direct_exec(nonQuery.c_str(), otl_exception::disabled);
}
//
//*****************************************************************************
/// \brief Count number of table rows
/// \param[in] tableName Count rows in this table
//*****************************************************************************
//
long DB_capublic::CountRows(const std::string& tableName, const std::string& whereClause) {
   std::stringstream ss;
   ss << "Select Count(*) from " << tableName << whereClause;
   long result(-1);
   try {
      otl_stream is(1, ss.str().c_str(), db);      // Count has a single result, 1 row required
      is >> result;
   } catch (const otl_exception& ex) {
      std::stringstream ss;
      ss << "OTL exception in DB_capublic::CountRows" << ex.msg;
      Report(ss.str());
   }
   return result;
}

//
//*****************************************************************************
/// \brief Report exceptions to std::cout
//*****************************************************************************
//
void DB_capublic::Report(const std::string& message) {
   std::cout << message << std::endl;
}
//
//*****************************************************************************
/// \brief Report exceptions
//*****************************************************************************
//
void DB_capublic::ReportException(const std::string& reporter) {
   try {
      throw;
   } catch (const otl_exception& ex) {
      Report(std::string("Exception caught in ") + reporter);
      Report(reinterpret_cast<const char*>(ex.msg));        // Report error message
      Report(ex.stm_text);                                  // Report SQL that caused the error
      Report(ex.var_info);                                  // Report the variable that caused the error
   } catch (const std::exception& ex) {
      Report(ex.what());                                    // Report error message
   }
}
