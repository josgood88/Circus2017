#ifndef capublic_bill_history_tbl_h
#define capublic_bill_history_tbl_h

#include <abstract_table.h>
#include <BillHistoryTableRow.h>
#include <Readers.h>

#include "db_capublic.h"
#include <boost/weak_ptr.hpp>
#include <string>

class capublic_bill_history_tbl : abstract_table {
public:
   capublic_bill_history_tbl(boost::weak_ptr<DB_capublic> database, bool import_leg_data);
   ~capublic_bill_history_tbl() {}
   bool Insert(const BillHistoryTableRow& row);
   std::vector<std::vector<std::string>> BillHistory(const std::string& bill_id) { return Readers::BillHistory(db_public, bill_id); }
private:
   void InsertBillHistoryTable(const std::string& path);
   bool Insert(const BillHistoryTableRow& row,boost::weak_ptr<DB_capublic>& db_public);
   void ParseAndWriteRow(const std::string& line,boost::weak_ptr<DB_capublic>& db_public);

   boost::weak_ptr<DB_capublic> db_public;
};

#endif
