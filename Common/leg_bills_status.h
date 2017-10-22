#pragma once

#include <BillRanker.h>
#include <CommonTypes.h>
#include "db_capublic.h"
#include <LegBillRow.h>

#include <boost/weak_ptr.hpp>
#include <map>
#include <string>
#include <vector>

class leg_bills_status {
public:
   leg_bills_status(boost::weak_ptr<DB_capublic> database);
   ~leg_bills_status() {}
   bool Insert(const LegBillRow& row);
   std::vector<LegBillRow> FetchLegBillRow(const std::string& query);
   bool UpdateLegStatus(const std::vector<BillRanker::BillRanking>& rankings);
private:
   void InsertBillTable(const std::string& path);
   boost::weak_ptr<DB_capublic> db_public;
};
