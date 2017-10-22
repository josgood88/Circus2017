#ifndef DB_h
#define DB_h

#include "RowBillTable.h"

// Any project including DB.cpp must define OTL_STL in the project property sheet, to enable std::string <==> varchar/char
#include <OTL/otlv4.h>     // http://otl.sourceforge.net/otl3.htm

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <string>

class RowBillTable;   // DB and RowBillTable include each other.  This avoids C2061.

class DB : public boost::noncopyable {
   friend class RowBillTable;
public:
   DB();
   DB(const std::string& connectString, const std::string& databaseName);
   ~DB();

   // Actions on any table
   long NonQuery(const std::string& nonQuery);
   long CountRows(const std::string& tableName, const std::string& whereClause="");

   // Actions on the Bills table
   void InsertBillRow(const RowBillTable& row, const std::string& tableName);
   RowBillTable SelectBillRow(const std::string& query);

private:
   otl_connect db;                                          // Database connection
   char buffer[1024];                                       // Selct result row into this buffer
};

#endif
