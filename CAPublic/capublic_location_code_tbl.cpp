#include "capublic_location_code_tbl.h"
#include "CAPublicTablesNS.h"
#include "db_capublic.h"
#include <ElapsedTime.h>
#include "Logger.h"
#include <Readers.h>
#include "ScopedElapsedTime.h"
#include <Utility.h>

#include <boost/shared_ptr.hpp>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
   const std::string sql_create_capublic_location_code_tbl(
      "CREATE TABLE IF NOT EXISTS location_code_tbl (" 
         "session_year          TEXT NULL, "
         "location_code         TEXT NULL, "
         "location_type         TEXT NULL, "
         "consent_calendar_code TEXT NULL, "
         "description           TEXT NULL, "
         "long_description      TEXT NULL, "
         "active_flg            TEXT NULL, "
         "trans_uid             TEXT NULL, "
         "trans_update          TEXT NULL, "
         "inactive_file_flg     TEXT NULL "
      ");"
   );

   // Insert a token into a LocationTableRow, based on the column offset into the row
   void SetColumn(LocationTableRow& row, const std::string& _token, unsigned int offset) {
      const std::string token(CAPublicTablesNS::SQLQuote(_token));
      switch (offset) {
         case  0: row.session_year          = token; break;
         case  1: row.location_code         = token; break;
         case  2: row.location_type         = token; break;
         case  3: row.consent_calendar_code = token; break;
         case  4: row.description           = token; break;
         case  5: row.long_description      = token; break;
         case  6: row.active_flg            = token; break;
         case  7: row.trans_uid             = token; break;
         case  8: row.trans_update          = token; break;
         case  9: row.inactive_file_flg     = token; break;
         default: 
            std::stringstream ss;
            ss << "capublic_location_code_tbl InsertColumn: offset " << offset << " is out of range." << std::endl;
            throw new std::out_of_range(ss.str());
            break;
      }
   }

   // Read a line from bill_history_tbl, parse out the fields into a row, and insert the row. 
   LocationTableRow ParseRowToStruct(const std::string& line) {
      LocationTableRow result;
      auto table_offset(0u);                 // Offset into the table

      const std::regex splitOn("\\t");
      std::sregex_token_iterator itr(line.begin(),line.end(),splitOn,-1);
      const std::sregex_token_iterator endOfSequence;
      while (itr != endOfSequence) {
         const auto token((*itr++).str());               // Need string, not sub_match
         SetColumn(result,CAPublicTablesNS::TrimFirstLastSingleQuote(token),table_offset++);
      }
      return result;
   }
}


bool capublic_location_code_tbl::Insert(const LocationTableRow& row) {
   bool result(false);
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      std::stringstream ss;
      ss << "Insert into location_code_tbl ("
         << "session_year, location_code,    location_type, consent_calendar_code, "
         << "description,  long_description, active_flg,    trans_uid, "
         << "trans_update, inactive_file_flg) "
         << "Values ("
         << QC(row.session_year) << QC(row.location_code)    << QC(row.location_type) << QC(row.consent_calendar_code)
         << QC(row.description)  << QC(row.long_description) << QC(row.active_flg)    << QC(row.trans_uid) 
         << QC(row.trans_update) << Q (row.inactive_file_flg) 
         << ");";
      result = ConcatentateMultipleInsertsBeforeExecuting(ss.str(),db_public,false);
   }
   return result;
}
void capublic_location_code_tbl::ParseAndWriteRow(const std::string& line,boost::weak_ptr<DB_capublic>& db_public) {
   LocationTableRow row(ParseRowToStruct(line));
   Insert(row);
}

// Constructor ensures that the database table exists
capublic_location_code_tbl::capublic_location_code_tbl(boost::weak_ptr<DB_capublic> database,bool import_leg_data): db_public(database) {
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      if (import_leg_data) {
         if (wp->ExecuteSQL(sql_create_capublic_location_code_tbl)) {
            LoggerNS::Logger::Log("Creating location_code_tbl");
            InsertLocationTable("D:/CCHR/2017-2018/LatestDownload/location_code_tbl.dat");
         } else {
            LoggerNS::Logger::Log(std::string("Failed SQL \n") + sql_create_capublic_location_code_tbl);
         }
      }
   }
}

void capublic_location_code_tbl::InsertLocationTable(const std::string& path) {
   ScopedElapsedTime elapsed_time("\tReading Location_Code_TBL.dat","\tTable creation time: ");
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      auto file_contents(ReadFileLineByLine(path));
      const auto initial_row_count(wp->Count("location_code_tbl",""));
      const auto new_row_count(file_contents.size());
      std::stringstream ss1;
      ss1 << "\tLocation code table currently has " << initial_row_count << " database entries.  Location_Code_Tbl.dat has " << new_row_count << " entries. There are " << new_row_count-initial_row_count << " new entries.";
      LoggerNS::Logger::Log(ss1.str());

      // Clear table contents before inserting, since bill_tbl from the leg site lists only the latest versions of bills.
      if (CAPublicTablesNS::ClearTable(wp,"location_code_tbl")) {
         // Parse each row into a struct, write the structs to the database table
         std::for_each(file_contents.begin(),file_contents.end(),[&](const std::string& line) {
            ParseAndWriteRow(line,db_public);
         });
      } else {
         LoggerNS::Logger::Log("Unable to clear location_code_tbl");
      }
   }
   // Flush the sql accumulator.
   ConcatentateMultipleInsertsBeforeExecuting(std::string(),db_public,true);
   std::stringstream ss2;
   ss2 << "\tAfter creation and filling, location_code_tbl has " << wp->Count("location_code_tbl","") << " rows.";
   LoggerNS::Logger::Log(ss2.str());
}

std::string capublic_location_code_tbl::FieldQuery(const std::string& query) {
   return Readers::ReadSingleString(db_public,query);
}
