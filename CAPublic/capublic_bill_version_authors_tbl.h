#ifndef capublic_bill_version_authors_tbl_h
#define capublic_bill_version_authors_tbl_h

#include <abstract_table.h>
#include "db_capublic.h"
#include <boost/weak_ptr.hpp>
#include <string>

struct BillAuthorsTableRow {
   std::string bill_version_id;
   std::string type;
   std::string house;
   std::string name;
   std::string contribution;
   std::string committee_members;
   std::string active_flg;
   std::string trans_uid;
   std::string trans_update;
   std::string primary_author_flg;
};

class capublic_bill_version_authors_tbl : abstract_table {
public:
   capublic_bill_version_authors_tbl(boost::weak_ptr<DB_capublic> database, bool import_leg_data);
   ~capublic_bill_version_authors_tbl() {}
   std::string Author(const std::string& bill_ID);
private:
   void InsertBillAuthorsTable(const std::string& path);
   bool Insert(const BillAuthorsTableRow& row);
   void ParseAndWriteRow(const std::string& line,boost::weak_ptr<DB_capublic>& db_public);
   boost::weak_ptr<DB_capublic> db_public;
};

#endif
