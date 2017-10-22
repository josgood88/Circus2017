#pragma once

#include <string>

struct LegBillRow {
   std::string configuration;
   std::string score;
   std::string position;
   std::string lob;
   std::string bill_id;             // e.g., 201720180AB1
   std::string bill;                // e.g., AB1
   std::string bill_version_id;     // e.g., 20170SCR1499INT
   std::string author;
   std::string title;

LegBillRow() {}
LegBillRow(const std::string& c, const std::string& s, const std::string& l) : configuration(c), score(s), lob(l) {}
bool operator< (const LegBillRow &rhs) const { return bill < rhs.bill; }
};
