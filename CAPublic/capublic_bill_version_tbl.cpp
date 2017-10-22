#include "capublic_bill_version_tbl.h"
#include "CAPublicTablesNS.h"
#include <CommonTypes.h>
#include "db_capublic.h"
#include <ElapsedTime.h>
#include "Logger.h"
#include <Readers.h>
#include "ScopedElapsedTime.h"
#include <Utility.h>

#include <algorithm>
#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

namespace fs = boost::filesystem;

namespace {
   const std::string sql_create_version_bill_tbl(
      "CREATE TABLE IF NOT EXISTS bill_version_tbl ("
      "bill_version_id          TEXT NOT NULL, "
      "bill_id                  TEXT NOT NULL, "
      "version_num              TEXT NOT NULL, "
      "bill_version_action_date TEXT NOT NULL, "
      "bill_version_action      TEXT NULL, "
      "request_num              TEXT NULL, "
      "subject                  TEXT NULL, "
      "vote_required            TEXT NULL, "
      "appropriation            TEXT NULL, "
      "fiscal_committee         TEXT NULL, "
      "local_program            TEXT NULL, "
      "substantive_changes      TEXT NULL, "
      "urgency                  TEXT NULL, "
      "taxlevy                  TEXT NULL, "
      "bill_xml                 TEXT NULL, "
      "active_flg               TEXT NULL, "
      "trans_uid                TEXT NULL, "
      "trans_update             TEXT NULL"
      ");"
   );

   // Insert a token into a BillVersionTableRow, based on the column offset into the row
   void SetColumn(BillVersionTableRow& row,const std::string& token,unsigned int offset) {
      switch (offset) {
      case  0: row.bill_version_id          = token; break;
      case  1: row.bill_id                  = token; break;
      case  2: row.version_num              = token; break;
      case  3: row.bill_version_action_date = token; break;
      case  4: row.bill_version_action      = token; break;
      case  5: row.request_num              = token; break;
      case  6: row.subject                  = token; break;
      case  7: row.vote_required            = token; break;
      case  8: row.appropriation            = token; break;
      case  9: row.fiscal_committee         = token; break;
      case 10: row.local_program            = token; break;
      case 11: row.substantive_changes      = token; break;
      case 12: row.urgency                  = token; break;
      case 13: row.taxlevy                  = token; break;
      case 14: row.bill_xml                 = token; break;
      case 15: row.active_flg               = token; break;
      case 16: row.trans_uid                = token; break;
      case 17: row.trans_update             = token; break;
      default:
         std::stringstream ss;
         ss << "capublic_bill_version_tbl InsertColumn: offset " << offset << " is out of range." << std::endl;
         throw new std::out_of_range(ss.str());
         break;
      }
   }

   // Bill_Version_Tbl can have incorrect data.
   // Check the LOB file by opening it and comparing measure type and measure Num to the LOB file contents "<caml:MeasureType>AB</caml:MeasureType><caml:MeasureNum>404<"
   // If the data is wrong, log it and set the result.bill_xml to "Wrong lob file"
   //bool DoesInputDataPassMuster(const BillVersionTableRow& row, size_t offset) {

   //   // Get measure_type and measure_num from the lob file.
   //   const std::string raw_lob_files_folder("D:/CCHR/2017-2018/LatestDownload/Bills");
   //   const std::string marker_measure_type("<caml:MeasureType>");
   //   const std::string marker_measure_num ("<caml:MeasureNum>");
   //   const std::string file_contents(ReadFile(raw_lob_files_folder + "/" + row.bill_xml));
   //   const auto begin_measure_type (file_contents.find(marker_measure_type) + marker_measure_type.length());
   //   const auto begin_measure_num  (file_contents.find(marker_measure_num)  + marker_measure_num.length());
   //   const auto length_measure_type(file_contents.find("<",begin_measure_type) - begin_measure_type);
   //   const auto length_measure_num (file_contents.find("<",begin_measure_num)  - begin_measure_num);
   //   const auto measure_type(file_contents.substr(begin_measure_type,length_measure_type));
   //   const auto measure_num (file_contents.substr(begin_measure_num,length_measure_num));

   //   // Get measure_type and measure_num from the BillVersionTableRow.
   //   const std::string prefix("201720180");
   //   const std::string type_and_num(row.bill_id.substr(prefix.length()));
   //   const size_t offset_num(type_and_num.find_first_of("1234567890"));

   //   // Version table data is OK if the BillVersionTableRow measure type and number match the lob file contents.
   //   if (offset_num != std::string::npos) {
   //         std::stringstream ss;
   //      if (measure_type == type_and_num.substr(0,offset_num) && (measure_num == type_and_num.substr(offset_num))) {
   //         ss << row.bill_id << " is OK.";
   //         LoggerNS::Logger::Log(ss.str());
   //         return true;
   //      } else {
   //         ss << offset << ", " << row.bill_id << ", " << measure_type << ", " << type_and_num.substr(0,offset_num) << ", " << measure_num << ", " << type_and_num.substr(offset_num);
   //         LoggerNS::Logger::Log(ss.str());
   //      }
   //   }
   //   return false;
   //}

   // Read a line from Bill_Version_TBL, parse out the fields into a row, and insert the row. 
   BillVersionTableRow ParseRowToStruct(const std::string& line) {
      BillVersionTableRow result;
      auto table_offset(0u);                 // Offset into the table

      const std::regex splitOn("\\t");
      std::sregex_token_iterator itr(line.begin(),line.end(),splitOn,-1);
      const std::sregex_token_iterator endOfSequence;
      while (itr != endOfSequence) {
         const auto token((*itr++).str());               // Need string, not sub_match
         SetColumn(result,CAPublicTablesNS::TrimFirstLastSingleQuote(token),table_offset++);
      }
      return result;
//    return DoesInputDataPassMuster(row,leading_offset);
   }
}


bool capublic_bill_version_tbl::Insert(const BillVersionTableRow& row,boost::weak_ptr<DB_capublic>& db_public) {
   bool result(false);
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      std::stringstream ss;
      ss << "Insert into bill_version_tbl ("
         << "bill_version_id, bill_id, version_num,   bill_version_action_date, bill_version_action, "
         << "request_num,     subject, vote_required, appropriation,            fiscal_committee, "
         << "local_program,   substantive_changes,    urgency,                  taxlevy, "
         << "bill_xml,        active_flg,             trans_uid,                trans_update) "
         << "Values ("
         << QC(row.bill_version_id) << QC(row.bill_id) << QC(row.version_num)   << QC(row.bill_version_action_date) << QC(row.bill_version_action)
         << QC(row.request_num)     << QC(row.subject) << QC(row.vote_required) << QC(row.appropriation)            << QC(row.fiscal_committee)
         << QC(row.local_program)   << QC(row.substantive_changes)              << QC(row.urgency)                  << QC(row.taxlevy)
         << QC(row.bill_xml)        << QC(row.active_flg)                       << QC(row.trans_uid)                << Q (row.trans_update)
         << ");";
      result = ConcatentateMultipleInsertsBeforeExecuting(ss.str(),db_public,false);
   }
   return result;
}

void capublic_bill_version_tbl::ParseAndWriteRow(const std::string& line,boost::weak_ptr<DB_capublic>& db_public) {
   BillVersionTableRow row(ParseRowToStruct(line));
   Insert(row,db_public);
}

// Constructor ensures that the database table exists
capublic_bill_version_tbl::capublic_bill_version_tbl(boost::weak_ptr<DB_capublic> database,bool import_leg_data): db_public(database) {
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      if (import_leg_data) {
         if (wp->ExecuteSQL(sql_create_version_bill_tbl)) {
            LoggerNS::Logger::Log("Creating capublic_bill_version_tbl");
            InsertBillVersionTable("D:/CCHR/2017-2018/LatestDownload/BILL_VERSION_TBL.dat");
         } else {
            LoggerNS::Logger::Log(std::string("Failed SQL \n") + sql_create_version_bill_tbl);
         }
      }
   }
}

void capublic_bill_version_tbl::InsertBillVersionTable(const std::string& path) {
   ScopedElapsedTime elapsed_time("\tReading Bill_Version_Tbl.dat","\tTable creation time: ");
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      auto file_contents(ReadFileLineByLine(path));
      const auto initial_row_count(wp->Count("bill_version_tbl",""));
      const auto new_row_count(file_contents.size());
      std::stringstream ss1;
      ss1 << "\tBill version table currently has " << initial_row_count << " database entries.  Bill_Version_Tbl.dat has " << new_row_count << " entries. There are " << new_row_count-initial_row_count << " new entries.";
      LoggerNS::Logger::Log(ss1.str());

      // Clear table contents before inserting, since bill_tbl from the leg site lists only the latest versions of bills.
      if (CAPublicTablesNS::ClearTable(wp,"bill_version_tbl")) {
         // Parse each row into a struct, write the structs to the database table
         std::for_each(file_contents.begin(),file_contents.end(),[&](const std::string& line) {
            ParseAndWriteRow(line,db_public);
         });
         // Flush the concatenation buffer
         ConcatentateMultipleInsertsBeforeExecuting(std::string(),db_public,true);
      } else {
         LoggerNS::Logger::Log("Unable to clear bill_version_tbl");
      }
   }
   // Flush the sql accumulator.
   ConcatentateMultipleInsertsBeforeExecuting(std::string(),db_public,true);
   std::stringstream ss2;
   ss2 << "\tAfter creation and filling, bill_version_tbl has " << wp->Count("bill_version_tbl","") << " rows.";
   LoggerNS::Logger::Log(ss2.str());
}

// Obtain a map of the latest version of each bill to the .lob file that contains its contents
namespace {
   std::map<bill_ver_id_t,bill_xml_t> lob_result;
   bill_ver_id_t ID;

   static int lob_callback(void *count,int argc,char **argv,char **azColName) {
      const std::string bill_version_id("bill_version_id");

      const std::string col_name(*azColName);
      if (bill_version_id.compare(col_name) == 0) {
         if (argc == 2) lob_result [ std::string(argv [ 0 ]) ] = std::string(argv [ 1 ]);
      } else {
         //Report(std::string("lob_callback: Unexpected type ") + col_name);
      }
      return SQLITE_OK;
   }
}

namespace {
   std::string ReadSingleString(boost::weak_ptr<DB_capublic>& db_public,const std::string& query,const std::string& arg) {
      std::stringstream ss;
      ss << query << arg << "';";
      return Readers::ReadSingleString(db_public,ss.str());
   }
}

// Fetch the Bill ID whose text is contained in a specified lob file
std::string capublic_bill_version_tbl::BillIDFromLob(const std::string& lob) {
   const std::string query("Select bill_id From bill_version_tbl Where bill_xml = '");
   return ReadSingleString(db_public,query,lob);
}

// Fetch the Version ID for the Bill whose text is contained in a specified LOB file
std::string capublic_bill_version_tbl::BillVersionIDFromLob(const std::string& lob) {
   const std::string query("Select bill_version_id From bill_version_tbl Where bill_xml = '");
   return ReadSingleString(db_public,query,lob);
}

// Fetch the Title for the Bill whose text is contained in a specified LOB file
std::string capublic_bill_version_tbl::Title(const std::string& lob) {
   const std::string query("Select subject From bill_version_tbl Where bill_xml = '");
   return ReadSingleString(db_public,query,lob);
}

std::string capublic_bill_version_tbl::FieldQuery(const std::string& query) {
   return Readers::ReadSingleString(db_public,query);
}
