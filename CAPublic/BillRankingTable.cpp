#include "db_capublic.h"
#include "BillRanker.h"
#include <BillRankingTable.h>
#include <Readers.h>

#include <boost/shared_ptr.hpp>
#include <sstream>
#include <vector>

#define Q  DB_capublic::Quote
#define QC DB_capublic::QuoteC

namespace {
   std::string ToString(int arg) { std::stringstream ss; ss << arg; std::string result; ss >> result; return result; }
}

BillRankingTable::BillRankingTable(boost::weak_ptr<DB_capublic> database, bool import_leg_data) : db_public(database) { }

// Insert a single BillRow into the database
bool BillRankingTable::Insert(const BillRanker::BillRanking& row) {
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      std::stringstream ss;
      ss << "Insert into bill_rankings_tbl (configuration, path, word_score) Values ("
         << Q(row.configuration) << ", " << Q(row.path) << ", " << Q(ToString(row.word_score)) 
         << ");";
      if (!wp->ExecuteSQL(ss.str())) {
         wp->Report(std::string("Failed SQL \n") + ss.str());
      }
   }
   return true;
}

bool BillRankingTable::Write(const std::vector<BillRanker::BillRanking>& table) {
   const std::string cmd_delete("Delete From bill_rankings_tbl;");
   const std::string cmd_vacuum("VACUUM;");
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      wp->Report("Writing bill ranking table");
      if (wp->ExecuteSQL(cmd_delete)) {
         if (wp->ExecuteSQL(cmd_vacuum)) {
            std::for_each(table.begin(), table.end(), [&](const BillRanker::BillRanking& row) {
               Insert(row);
            });
         } else {
            wp->Report(std::string("Failed SQL \n") + cmd_delete);
         }
      } else {
         wp->Report(std::string("Failed SQL \n") + cmd_vacuum);
      }
      wp->Report("Through writing bill ranking table");
   }
   return true;
}

std::vector<BillRanker::BillRanking> BillRankingTable::Read() {
   return Readers::ReadVectorBillRanking(db_public);
}

namespace {
   std::string ReadSingleString(boost::weak_ptr<DB_capublic>& db_public, const std::string& query, const std::string& arg) {
      std::stringstream ss;
      ss << query << arg << "';";
      return Readers::ReadSingleString(db_public,ss.str());
   }

   score_t ScoreCommon(boost::weak_ptr<DB_capublic>& db_public, const std::string& query, const std::string& lob) {
      std::stringstream ss;
      score_t result;
      ss << ReadSingleString(db_public,query,lob);
      ss >> result;
      return result;
   }
}

score_t BillRankingTable::NegativeScore(const std::string& lob) {
   const std::string query("Select word_score from bill_rankings_tbl where configuration='Negative' and path = '");
   return ScoreCommon(db_public,query,lob);
}

score_t BillRankingTable::PositiveScore(const std::string& lob) {
   const std::string query("Select word_score from bill_rankings_tbl where configuration='Positive' and path = '");
   return ScoreCommon(db_public,query,lob);
}
