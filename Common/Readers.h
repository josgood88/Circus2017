#pragma once

#include <BillRow.h>
#include "db_capublic.h"

#include <boost/weak_ptr.hpp>
#include <string>

namespace Readers {
   std::string                           ReadSingleString       (boost::weak_ptr<DB_capublic>& db_public, const std::string& query);
   std::vector<std::string>              ReadVectorString       (boost::weak_ptr<DB_capublic>& db_public, const std::string& query);
   std::vector<std::vector<std::string>> ReadVectorVector       (boost::weak_ptr<DB_capublic>& db_public, const std::string& query);
   std::vector<BillRow>                  ReadVectorBillRow      (boost::weak_ptr<DB_capublic>& db_public);
   std::vector<BillRow>                  ReadVectorBillRow      (boost::weak_ptr<DB_capublic>& db_public, const std::string& query);
   std::vector<BillRow>                  ReadVectorVersionIdTbl (boost::weak_ptr<DB_capublic>& db_public);
   std::vector<BillRow>                  ReadVectorVersionIdTbl (boost::weak_ptr<DB_capublic>& db_public, const std::vector<std::string>&);
   std::vector<BillRow>                  ReadVectorLegBillTable (boost::weak_ptr<DB_capublic>& db_public);
   std::vector<std::vector<std::string>> BillHistory            (boost::weak_ptr<DB_capublic>& db_public, const std::string& bill_id);
}
