#pragma once

#ifdef CAPublic_EXPORTS  
#define CAPublic_API __declspec(dllexport)   
#else  
#define CAPublic_API __declspec(dllimport)   
#endif 

//#include <BillRanker.h>
//#include <BillRow.h>
#include <BillRowTable.h>
#include "capublic_bill_tbl.h"
#include "capublic_bill_history_tbl.h"
#include "capublic_bill_version_authors_tbl.h"
#include "capublic_bill_version_tbl.h"
#include "capublic_location_code_tbl.h"
#include "DB_capublic.h"

#include <boost/shared_ptr.hpp>
#include <string>

//
//*****************************************************************************
/// \brief CAPublic encapsulates the database which receives data from the California Legislature website.
///        CAPublic owns the various classes representing tables in the database.
//*****************************************************************************
//

class CAPublic {
public:
   CAPublic_API CAPublic();
   CAPublic_API CAPublic(bool _import_leg_data);
   CAPublic_API ~CAPublic() {}
   CAPublic_API bool ExecuteSQL(const std::string& command);

   CAPublic_API std::string Author          (const std::string& id)    { return bill_version_authors_tbl->Author(id); }
   CAPublic_API std::string Title           (const std::string& lob)   { return bill_version_tbl->Title(lob);         }
   CAPublic_API std::string MeasureType     (const std::string& id)    { return bill_tbl->MeasureType(id);            }
   CAPublic_API std::string MeasureNum      (const std::string& id)    { return bill_tbl->MeasureNum(id);             }
   CAPublic_API std::string BillField       (const std::string& query) { return bill_row_tbl->FieldQuery(query);      }
   CAPublic_API std::string BillTableField  (const std::string& query) { return bill_tbl->FieldQuery(query);          }
   CAPublic_API std::string BillVersionField(const std::string& query) { return bill_version_tbl->FieldQuery(query);  }
   CAPublic_API std::string LocationField   (const std::string& query) { return location_code_tbl->FieldQuery(query); }
   CAPublic_API bool UpdateBillRow          (const BillRow& item)      { return bill_row_tbl->Update(item);           }

   CAPublic_API std::vector<BillRow>                 ReadBillRows()           { return bill_row_tbl->Read();     }
   CAPublic_API             BillRow                  ReadSingleBillRow(const std::string& measure_type, const std::string& measure_num) { return bill_row_tbl->ReadSingleBillRow(measure_type,measure_num); }
   CAPublic_API std::vector<BillRow>                 ReadVersionTable()       { return bill_version_tbl->Read(); }
   CAPublic_API std::vector<std::string> ReadVectorString(const std::string& query)  { return Readers::ReadVectorString(WP(),query); }
   CAPublic_API std::vector<std::vector<std::string>> ReadVectorVector(const std::string& query)  { return Readers::ReadVectorVector(WP(),query); }

   CAPublic_API std::vector<std::vector<std::string>> BillHistory(const std::string& bill_id) { return bill_history_tbl->BillHistory(bill_id); }
   CAPublic_API boost::weak_ptr<DB_capublic> WP() { return boost::weak_ptr<DB_capublic> (sp_capublic); }

private:
   void Initialize(const std::string& databaseName);
   boost::shared_ptr<DB_capublic>     sp_capublic;
   bool                               import_leg_data;
   capublic_bill_tbl*                 bill_tbl;
   capublic_bill_history_tbl*         bill_history_tbl;
   capublic_bill_version_tbl*         bill_version_tbl;
   capublic_bill_version_authors_tbl* bill_version_authors_tbl;
   BillRowTable*                      bill_row_tbl;
   capublic_location_code_tbl*        location_code_tbl;
};

