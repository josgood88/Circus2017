#pragma once

//#include <BillRanker.h>
#include <BillRow.h>
#include "db_capublic.h"

#include <boost/weak_ptr.hpp>
#include <vector>

class BillRowTable {
public:
   BillRowTable(boost::weak_ptr<DB_capublic> database, bool import_leg_data);
   ~BillRowTable() {}
   std::vector<BillRow> Read();
   bool Insert(const BillRow& row);
   std::string FieldQuery(const std::string& query);
   BillRow ReadSingleBillRow(const std::string& measure_type, const std::string& measure_num);
   bool Replace(const std::vector<BillRow>& bill_row_table);
   bool Update(const BillRow& row);
private:
   boost::weak_ptr<DB_capublic> db_public;
};
