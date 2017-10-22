#pragma once

#include "db_capublic.h"

#include <boost/weak_ptr.hpp>
#include <string>
#include <vector>

typedef std::vector<std::string> TableRow;
typedef std::vector<TableRow>    TableRowSet;

namespace CAPublicTablesNS {
   bool        ClearTable(boost::weak_ptr<DB_capublic> db, const std::string& table_name);
   TableRowSet ParseRowsToVector(std::string file_contents, TableRow& row);
   void        ReadFields(const std::string& source, size_t& trailing_offset, std::vector<std::string *>& results);
   std::string ReadFile(const std::string& path);
   void        Report(const std::string& message);
   void        ReportException(const std::string& announce, const std::exception& ex);
   std::string SQLQuote(const std::string& source);
   std::string TrimFirstLastSingleQuote(const std::string& source);
}
