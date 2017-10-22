#pragma once

#include <CommonTypes.h>

#include <regex>
#include <sstream>

struct BillRow {
   change_t       change;
   score_t        neg_score;
   score_t        pos_score;
   measure_type_t measure_type;
   measure_num_t  measure_num;
   position_t     position;
   bill_lob_t     lob;              // path to the lob file
   bill_id_t      bill_id;          // e.g., 201720180AB1
   std::string    bill;             // e.g., AB1
   bill_ver_id_t  bill_version_id;  // e.g., 20170SCR1499INT.  LATEST bill version id from bill_tbl
   bill_author_t  author;
   bill_title_t   title;

   BillRow() : neg_score(0), pos_score(0) {}
   // Create from bill_version_tbl data
   BillRow(const bill_id_t& a, const bill_ver_id_t& b, const bill_lob_t& c) : bill_id(a), bill_version_id(b), lob(c), neg_score(0), pos_score(0) {
      const std::string house_and_number = bill_id.substr(9);           // Isolate house and bill number
      const std::regex rx_extract_house("\\b([A-Z]+).*");
      measure_type = std::regex_replace(house_and_number,rx_extract_house,std::string("$1"));
      measure_num = house_and_number.substr(measure_type.length());
   }
   // Create from bill_version_tbl data
   BillRow(const bill_id_t& a, const bill_ver_id_t& b, const bill_lob_t& c, const measure_type_t& d, const measure_num_t& e) 
     : bill_id(a), bill_version_id(b), lob(c), measure_type(d), measure_num(e), neg_score(0), pos_score(0) {}

   BillRow(const change_t&       _change,         const score_t&        _neg_score,   const score_t&      _pos_score, 
           const measure_type_t& _measure_type,   const measure_num_t&  _measure_num, const position_t&   _position, 
           const bill_lob_t&     _lob,            const bill_id_t& _bill_id,          const std::string&  _bill,     
           const bill_ver_id_t& _bill_version_id, const bill_author_t& _author,       const bill_title_t& _title) 
           : 
           change(_change),                       neg_score(_neg_score),              pos_score(_pos_score),  
           measure_type(_measure_type),           measure_num(_measure_num),          position(_position), 
           lob(_lob),                             bill_id(_bill_id),                  bill(_bill),
           bill_version_id(_bill_version_id),     author(_author),                    title(_title) {}

   bool MatchBillID(const bill_id_t& rhs) { return bill_id == rhs; }

   bool operator< (const BillRow &rhs) const {
      // Sort by house of origin
      if (measure_type != rhs.measure_type) return (measure_type < rhs.measure_type);
      // Subsort by measure number
      else if (measure_num != rhs.measure_num) {
         std::stringstream left_measure_num_stringstream,right_measure_num_stringstream;
         unsigned int left_measure_num_as_int,right_measure_num_as_int;
         left_measure_num_stringstream << measure_num;         right_measure_num_stringstream << rhs.measure_num;
         left_measure_num_stringstream >> left_measure_num_as_int; right_measure_num_stringstream >> right_measure_num_as_int;
         return left_measure_num_as_int < right_measure_num_as_int;
      // Subsort by version number
      } else {
         const std::string prefix("BILL_VERSION_TBL_");
         const size_t p_len(prefix.length());
         const size_t lhs_per =     lob.find('.', p_len);
         const size_t rhs_per = rhs.lob.find('.', p_len);
         const std::string lhs_lob_s =     lob.substr(p_len,lhs_per-p_len);
         const std::string rhs_lob_s = rhs.lob.substr(p_len,lhs_per-p_len);
         std::stringstream left_ss,right_ss;
         int left_int, right_int;
         left_ss  << lhs_lob_s;  left_ss  >> left_int;
         right_ss << rhs_lob_s;  right_ss >> right_int;
         return left_int < right_int;
      }
   }

   friend bool operator== (const BillRow& lhs,       BillRow& rhs) { return lhs == const_cast<const BillRow&>(rhs); }
   friend bool operator== (const BillRow& lhs, const BillRow& rhs) { 
      bool b =
             (lhs.change          == rhs.change)          &&
             (lhs.neg_score       == rhs.neg_score)       &&
             (lhs.pos_score       == rhs.pos_score)       &&
             (lhs.position        == rhs.position)        &&
             (lhs.lob             == rhs.lob)             &&
             (lhs.bill_id         == rhs.bill_id)         &&
             (lhs.bill            == rhs.bill)            &&
             (lhs.bill_version_id == rhs.bill_version_id) &&
             (lhs.author          == rhs.author)          &&
             (lhs.title           == rhs.title);          
      return b;
   }   
   friend bool operator!= (const BillRow& lhs, const BillRow& rhs) { return !(lhs == rhs); }

   void UpdateYourself(const BillRow& rhs) {
      std::stringstream ss;
      if (rhs.measure_type.length()    > 0 && rhs.measure_type.length() <= 3) measure_type    = rhs.measure_type;
      if (rhs.measure_num.length()     > 0 && rhs.measure_num.length()  <= 4) measure_num     = rhs.measure_num;
      if (rhs.neg_score               >= 0 && rhs.neg_score         <= 10000) neg_score       = rhs.neg_score;
      if (rhs.pos_score               >= 0 && rhs.pos_score         <= 10000) pos_score       = rhs.pos_score;
      if (rhs.position.length()        > 0                                  ) position        = rhs.position;
      if (rhs.lob.length()             > 0                                  ) lob             = rhs.lob;
      if (rhs.bill.length()            > 0                                  ) bill            = rhs.bill;
      if (rhs.bill_version_id.length() > 0                                  ) bill_version_id = rhs.bill_version_id;
      if (rhs.author.length()          > 0                                  ) author          = rhs.author;
      if (rhs.title.length()           > 0                                  ) title           = rhs.title;
      ss  >> change;
   }
};


