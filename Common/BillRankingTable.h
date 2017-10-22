#pragma once

#include <BillRanker.h>
#include <CommonTypes.h>
#include <db_capublic.h>

#include <boost/weak_ptr.hpp>
#include <map>
#include <string>
#include <vector>

class BillRankingTable {
public:
   BillRankingTable(boost::weak_ptr<DB_capublic> database, bool import_leg_data);
   ~BillRankingTable() {}
   std::vector<BillRanker::BillRanking> Read();
   bool Write(const std::vector<BillRanker::BillRanking>& table);
   score_t NegativeScore(const std::string& lob);
   score_t PositiveScore(const std::string& lob);

private:
   boost::weak_ptr<DB_capublic> db_public;
   bool Insert(const BillRanker::BillRanking& row);
};
