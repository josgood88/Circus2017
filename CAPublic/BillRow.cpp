
#include <BillRow.h>
#include "capublic.h"

#include <boost/shared_ptr.hpp>

//bool operator== (const BillRow &lhs,const BillRow &rhs) { 
//   return (lhs.change          == rhs.change)          &&
//          (lhs.neg_score       == rhs.neg_score)       &&
//          (lhs.pos_score       == rhs.pos_score)       &&
//          (lhs.position        == rhs.position)        &&
//          (lhs.lob             == rhs.lob)             &&
//          (lhs.bill_id         == rhs.bill_id)         &&
//          (lhs.bill            == rhs.bill)            &&
//          (lhs.bill_version_id == rhs.bill_version_id) &&
//          (lhs.author          == rhs.author)          &&
//          (lhs.title           == rhs.title);          }

bool operator!= (const BillRow &lhs,const BillRow &rhs) { return !(lhs == rhs); }
