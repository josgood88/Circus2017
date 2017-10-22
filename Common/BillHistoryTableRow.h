#pragma once

#include <string>

struct BillHistoryTableRow {
   std::string bill_id;
   std::string bill_history_id;
   std::string action_date;
   std::string action;
   std::string trans_uid;
   std::string trans_update_dt;
   std::string action_sequence;
   std::string action_code;
   std::string action_status;
   std::string primary_location;
   std::string secondary_location;
   std::string ternary_location;
   std::string end_status;
};
