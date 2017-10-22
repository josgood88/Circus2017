//
/// \page databaseInterface databaseInterface
/// \remark databaseInterface localizes all the information necessary to working with the capublic database.

#pragma warning (disable:4996)      // 'std::_Copy_backward': Function call with parameters that may be unsafe
#include "DB_capublic.h"
#include "Logger.h"
#include "sqlite3.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/thread/thread.hpp>
#include <string>
#include <sstream>

namespace {
   boost::mutex execute_sql_mutex;

   void Report(const std::string& message) {
      LoggerNS::Logger::Log(message);
   }
   //
   //*****************************************************************************
   // Error reporting callback for sqlite3_exec
   //*****************************************************************************
   //
   int ErrorMessageCallback(void *NotUsed, int argc, char **argv, char **azColName) {
      std::stringstream ss;
      for (int i=0; i<argc; i++) {
         ss << azColName[i] << " = " << (argv[i] ? argv[i] : "NULL") << std::endl;
      }
      Report(ss.str());
      return 0;
   }
}

//
//*****************************************************************************
// Start of class definition.  Constructors and destructors first.
//*****************************************************************************
//
DB_capublic::DB_capublic() {
   LoggerNS::Logger::Log("Ensuring database exists (default name - capublic)");
   Initialize("capublic");
}

DB_capublic::DB_capublic(const std::string& databaseName) {
   std::stringstream ss;
   ss << "Ensuring database " << databaseName << " exists";
   LoggerNS::Logger::Log(ss.str());
   Initialize(databaseName);
}

DB_capublic::~DB_capublic() {
   sqlite3_close(db);
}
//
//*****************************************************************************
/// Initialize database connection.
//*****************************************************************************
//
bool DB_capublic::Initialize(const std::string& databaseName) {
   if (sqlite3_open(databaseName.c_str(), &db) != 0) {
      LoggerNS::Logger::Log(std::string("DB_capublic::Initialize was not able to open ") + databaseName);
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
bool DB_capublic::ExecuteSQL(const std::string& command) {
   boost::unique_lock<boost::mutex> lock(execute_sql_mutex);
   const char* cp(command.c_str());
   char* error_message(NULL);
   if (sqlite3_exec(db, cp, ErrorMessageCallback, "", &error_message) != SQLITE_OK) {
      LoggerNS::Logger::Log(std::string("DB_capublic::ExecuteSQL was unable to execute \n") + command + "\n");
      LoggerNS::Logger::Log(error_message ? error_message : "SQLite did not generate an error message");
      return false;
   }
   return true;
}
//
//*****************************************************************************
/// \brief Count number of table rows
/// \param[in] tableName Count rows in this table
//*****************************************************************************
//
namespace {
   static int count_callback(void *count, int argc, char **argv, char **azColName) {
      std::stringstream ss;
      ss << argv[0];
      int cc;
      ss >> cc;
      int* ptr = reinterpret_cast<int*>(count);
      *ptr = cc;
      return 0;
   }
}

unsigned long DB_capublic::Count(const std::string& tableName, const std::string& whereClause) {
   unsigned long result(0);
   std::stringstream ss;
   ss << "Select Count(*) from " << tableName << whereClause;
   char *zErrMsg = 0;
   if (sqlite3_exec(db, ss.str().c_str(), count_callback, &result, &zErrMsg) != SQLITE_OK) {
      LoggerNS::Logger::Log(std::string("DB_capublic::Count error: ") + (zErrMsg ? zErrMsg : "No Indication"));
      sqlite3_free(zErrMsg);
   }   
   return result;
}

//
//*****************************************************************************
/// \brief Report exceptions to std::cout
//*****************************************************************************
//
void DB_capublic::Report(const std::string& message) {
   ::Report(message);
}
//
//*****************************************************************************
/// \brief Report exceptions
//*****************************************************************************
//
void DB_capublic::ReportException(const std::string& reporter) {
   //try {
   //   throw;
   //} catch (const otl_exception& ex) {
   //   Report(std::string("Exception caught in ") + reporter);
   //   Report(reinterpret_cast<const char*>(ex.msg));        // Report error message
   //   Report(ex.stm_text);                                  // Report SQL that caused the error
   //   Report(ex.var_info);                                  // Report the variable that caused the error
   //} catch (const std::exception& ex) {
   //   Report(ex.what());                                    // Report error message
   //}
}
