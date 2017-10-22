#include "capublic_bill_version_authors_tbl.h"
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
   const std::string sql_create_bill_version_authors_tbl(
      "CREATE TABLE IF NOT EXISTS bill_version_authors_tbl (" 
      "bill_version_id             TEXT NOT NULL, "
      "type                        TEXT NOT NULL, "
      "house                       TEXT NULL, "
      "name                        TEXT NULL, "
      "contribution                TEXT NULL, "
      "committee_members           TEXT NULL, "
      "active_flg                  TEXT NULL, "
      "trans_uid                   TEXT NULL, "
      "trans_update                TEXT NULL, "
      "primary_author_flg          TEXT NULL"
      ");"
   );

   // Insert a token into a BillAuthorsTableRow, based on the column offset into the row
   void SetColumn(BillAuthorsTableRow& row, const std::string& _token, unsigned int offset) {
      const std::string token(CAPublicTablesNS::SQLQuote(_token));
      switch (offset) {
         case  0: row.bill_version_id    = token; break;
         case  1: row.type               = token; break;
         case  2: row.house              = token; break;
         case  3: row.name               = token; break;
         case  4: row.contribution       = token; break;
         case  5: row.committee_members  = token; break;
         case  6: row.active_flg         = token; break;
         case  7: row.trans_uid          = token; break;
         case  8: row.trans_update       = token; break;
         case  9: row.primary_author_flg = token; break;
         default: 
            std::stringstream ss;
            ss << "capublic_bill_version_authors_tbl InsertColumn: offset " << offset << " is out of range." << std::endl;
            throw new std::out_of_range(ss.str());
            break;
      }
   }

   // Read a line from bill_version_authors_tbl, parse out the fields into a row, and insert the row. 
   BillAuthorsTableRow ParseRowToStruct(const std::string& line) {
      BillAuthorsTableRow result;
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

bool capublic_bill_version_authors_tbl::Insert(const BillAuthorsTableRow& row) {
   bool result(false);
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      std::stringstream ss;
      ss << "Insert into bill_version_authors_tbl ("
         << "bill_version_id, " << "type, "      << "house, "        << "name, " << "contribution, " << "committee_members, "
         << "active_flg, "      << "trans_uid, " << "trans_update, " << "primary_author_flg) "
         << "Values ("
         << QC(row.bill_version_id) << QC(row.type)      << QC(row.house)        << QC(row.name) << QC(row.contribution) << QC(row.committee_members)
         << QC(row.active_flg)      << QC(row.trans_uid) << QC(row.trans_update) << Q (row.primary_author_flg)
         << ");";
      result = ConcatentateMultipleInsertsBeforeExecuting(ss.str(),db_public,false);
   }
   return result;
}

void capublic_bill_version_authors_tbl::ParseAndWriteRow(const std::string& line,boost::weak_ptr<DB_capublic>& db_public) {
   BillAuthorsTableRow row(ParseRowToStruct(line));
   Insert(row);
}

// Constructor ensures that the database table exists
capublic_bill_version_authors_tbl::capublic_bill_version_authors_tbl(boost::weak_ptr<DB_capublic> database,bool import_leg_data): db_public(database) {
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      if (import_leg_data) {
         if (wp->ExecuteSQL(sql_create_bill_version_authors_tbl)) {
            LoggerNS::Logger::Log("Creating capublic_bill_version_authors_tbl");
            InsertBillAuthorsTable("D:/CCHR/2017-2018/LatestDownload/bill_version_authors_tbl.dat");
         } else {
            LoggerNS::Logger::Log(std::string("Failed SQL \n") + sql_create_bill_version_authors_tbl);
         }
      }
   }
}

void capublic_bill_version_authors_tbl::InsertBillAuthorsTable(const std::string& path) {
   ScopedElapsedTime elapsed_time("\tReading Bill_Version_Authors_Tbl.dat","\tTable creation time: ");
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      auto file_contents(ReadFileLineByLine(path));
      const auto initial_row_count(wp->Count("bill_version_authors_tbl",""));
      const auto new_row_count(file_contents.size());
      std::stringstream ss1;
      ss1 << "\tBill version authors table currently has " << initial_row_count << " database entries.  Bill_Version_Authors_Tbl.dat has " << new_row_count << " entries. There are " << new_row_count-initial_row_count << " new entries.";
      LoggerNS::Logger::Log(ss1.str());

      // Clear table contents before inserting, since bill_tbl from the leg site lists only the latest versions of bills.
      if (CAPublicTablesNS::ClearTable(wp,"bill_version_authors_tbl")) {
         // Parse each row into a struct, write the structs to the database table
         std::for_each(file_contents.begin(),file_contents.end(),[&](const std::string& line) {
            ParseAndWriteRow(line,db_public);
         });
         // Flush the concatenation buffer
         ConcatentateMultipleInsertsBeforeExecuting(std::string(),db_public,true);
      } else {
         LoggerNS::Logger::Log("Unable to clear bill_version_authors_tbl");
      }
   }
   // Flush the sql accumulator.
   ConcatentateMultipleInsertsBeforeExecuting(std::string(),db_public,true);
   std::stringstream ss2;
   ss2 << "\tAfter creation and filling, bill_version_authors_tbl has " << wp->Count("bill_version_authors_tbl","") << " rows.";
   LoggerNS::Logger::Log(ss2.str());
}

std::string capublic_bill_version_authors_tbl::Author(const std::string& id) { 
   std::stringstream query;
   query << "Select name From bill_version_authors_tbl Where bill_version_id = '" << id << "' And primary_author_flg = 'Y';" ;
   return Readers::ReadSingleString(db_public,query.str());
}
