#ifndef DB_h
#define DB_h

// Any project including DB.cpp must define OTL_STL in the project property sheet, to enable std::string <==> varchar/char
#include <OTL/otlv4.h>     // http://otl.sourceforge.net/otl3.htm

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <string>

class DB : public boost::noncopyable {
public:
   DB();
   DB(const std::string& databaseName);
   DB(const std::string& connectString, const std::string& databaseName);
   ~DB();

   // Actions on any table
   bool ExecuteSQL(const std::string& command);
   long NonQuery(const std::string& nonQuery);
   long CountRows(const std::string& tableName, const std::string& whereClause="");

private:
   otl_connect db;                                          // Database connection
   char buffer[1024];                                       // Select result row into this buffer
};

#endif
