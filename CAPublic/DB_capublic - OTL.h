#ifndef DB_capublic_h
#define DB_capublic_h

// Any project including DB.cpp must define OTL_STL in the project property sheet, to enable std::string <==> varchar/char
#include <OTL/otlv4.h>     // http://otl.sourceforge.net/otl3.htm

#include <string>

//
//*****************************************************************************
/// \brief DB provides database encapsulation.  
///        This version is used for the database which receives data from the California Legislature website.
//*****************************************************************************
//

class DB_capublic {
public:
   DB_capublic();
   DB_capublic(const std::string& databaseName);
   DB_capublic(const std::string& connectString, const std::string& databaseName);
   ~DB_capublic();

   void Report(const std::string& message);
   void ReportException(const std::string& reporter);
   otl_connect db;                                 // Database connection

   // Actions on any table
   bool ExecuteSQL(const std::string& command);
   long NonQuery(const std::string& nonQuery);
   long CountRows(const std::string& tableName, const std::string& whereClause="");

private:
   void Initialize(const std::string& connectString, const std::string& databaseName);
   //char buffer[1024];                              // Select result row into this buffer
};

#endif
