#include "db_capublic.h"
#include "BillRanker.h"
#include <BillRow.h>
#include <BillRowTable.h>
#include <Readers.h>

#include <boost/shared_ptr.hpp>
#include <sstream>
#include <vector>

#define  Q DB_capublic::Quote
#define QC DB_capublic::QuoteC
#define IC DB_capublic::IComma

BillRowTable::BillRowTable(boost::weak_ptr<DB_capublic> database, bool import_leg_data) : db_public(database) { }

// Insert a single BillRow into the database
bool BillRowTable::Insert(const BillRow& row) {
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      std::stringstream ss;
      ss << "Insert into BillRows ("
            "Change, NegativeScore, PositiveScore, MeasureType, MeasureNum, Position, Lob, BillId, Bill, BillVersionId, Author, Title"
            ") Values ("
         << QC(row.change)   <<  IC(row.neg_score) << IC(row.pos_score)  << QC(row.measure_type) << QC(row.measure_num)
         << QC(row.position) <<  QC(row.lob)       << QC(row.bill_id)    << QC(row.bill)         <<  QC(row.bill_version_id) 
         << QC(row.author)   <<  Q (row.title)    
         << ");";
      if (!wp->ExecuteSQL(ss.str())) {
         wp->Report(std::string("Failed SQL \n") + ss.str());
      }
   }
   return true;
}

// Update a single BillRow
// If the bill hasn't been processed by an earlier run, then insert a new row.
// Otherwise update the positive and negative scores, along with the Lob file name.
// The Lob file name is critical because that is what distinguishes one update from another.
bool BillRowTable::Update(const BillRow& row) {
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      std::stringstream ss1,ss2;
      ss1 << " Where MeasureType = '" << row.measure_type << "' And MeasureNum = '" << row.measure_num << "';";
      if (wp->Count("BillRows",ss1.str()) == 0) {
         Insert(row);
      } else {
         ss2 << "Update BillRows Set PositiveScore = '" << row.pos_score << "', NegativeScore = '" << row.neg_score << "', "
             << "Lob = '" << row.lob << "', BillVersionId = '" << row.bill_version_id << "'"
             << " Where MeasureType = '" << row.measure_type << "' And MeasureNum = '" << row.measure_num << "';";
         if (!wp->ExecuteSQL(ss2.str())) {
            wp->Report(std::string("Failed SQL \n") + ss2.str());
         }
      }
   }
   return true;
}

bool BillRowTable::Replace(const std::vector<BillRow>& table) {
   const std::string cmd_delete("Delete From BillRows;");
   const std::string cmd_vacuum("VACUUM;");
   boost::shared_ptr<DB_capublic> wp = db_public.lock();
   if (wp) {
      wp->Report("Writing BillRow table");
      if (wp->ExecuteSQL(cmd_delete)) {
         if (wp->ExecuteSQL(cmd_vacuum)) {
            std::for_each(table.begin(), table.end(), [&](const BillRow& row) {
               Insert(row);
            });
         } else {
            wp->Report(std::string("Failed SQL \n") + cmd_delete);
         }
      } else {
         wp->Report(std::string("Failed SQL \n") + cmd_vacuum);
      }
      wp->Report("Through writing BillRow table");
   }
return true;
}

// Update this table with the latest ranking data.  There can be the following types of updates:
//    1) An updated bill will have a new lob file.  It may also have a new score.
//    2) New bills appear and need to be added to the table.
//    3) An updated bill may have new authors or title.
// This method isn't concerned with updating our position.  New additions have a position of None, an existing bill's position isn't updated here.
//bool BillRowTable::UpdateLegStatus(const std::vector<BillRanker::BillRanking>& rankings) { 
//   boost::shared_ptr<DB_capublic> wp = db_public.lock();
//   if (wp) {
//      std::vector<BillRow>           bill_row_table = Read();
//      std::vector<BillRanker::BillRanking> rankings = Readers::ReadVectorBillRanking(db_public);
//      std::for_each(rankings.begin(), rankings.end(), [&](const BillRanker::BillRanking& r) {
//      });
//   }
//   return true; 
//}

std::vector<BillRow> BillRowTable::Read() { return Readers::ReadVectorBillRow(db_public); }

BillRow BillRowTable::ReadSingleBillRow(const std::string& measure_type, const std::string& measure_num) {
   std::stringstream ss;
   ss << "Select Change,MeasureType,MeasureNum,NegativeScore,PositiveScore,Position,Lob,BillId,Bill,BillVersionId,Author,Title From BillRows Where "
      << "MeasureType = '" << measure_type << "' And MeasureNum = '" << measure_num << "';";
   std::vector<BillRow> row(Readers::ReadVectorBillRow(db_public,ss.str()));
   return *(row.begin());
}


std::string BillRowTable::FieldQuery(const std::string& query) {
   return Readers::ReadSingleString(db_public,query);
}
