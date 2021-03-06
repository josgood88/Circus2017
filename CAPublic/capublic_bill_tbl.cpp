#include <BillRow.h>
#include "capublic_bill_tbl.h"
#include "CAPublicTablesNS.h"
#include "db_capublic.h"
#include <Logger.h>
#include <Readers.h>
#include <ScopedElapsedTime.h>
#include <Utility.h>

#include <boost/shared_ptr.hpp>
#include <sstream>
#include <string>

namespace {
   const std::string sql_create_bill_tbl(
      "CREATE TABLE IF NOT EXISTS bill_tbl ("
      "bill_id                TEXT NOT NULL,"
      "session_year           TEXT NOT NULL,"
      "session_num            TEXT NOT NULL,"
      "measure_type           TEXT NOT NULL,"
      "measure_num            TEXT NOT NULL,"
      "measure_state          TEXT NOT NULL,"
      "chapter_year           TEXT NULL,"
      "chapter_type           TEXT NULL,"
      "chapter_session_num    TEXT NULL,"
      "chapter_num            TEXT NULL,"
      "latest_bill_version_id TEXT NULL,"
      "active_flg             TEXT NULL,"
      "trans_uid              TEXT NULL,"
      "trans_update           TEXT NULL,"
      "current_location       TEXT NULL,"
      "current_secondary_loc  TEXT NULL,"
      "current_house          TEXT NULL,"
      "current_status         TEXT NULL,"
      "days_31st_in_print     TEXT NULL "
      ");"
   );

   // Insert a token into a BillTableRow, based on the column offset into the row
   void SetColumn(BillTableRow& row,const std::string& token,unsigned int offset) {
      switch (offset) {
      case  0: row.bill_id                = token; break;
      case  1: row.session_year           = token; break;
      case  2: row.session_num            = token; break;
      case  3: row.measure_type           = token; break;
      case  4: row.measure_num            = token; break;
      case  5: row.measure_state          = token; break;
      case  6: row.chapter_year           = token; break;
      case  7: row.chapter_type           = token; break;
      case  8: row.chapter_session_num    = token; break;
      case  9: row.chapter_num            = token; break;
      case 10: row.latest_bill_version_id = token; break;
      case 11: row.active_flg             = token; break;
      case 12: row.trans_uid              = token; break;
      case 13: row.trans_update           = token; break;
      case 14: row.current_location       = token; break;
      case 15: row.current_secondary_loc  = token; break;
      case 16: row.current_house          = token; break;
      case 17: row.current_status         = token; break;
      case 18: row.days_31st_in_print     = token; break;
      default:
         std::stringstream ss;
         ss << "capublic_bill_tbl InsertColumn: offset " << offset << " is out of range." << std::endl;
         throw new std::out_of_range(ss.str());
         break;
      }
   }

   // Read a line from Bill_Tbl.dat, parse out the fields into a row, and insert the row. 
   BillTableRow ParseRowToStruct(const std::string& line) {
      BillTableRow result;
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

// Constructor ensures that the database table exists
capublic_bill_tbl::capublic_bill_tbl(boost::weak_ptr<DB_capublic> database,bool import_leg_data): db_public(database) {
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      if (import_leg_data) {
         if (wp->ExecuteSQL(sql_create_bill_tbl)) {
            LoggerNS::Logger::Log("Creating capublic_bill_tbl");
            InsertBillTable("D:/CCHR/2017-2018/LatestDownload/BILL_TBL.dat");
         } else {
            LoggerNS::Logger::Log(std::string("Failed SQL \n") + sql_create_bill_tbl);
         }
      }
   }
}

bool capublic_bill_tbl::Insert(const BillTableRow& row,boost::weak_ptr<DB_capublic>& db_public) {
   bool result(false);
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      std::stringstream ss;
      ss << "Insert into bill_tbl ("
         "bill_id, session_year, session_num, measure_type, measure_num, measure_state, chapter_year, "
         "chapter_type, chapter_session_num, chapter_num, latest_bill_version_id, active_flg,"
         "trans_uid, trans_update, current_location, current_secondary_loc, current_house, current_status, days_31st_in_print) "
         "Values ("
         << QC(row.bill_id)                << QC(row.session_year)  << QC(row.session_num)    << QC(row.measure_type)         << QC(row.measure_num)
         << QC(row.measure_state)          << QC(row.chapter_year)  << QC(row.chapter_type)   << QC(row.chapter_session_num)  << QC(row.chapter_num)
         << QC(row.latest_bill_version_id) << QC(row.active_flg)    << QC(row.trans_uid)      << QC(row.trans_update)         << QC(row.current_location)
         << QC(row.current_secondary_loc)  << QC(row.current_house) << QC(row.current_status) << Q (row.days_31st_in_print)
         << ");";
      result = ConcatentateMultipleInsertsBeforeExecuting(ss.str(),db_public,false);
   }
   return result;
}

void capublic_bill_tbl::ParseAndWriteRow(const std::string& line,boost::weak_ptr<DB_capublic>& db_public) {
   BillTableRow row(ParseRowToStruct(line));
   Insert(row,db_public);
}

void capublic_bill_tbl::InsertBillTable(const std::string& path) {
   ScopedElapsedTime elapsed_time("\tReading Bill_Tbl.dat","\tTable creation time: ");
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      auto file_contents(ReadFileLineByLine(path));
      const auto initial_row_count(wp->Count("bill_tbl",""));
      const auto new_row_count(file_contents.size());
      std::stringstream ss1;
      ss1 << "\tBill table currently has " << initial_row_count << " database entries.  Bill_Tbl.dat has " << new_row_count << " entries. There are " << new_row_count-initial_row_count << " new entries.";
      LoggerNS::Logger::Log(ss1.str());

      // Clear table contents before inserting, since bill_tbl from the leg site lists only the latest versions of bills.
      if (CAPublicTablesNS::ClearTable(wp,"bill_tbl")) {
         // Parse each row into a struct, write the structs to the database table
         std::for_each(file_contents.begin(),file_contents.end(),[&](const std::string& line) {
            ParseAndWriteRow(line,db_public);
         });
         // Flush the concatenation buffer
         ConcatentateMultipleInsertsBeforeExecuting(std::string(),db_public,true);
      } else {
         LoggerNS::Logger::Log("Unable to clear bill_tbl");
      }
   }
   // Flush the sql accumulator.
   ConcatentateMultipleInsertsBeforeExecuting(std::string(),db_public,true);
   std::stringstream ss2;
   ss2 << "\tAfter creation and filling, bill_tbl has " << wp->Count("bill_tbl","") << " rows.";
   LoggerNS::Logger::Log(ss2.str());
}

std::vector<BillRow> capublic_bill_tbl::Read() { return Readers::ReadVectorLegBillTable(db_public); }

namespace {
   std::string ReadSingleString(boost::weak_ptr<DB_capublic>& db_public,const std::string& query,const std::string& arg) {
      std::stringstream ss;
      ss << query << arg << "';";
      return Readers::ReadSingleString(db_public,ss.str());
   }
}

std::string capublic_bill_tbl::MeasureType(const std::string& id) {
   const std::string query("Select measure_type from bill_tbl where latest_bill_version_id = '");
   return ReadSingleString(db_public,query,id);
}

std::string capublic_bill_tbl::MeasureNum(const std::string& id) {
   const std::string query("Select measure_num  from bill_tbl where latest_bill_version_id = '");
   return ReadSingleString(db_public,query,id);
}

std::string capublic_bill_tbl::FieldQuery(const std::string& query) {
   return Readers::ReadSingleString(db_public,query);
}
