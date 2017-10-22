#pragma warning (disable:4996)      // 'std::_Copy_backward': Function call with parameters that may be unsafe

#include "db.h"

#include <exception>
#include <sstream>
#include <vector>

// OTL_STL is defined in the project property sheet, to enable std::string <==> varchar/char
#include <OTL/otlv4.h>     // http://otl.sourceforge.net/otl3.htm

namespace {
   otl_connect db;
   const std::string leg_db_name("capublic");
   const std::string bill_analysis_tbl(
      "IF NOT EXISTS (SELECT * FROM sysobjects WHERE name='bill_analysis_tbl' AND xtype='U')" \
      "CREATE TABLE bill_analysis_tbl (" \
         "analysis_id      DECIMAL(22, 0)  NOT NULL PRIMARY KEY," \
         "bill_id          VARBINARY(20)   NOT NULL," \
         "house            VARBINARY(1)    NULL,"     \
         "analysis_type    VARBINARY(100)  NULL,"     \
         "committee_code   VARBINARY(6)    NULL,"     \
         "committee_name   VARBINARY(200)  NULL,"     \
         "amendment_author VARBINARY(100)  NULL,"     \
         "analysis_date    DATETIME        NULL,"     \
         "amendment_date   DATETIME        NULL,"     \
         "page_num         DECIMAL(22, 0)  NULL,"     \
         "source_doc       VARBINARY(MAX)  NULL,"     \
         "released_floor   VARBINARY(1)    NULL,"     \
         "active_flg       VARBINARY(1)    NULL,"     \
         "trans_uid        VARBINARY(20)   NULL,"     \
         "trans_update     DATETIME        NULL"      \
       ")");
   const std::string bill_tbl(
       "IF NOT EXISTS (SELECT * FROM sysobjects WHERE name='bill_tbl' AND xtype='U')" \
       "CREATE TABLE bill_tbl (" \
         "bill_id                VARBINARY(19)  NOT NULL PRIMARY KEY," \
         "session_year           VARBINARY(8)   NOT NULL," \
         "session_num            VARBINARY(2)   NOT NULL," \
         "measure_type           VARBINARY(4)   NOT NULL," \
         "measure_num            INT            NOT NULL," \
         "measure_state          VARBINARY(40)  NOT NULL," \
         "chapter_year           VARBINARY(4)   NULL," \
         "chapter_type           VARBINARY(10)  NULL," \
         "chapter_session_num    VARBINARY(2)   NULL," \
         "chapter_num            VARBINARY(10)  NULL," \
         "latest_bill_version_id VARBINARY(30)  NULL," \
         "active_flg             VARBINARY(1)   NULL," \
         "trans_uid              VARBINARY(30)  NULL," \
         "trans_update           DATETIME       NULL," \
         "current_location       VARBINARY(200) NULL," \
         "current_secondary_loc  VARBINARY(60)  NULL," \
         "current_house          VARBINARY(60)  NULL," \
         "current_status         VARBINARY(60)  NULL," \
         "days_31st_in_print     DATETIME       NULL " \
         ")"
   );

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
   /// \brief Report exceptions
   //*****************************************************************************
   //
   void ReportException(const std::string& reporter) {
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
   //
   //*****************************************************************************
   /// Connect to the database server.
   //*****************************************************************************
   //
   bool Connect(const std::string& connectString) {
      try {
         db.rlogon(connectString.c_str());                     // Connect to ODBC
      } catch (const otl_exception& ) {
         ReportException("EnsureLegislativeDatabase::Connect");
         return false;
      }
      return true;
   }

   bool Execute(const std::string& command) {
      try {
         db.direct_exec(command.c_str());
      } catch (const otl_exception& ) {
         ReportException("EnsureLegislativeDatabase::Use");
         return false;
      }
      return true;
   }

   bool Use(const std::string& databaseName) {
      std::string useStr = std::string("Use ") + databaseName;
      return Execute(useStr);                               // Use this database
   }
}
//
//*****************************************************************************
/// EnsureLegislativeDatabase is responsible for
/// -# Creating the database, if necessary
/// -# Verifying that the necessary tables are present
//*****************************************************************************
//
void main(int argc, char* argv[]) {
   Report("Ensure legislative database existence");
   try {
      DB db("capublic");
      db.NonQuery(bill_analysis_tbl);           // Ensure bill analysis table
   } catch (...) {
      Report("EnsureLegislativeDatabase caught an ellipsis exception");
   }
   Report("EnsureLegislativeDatabase is exiting");
}
