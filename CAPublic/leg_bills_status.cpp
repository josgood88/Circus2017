#include "BillRanker.h"
#include "CAPublicTablesNS.h"
#include "db_capublic.h"
#include "leg_bills_status.h"
#include <ElapsedTime.h>

#include <boost/shared_ptr.hpp>
#include <sstream>
#include <stdexcept>
#include <string>

#define Q  DB_capublic::Quote //measure_num latest_bill_version_id
#define QC DB_capublic::QuoteC

namespace {
   // Insert a token into a LegBillRow, based on the column offset into the row
   void SetColumn(LegBillRow& row, const std::string& token, unsigned int offset) {
      switch (offset) {
         case  0: row.score    = token; break;
         case  1: row.position = token; break;
         case  2: row.lob      = token; break;
         case  3: row.bill     = token; break;
         case  4: row.author   = token; break;
         case  5: row.title    = token; break;
         default: 
            std::stringstream ss;
            ss << "leg_bills_status InsertColumn: offset " << offset << " is out of range." << std::endl;
            throw new std::out_of_range(ss.str());
            break;
      }
   }

   // Read a line from Bill_Tbl.dat, parse out the fields into a row, and insert the row. 
   LegBillRow ParseRowToStruct(std::string& source, size_t& string_offset) {
      LegBillRow result;
      const char tab(0x09);
      const char newline(0x0a);
      // final item terminated by 0x0a, not 0x09
      auto elements_count(sizeof(LegBillRow) / sizeof(std::string));
      auto leading_offset = string_offset;   // Offset into the input string
      auto table_offset(0u);                 // Offset into the table

      for (table_offset=0; table_offset < elements_count-1; ++table_offset) {  
         leading_offset = source.find(tab,string_offset); // find the end of the next token
         SetColumn(result, CAPublicTablesNS::TrimFirstLastSingleQuote(source.substr(string_offset,leading_offset-string_offset)), table_offset);
         string_offset = leading_offset+1;
      }

      leading_offset = source.find(newline,string_offset);
      SetColumn(result, CAPublicTablesNS::TrimFirstLastSingleQuote(source.substr(string_offset,leading_offset-string_offset)), table_offset);
      string_offset = leading_offset+1;
      return result;
   }
}

// Partial LegBillRow specification, with the rest looked up in the database
//LegBillRow::LegBillRow(const std::string& c, const std::string& s, const std::string& l) : configuration(c), score(s), lob(l) {
//}

// Constructor ensures that the database table exists
leg_bills_status::leg_bills_status(boost::weak_ptr<DB_capublic> database) : db_public(database) {
   //boost::shared_ptr<DB_capublic> wp = db_public.lock();
   //if (wp) {
   //   wp->Report("\tleg_bills_status creation");
   //   if (wp->ExecuteSQL(sql_create_bill_tbl)) {
   //      InsertBillTable("D:/CCHR/2017-2018/LatestDownload/BILL_TBL.dat");
   //   }
   //}
}

void leg_bills_status::InsertBillTable(const std::string& path) {
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      wp->Report("\tleg_bills_status data fetch");
      boost::posix_time::ptime start_time(StartingTime());
      auto file_contents(CAPublicTablesNS::ReadFile(path));
      size_t string_offset(0);

      // Clear table contents before inserting, since bill_tbl from the leg site lists only the latest versions of bills.
      if (CAPublicTablesNS::ClearTable(wp,"bill_tbl")) {
         const boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::universal_time();
         while (string_offset < file_contents.length()) {
            LegBillRow row(ParseRowToStruct(file_contents, string_offset));
            Insert(row);
         }

         std::stringstream ss;
         ss << "\tbill_tbl has " << wp->Count("bill_tbl","") << " rows.";
         wp->Report(ss.str());
      } else {
         wp->Report("leg_bills_status::InsertBillTable was unable to clear the bill table.");
      }
      std::stringstream ss;
      ss << "\tInsert run time: " << ElapsedTime(start_time);
      wp->Report(ss.str());
   }
}

bool leg_bills_status::Insert(const LegBillRow& row) {
   //boost::shared_ptr<DB_capublic> wp = db_public.lock();
   //if (wp) {
   //   std::stringstream ss;
   //   ss << "Insert into bill_tbl ("
   //         "bill_id, session_year, session_num, measure_type, measure_num, measure_state, chapter_year, "
   //         "chapter_type, chapter_session_num, chapter_num, latest_bill_version_id, active_flg,"
   //         "trans_uid, trans_update, current_location, current_secondary_loc, current_house, current_status, days_31st_in_print) "
   //         "Values ("
   //      << QC(row.bill_id)                << QC(row.session_year)  << QC(row.session_num)    << QC(row.measure_type)         << QC(row.measure_num)
   //      << QC(row.measure_state)          << QC(row.chapter_year)  << QC(row.chapter_type)   << QC(row.chapter_session_num)  << QC(row.chapter_num) 
   //      << QC(row.latest_bill_version_id) << QC(row.active_flg)    << QC(row.trans_uid)      << QC(row.trans_update)         << QC(row.current_location)
   //      << QC(row.current_secondary_loc)  << QC(row.current_house) << QC(row.current_status) << Q (row.days_31st_in_print)
   //      << ");";
   //   if (!wp->ExecuteSQL(ss.str())) {
   //      wp->Report(std::string("Failed SQL ") + ss.str());
   //   }
   //}
   return true;
}

// Obtain data on each bill
namespace {
   std::map<bill_id_t,bill_version_id_t> version_id_result;
   bill_version_id_t ID;

   static int version_id_callback(void *count, int argc, char **argv, char **azColName) {
      const std::string bill_version_id("bill_id");

      const std::string col_name(*azColName);
      if (bill_version_id.compare(col_name) == 0) {
         if (argc == 2) version_id_result[std::string(argv[0])] = std::string(argv[1]);
      } else {
         //Report(std::string("version_id_callback: Unexpected type ") + col_name);
      }
      return 0;
   }
}

//std::map<bill_id_t,bill_version_id_t> leg_bills_status::BillMap() {
//   version_id_result.clear();
//   const std::string query("select bill_id,latest_bill_version_id from bill_tbl order by bill_id;");
//   boost::shared_ptr<DB_capublic> wp = db_public.lock();
//   if (wp) {
//      char *zErrMsg = 0;
//      if (sqlite3_exec(wp->db, query.c_str(), version_id_callback, &version_id_result, &zErrMsg) != SQLITE_OK) {
//         wp->Report(std::string("leg_bills_status::BillMap error: ") + (zErrMsg ? zErrMsg : "No Indication"));
//         sqlite3_free(zErrMsg);
//      }   
//   }
//   return version_id_result;
//}

// Fill a LegBillRow when called back by SQLite
namespace {
   std::vector<LegBillRow> result;

   static int row_fill_callback(void *count, int argc, char **argv, char **azColName) {
      LegBillRow row;
      try {
         row.score    = *argv++;
         row.position = *argv++;
         row.lob      = *argv++;
         row.bill     = *argv++;
         row.author   = *argv++;
         row.title    = *argv++;
         result.push_back(row);
      } catch (const std::exception& ) {
         return SQLITE_ABORT;
      }
      return 0;
   }
}

std::vector<LegBillRow> leg_bills_status::FetchLegBillRow(const std::string& query) {
   result.clear();;
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      char *zErrMsg = 0;
      if (sqlite3_exec(wp->db, query.c_str(), row_fill_callback, &version_id_result, &zErrMsg) != SQLITE_OK) {
         wp->Report(std::string("leg_bills_status::Fetch error: ") + (zErrMsg ? zErrMsg : "No Indication"));
         sqlite3_free(zErrMsg);
      }   
   }
   return result;
}

// Update this table with the latest ranking data.  There can be the following types of updates:
//    1) An updated bill will have a new lob file.  It may also have a new score.
//    2) New bills appear and need to be added to the table.
//    3) An updated bill may have new authors or title.
// This method isn't concerned with updating our position.  New additions have a position of None, an existing bill's position isn't updated here.
bool leg_bills_status::UpdateLegStatus(const std::vector<BillRanker::BillRanking>& rankings) {
   std::vector<LegBillRow> reviewed_lobs(FetchLegBillRow("Select Score, Position, Lob, Bill, Author, Title from leg_bills_status"));
   return true;
}
