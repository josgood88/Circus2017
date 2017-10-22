#pragma once

#include <abstract_table.h>
#include "db_capublic.h"
#include <boost/weak_ptr.hpp>
#include <string>

struct LocationTableRow {
   std::string session_year;
   std::string location_code;
   std::string location_type;
   std::string consent_calendar_code;
   std::string description;
   std::string long_description;
   std::string active_flg;
   std::string trans_uid;
   std::string trans_update;
   std::string inactive_file_flg;
};

class capublic_location_code_tbl : abstract_table {
public:
   capublic_location_code_tbl(boost::weak_ptr<DB_capublic> database, bool import_leg_data);
   ~capublic_location_code_tbl() {}
   std::string FieldQuery(const std::string& query);
private:
   bool Insert(const LocationTableRow& row);
   void InsertLocationTable(const std::string& path);
   void ParseAndWriteRow(const std::string& line,boost::weak_ptr<DB_capublic>& db_public);

   boost::weak_ptr<DB_capublic> db_public;
};
