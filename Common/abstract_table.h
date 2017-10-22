#ifndef abstract_table_h
#define abstract_table_h

#include <boost/weak_ptr.hpp>
#include "db_capublic.h"
#include <Logger.h>
#include <string>

#define Q  DB_capublic::Quote
#define QC DB_capublic::QuoteC

class abstract_table {
public:

protected:
   bool ConcatentateMultipleInsertsBeforeExecuting(const std::string& input_sql,boost::weak_ptr<DB_capublic>& db_public,bool flush_accumulator) {
      bool result(true);
      boost::shared_ptr<DB_capublic> wp = db_public.lock();
      if (wp) {
         if (flush_accumulator || (sql_accumulator.length()+input_sql.length() > SQLITE_MAX_LENGTH)) {
            if (!wp->ExecuteSQL(sql_accumulator)) {
               LoggerNS::Logger::Log(std::string("Failed SQL \n") + sql_accumulator);
               result = false;
            }
            sql_accumulator = std::string();
         }
         if (sql_accumulator.length() == 0) sql_accumulator += input_sql;
         else {
            sql_accumulator[sql_accumulator.length()-1] = ',';                      // Replace terminating semi-colon with clause-separating comma
            const std::string values("Values ");
            const size_t values_start(input_sql.find(values));                      // Find Values clause in string-to-append
            if (values_start != std::string::npos) {
               sql_accumulator += input_sql.substr(values_start+values.length());   // Append the new clause
            } else {
               LoggerNS::Logger::Log(std::string("SQL missing Values clause\n") + input_sql);
               result = false;
            }
         }
      }
      return result;
   }

private:
   std::string sql_accumulator;                       // Accumulate SQL statements here, submit as single statement
   const size_t SQLITE_MAX_LENGTH = 80*1024;          // Sqlite.c comment says 1000000000
};

#endif
