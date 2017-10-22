#include <BillHistoryTableRow.h>
#include "capublic_bill_history_tbl.h"
#include "CAPublicTablesNS.h"
#include "db_capublic.h"
#include <Logger.h>
#include <ScopedElapsedTime.h>
#include <Utility.h>

#include <boost/shared_ptr.hpp>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
   const std::string sql_create_bill_history_tbl(
      "CREATE TABLE IF NOT EXISTS bill_history_tbl (" 
         "bill_id            TEXT NOT NULL, "
         "bill_history_id    TEXT NOT NULL, "
         "action_date        TEXT NULL, "
         "action             TEXT NULL, "
         "trans_uid          TEXT NULL, "
         "trans_update_dt    TEXT NULL, "
         "action_sequence    TEXT NULL, "
         "action_code        TEXT NULL, "
         "action_status      TEXT NULL, "
         "primary_location   TEXT NULL, "
         "secondary_location TEXT NULL, "
         "ternary_location   TEXT NULL, "
         "end_status         TEXT NULL "
      ");"
   );

   // Insert a token into a BillHistoryTableRow, based on the column offset into the row
   void SetColumn(BillHistoryTableRow& row, const std::string& _token, unsigned int offset) {
      const std::string token(CAPublicTablesNS::SQLQuote(_token));
      switch (offset) {
         case  0: row.bill_id            = token; break;
         case  1: row.bill_history_id    = token; break;
         case  2: row.action_date        = token; break;
         case  3: row.action             = token; break;
         case  4: row.trans_uid          = token; break;
         case  5: row.trans_update_dt    = token; break;
         case  6: row.action_sequence    = token; break;
         case  7: row.action_code        = token; break;
         case  8: row.action_status      = token; break;
         case  9: row.primary_location   = token; break;
         case 10: row.secondary_location = token; break;
         case 11: row.ternary_location   = token; break;
         case 12: row.end_status         = token; break;
         default: 
            std::stringstream ss;
            ss << "capublic_bill_history_tbl InsertColumn: offset " << offset << " is out of range." << std::endl;
            throw new std::out_of_range(ss.str());
            break;
      }
   }

   // Read a line from Bill_Tbl.dat, parse out the fields into a row, and insert the row. 
   BillHistoryTableRow ParseRowToStruct(const std::string& line) {
      BillHistoryTableRow result;
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

bool capublic_bill_history_tbl::Insert(const BillHistoryTableRow& row,boost::weak_ptr<DB_capublic>& db_public) {
   bool result(false);
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      std::stringstream ss;
      ss << "Insert into bill_history_tbl ("
         "bill_id, bill_history_id, action_date, action, trans_uid, trans_update_dt, action_sequence, "
         "action_code, action_status, primary_location, secondary_location, ternary_location, end_status "
         ") Values ("
         << QC(row.bill_id)            << QC(row.bill_history_id)  << QC(row.action_date) << QC(row.action)        << QC(row.trans_uid)
         << QC(row.trans_update_dt)    << QC(row.action_sequence)  << QC(row.action_code) << QC(row.action_status) << QC(row.primary_location)
         << QC(row.secondary_location) << QC(row.ternary_location) << Q (row.end_status)
         << ");";
      result = ConcatentateMultipleInsertsBeforeExecuting(ss.str(),db_public,false);
   }
   return result;
}

void capublic_bill_history_tbl::ParseAndWriteRow(const std::string& line,boost::weak_ptr<DB_capublic>& db_public) {
   BillHistoryTableRow row(ParseRowToStruct(line));
   Insert(row,db_public);
}

// Constructor ensures that the database table exists
capublic_bill_history_tbl::capublic_bill_history_tbl(boost::weak_ptr<DB_capublic> database,bool import_leg_data): db_public(database) {
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      if (import_leg_data) {
         if (wp->ExecuteSQL(sql_create_bill_history_tbl)) {
            LoggerNS::Logger::Log("Creating capublic_bill_history_tbl");

            InsertBillHistoryTable("D:/CCHR/2017-2018/LatestDownload/bill_history_tbl.dat");
         } else {
            LoggerNS::Logger::Log(std::string("Failed SQL \n") + sql_create_bill_history_tbl);
         }
      }
   }
}

void capublic_bill_history_tbl::InsertBillHistoryTable(const std::string& path) {
   ScopedElapsedTime elapsed_time("\tReading Bill_History_Tbl.dat","\tTable creation time: ");
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      auto file_contents(ReadFileLineByLine(path));
      const auto initial_row_count(wp->Count("bill_history_tbl",""));
      const auto new_row_count(file_contents.size());
      std::stringstream ss1;
      ss1 << "\tBill history table currently has " << initial_row_count << " database entries.  Bill_History_Tbl.dat has " << new_row_count << " entries. There are " << new_row_count-initial_row_count << " new entries.";
      LoggerNS::Logger::Log(ss1.str());

      // Clear table contents before inserting, since bill_tbl from the leg site lists only the latest versions of bills.
      if (CAPublicTablesNS::ClearTable(wp,"bill_history_tbl")) {
         // Parse each row into a struct, write the structs to the database table
         std::for_each(file_contents.begin(),file_contents.end(),[&](const std::string& line) {
            ParseAndWriteRow(line,db_public);
         });
         // Flush the concatenation buffer
         ConcatentateMultipleInsertsBeforeExecuting(std::string(),db_public,true);
      } else {
         LoggerNS::Logger::Log("Unable to clear bill_history_tbl");
      }
   }
   // Flush the sql accumulator.
   ConcatentateMultipleInsertsBeforeExecuting(std::string(),db_public,true);
   std::stringstream ss2;
   ss2 << "\tAfter creation and filling, bill_history_tbl has " << wp->Count("bill_history_tbl","") << " rows.";
   LoggerNS::Logger::Log(ss2.str());
}
