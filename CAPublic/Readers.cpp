#pragma once

#include "db_capublic.h"
#include <BillRow.h>
#include <Readers.h>
#include <Utility.h>

#include <boost/weak_ptr.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace {
   std::string single_string;
   std::vector<std::string> vector_string;
   std::vector<std::vector<std::string>> list_vector_string;
   unsigned int ToUInt(std::string arg) { std::stringstream ss; ss << arg; int result; ss >> result; return result; }

   int single_std_string_callback(void *result, int argc, char **argv, char **azColName) {
      if (argc == 1) {
         std::string* ptr = reinterpret_cast<std::string*>(result);
         *ptr = std::string(argv[0]);
      }
      return SQLITE_OK;
   }

   std::string read_single_string(boost::weak_ptr<DB_capublic>& db_public, const std::string& query) {
      boost::shared_ptr<DB_capublic> wp = db_public.lock();
      if (wp) {
         single_string.clear();
         char *zErrMsg = 0;
         if (sqlite3_exec(wp->db, query.c_str(), single_std_string_callback, &single_string, &zErrMsg) != SQLITE_OK) {
            wp->Report(std::string("read_single_string::SQL error: ") + (zErrMsg ? zErrMsg : "No Indication"));
            wp->Report(std::string("\t") + query);
            sqlite3_free(zErrMsg);
         }   
      }
      return single_string;
   }
   static int vector_string_callback(void *count, int argc, char **argv, char **azColName) {
      try {
         vector_string.push_back(*argv++);
         return SQLITE_OK;
      } catch (const std::exception& ) {
         return SQLITE_ABORT;
      }
   }
   static int list_vector_callback(void *count, int argc, char **argv, char **azColName) {
      vector_string.clear();
      unsigned int limit = argc;
      try {
         single_string.clear();
         const std::string single_quote(1,0x27);
         const std::string two_single_quotes(std::string(2,char(0x27)));
         while (limit--) {
            std::string part(*argv++);
            UtilityRemoveHTML(part); 
            vector_string.push_back(std::regex_replace(part,std::regex(two_single_quotes),single_quote));
         }
         list_vector_string.push_back(vector_string);
         return SQLITE_OK;
      } catch (const std::exception& ) {
         return SQLITE_ABORT;
      }
   }
   std::vector<std::vector<std::string>> read_list_vector_string(boost::weak_ptr<DB_capublic>& db_public, const std::string& query) {
      list_vector_string.clear();
      boost::shared_ptr<DB_capublic> wp = db_public.lock();
      if (wp) {
         char *zErrMsg = 0;
         if (sqlite3_exec(wp->db, query.c_str(), list_vector_callback, &list_vector_string, &zErrMsg) != SQLITE_OK) {
            wp->Report(std::string("read_list_vector_string::SQL error: ") + (zErrMsg ? zErrMsg : "No Indication"));
            wp->Report(std::string("\t") + query);
            sqlite3_free(zErrMsg);
         }   
      }
      return list_vector_string;
   }
}

namespace Readers {
   std::string ReadSingleString(boost::weak_ptr<DB_capublic>& db_public, const std::string& query) {
      return read_single_string(db_public,query);
   }

   
   std::vector<std::string> ReadVectorString(boost::weak_ptr<DB_capublic>& db_public, const std::string& query) {
      vector_string.clear();
      boost::shared_ptr<DB_capublic> wp = db_public.lock();
      if (wp) {
         char *zErrMsg = 0;
         if (sqlite3_exec(wp->db, query.c_str(), vector_string_callback, &vector_string, &zErrMsg) != SQLITE_OK) {
            wp->Report(std::string("ReadVectorString::SQL error: ") + (zErrMsg ? zErrMsg : "No Indication"));
            wp->Report(std::string("\t") + query);
            sqlite3_free(zErrMsg);
         }   
      }
      return vector_string;
   }

   std::vector<std::vector<std::string>> ReadVectorVector(boost::weak_ptr<DB_capublic>& db_public, const std::string& query) {
      return read_list_vector_string(db_public,query);
   }

   std::vector<BillRow> ReadVectorBillRow(boost::weak_ptr<DB_capublic>& db_public) {
      const std::string query("Select Change, MeasureType, MeasureNum, NegativeScore, PositiveScore, Position, Lob, BillId, BillVersionId, Bill, Author, Title from BillRows");
      return ReadVectorBillRow(db_public, query);
   }

   std::vector<BillRow> ReadVectorBillRow(boost::weak_ptr<DB_capublic>& db_public, const std::string& query) {
      std::vector<BillRow> result;
      std::vector<std::vector<std::string>> v = read_list_vector_string(db_public,query);
      std::for_each(v.begin(),v.end(),[&](const std::vector<std::string>& entry) {
         if (entry.size() == 12) {
            auto itr(entry.begin());
            auto change          = *itr++;
            auto measure_type    = *itr++;
            auto measure_num     = *itr++;
            auto neg_score       = DB_capublic::ToUInt(*itr++);
            auto pos_score       = DB_capublic::ToUInt(*itr++);
            auto position        = *itr++;
            auto lob             = *itr++;
            auto bill_id         = *itr++;
            auto bill_version_id = *itr++;
            auto bill            = *itr++;
            auto author          = *itr++;
            auto title           = *itr++;
            auto new_row = BillRow(change,neg_score,pos_score, measure_type,measure_num, position, lob, bill_id, bill_version_id, bill, author, title);
            result.push_back(new_row);
         }
      });
      return result;
   }

   std::vector<BillRow> ReadVectorVersionIdTbl(boost::weak_ptr<DB_capublic>& db_public) {
      std::vector<BillRow> result;
      const std::string query("Select bill_id, bill_version_id, bill_xml from bill_version_tbl");
      std::vector<std::vector<std::string>> v = read_list_vector_string(db_public,query);
      std::for_each(v.begin(),v.end(),[&](const std::vector<std::string>& entry) {
         if (entry.size() == 3) {
            auto itr(entry.begin());
            const auto bill_id(*itr++);
            const auto bill_ver(*itr++);
            const auto lob(*itr++);
            result.push_back(BillRow(bill_id,bill_ver,lob));
         }
      });
      return result;
   }

   std::vector<BillRow> ReadVectorVersionIdTbl(boost::weak_ptr<DB_capublic>& db_public, const std::vector<std::string>& selection) {
      std::vector<BillRow> result;
      const std::string query("Select bill_id, bill_version_id, bill_xml from bill_version_tbl Where bill_xml = '");
      std::for_each(selection.begin(),selection.end(),[&](const std::string member) {
         std::stringstream ss;
         ss << query << member << "';";
         auto vector_of_vector_of_string = read_list_vector_string(db_public,ss.str());
         auto vector_of_string = vector_of_vector_of_string[0];
         if (vector_of_string.size() == 3) {          // Must have 3 result strings (3 selections in query)
               auto itr = vector_of_string.begin();
               BillRow b;
               b.bill_id         = *itr++;
               b.bill_version_id = *itr++;
               b.lob             = *itr++;
               result.push_back(b);
         }
      });
      return result;
   }

   std::vector<BillRow> ReadVectorLegBillTable(boost::weak_ptr<DB_capublic>& db_public) {
      std::vector<BillRow> result;
      const std::string query("Select bill_id, measure_num, measure_type from bill_tbl");
      std::vector<std::vector<std::string>> v = read_list_vector_string(db_public,query);
      std::for_each(v.begin(),v.end(),[&](const std::vector<std::string>& entry) {
         if (entry.size() == 3) {
            auto itr(entry.begin());
            BillRow b;
            b.bill_id      = *itr++;
            b.measure_num  = *itr++;
            b.measure_type = *itr++;
            result.push_back(b);
         }
      });
      return result;
   }

   std::vector<std::vector<std::string>> BillHistory(boost::weak_ptr<DB_capublic>& db_public, const std::string& bill_id) {
      std::stringstream ss;
      ss << "Select action_date, action from bill_history_tbl where bill_id = '" << bill_id << "';";
      return read_list_vector_string(db_public,ss.str());
   }
}
