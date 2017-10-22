#ifndef capublic_bill_tbl_h
#define capublic_bill_tbl_h

#include <abstract_table.h>
#include <BillRow.h>
#include <CommonTypes.h>
#include "db_capublic.h"

#include <boost/weak_ptr.hpp>
#include <map>
#include <string>

struct BillTableRow {
   std::string bill_id;
   std::string session_year;
   std::string session_num;
   std::string measure_type;
   std::string measure_num;
   std::string measure_state;
   std::string chapter_year;
   std::string chapter_type;
   std::string chapter_session_num;
   std::string chapter_num;
   std::string latest_bill_version_id;
   std::string active_flg;
   std::string trans_uid;
   std::string trans_update;
   std::string current_location;
   std::string current_secondary_loc;
   std::string current_house;
   std::string current_status;
   std::string days_31st_in_print;
};

class capublic_bill_tbl : abstract_table {
public:
   capublic_bill_tbl(boost::weak_ptr<DB_capublic> database, bool import_leg_data);
   ~capublic_bill_tbl() {}
   std::vector<BillRow> capublic_bill_tbl::Read();
   std::string MeasureType(const std::string& id);
   std::string MeasureNum(const std::string& id);
   std::string FieldQuery(const std::string& query);
private:
   void InsertBillTable(const std::string& path);
   bool Insert(const BillTableRow& row,boost::weak_ptr<DB_capublic>& db_public);
   void ParseAndWriteRow(const std::string& line,boost::weak_ptr<DB_capublic>& db_public);

   boost::weak_ptr<DB_capublic> db_public;
};

#endif
