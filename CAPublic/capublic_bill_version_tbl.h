#pragma once

#include <abstract_table.h>
#include "db_capublic.h"
#include <CommonTypes.h>
#include <boost/weak_ptr.hpp>
#include <Readers.h>
#include <map>
#include <string>

struct BillVersionTableRow {
   std::string bill_version_id;
   std::string bill_id;
   std::string version_num;
   std::string bill_version_action_date;
   std::string bill_version_action;
   std::string request_num;
   std::string subject;
   std::string vote_required;
   std::string appropriation;
   std::string fiscal_committee;
   std::string local_program;
   std::string substantive_changes;
   std::string urgency;
   std::string taxlevy;
   std::string bill_xml;
   std::string active_flg;
   std::string trans_uid;
   std::string trans_update;

   void Clear() { bill_version_id = bill_id = version_num =  bill_version_action_date =  bill_version_action = request_num = 
                  subject = vote_required = appropriation = fiscal_committee = local_program = substantive_changes = 
                  urgency = taxlevy = bill_xml = active_flg = trans_uid = trans_update = std::string();
   }
};

class capublic_bill_version_tbl : abstract_table {
public:
   capublic_bill_version_tbl(boost::weak_ptr<DB_capublic> database, bool import_leg_data);
   ~capublic_bill_version_tbl() {}
   bool Insert(const BillVersionTableRow& row);
   std::string BillIDFromLob(const std::string& lob);
   std::string Title(const std::string& lob);
   std::string BillVersionIDFromLob(const std::string& lob);
   std::string FieldQuery(const std::string& query);

   std::vector<BillRow> Read() { return Readers::ReadVectorVersionIdTbl(db_public); }
   std::vector<BillRow> Read(const std::vector<std::string>& selection)  { return Readers::ReadVectorVersionIdTbl(db_public,selection); }

   private:
   void InsertBillVersionTable(const std::string& path);
   bool Insert(const BillVersionTableRow& row,boost::weak_ptr<DB_capublic>& db_public);
   void ParseAndWriteRow(const std::string& line,boost::weak_ptr<DB_capublic>& db_public);

   boost::weak_ptr<DB_capublic> db_public;
};
