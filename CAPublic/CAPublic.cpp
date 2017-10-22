#include <BillRowTable.h>
#include "CAPublic.h"
#include "capublic_bill_history_tbl.h"
#include "capublic_bill_version_authors_tbl.h"
#include "capublic_bill_version_tbl.h"
#include "capublic_location_code_tbl.h"
#include "db_capublic.h"
#include "Logger.h"
#include "ScopedElapsedTime.h"

#include <boost/weak_ptr.hpp>
#include <sstream>

namespace {
   const std::string database_location("../Data/capublic.db");
}

CAPublic::CAPublic()                      : import_leg_data(true)             { Initialize(database_location); }
CAPublic::CAPublic(bool _import_leg_data) : import_leg_data(_import_leg_data) { Initialize(database_location); }

void CAPublic::Initialize(const std::string& databaseName) {
   ScopedElapsedTime elapsed_time("Initializing database","Database initialization run time: ");
   sp_capublic = boost::shared_ptr<DB_capublic>(new DB_capublic(databaseName));
   boost::weak_ptr<DB_capublic> wp(sp_capublic);
   bill_tbl                 = new capublic_bill_tbl                (wp,import_leg_data);
   bill_history_tbl         = new capublic_bill_history_tbl        (wp,import_leg_data);
   bill_version_tbl         = new capublic_bill_version_tbl        (wp,import_leg_data);
   bill_version_authors_tbl = new capublic_bill_version_authors_tbl(wp,import_leg_data);
   bill_row_tbl             = new BillRowTable                     (wp,import_leg_data);
   location_code_tbl        = new capublic_location_code_tbl       (wp,import_leg_data);
}

bool CAPublic::ExecuteSQL(const std::string& command) {
   return sp_capublic->ExecuteSQL(command);

}
