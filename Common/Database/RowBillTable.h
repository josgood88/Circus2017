#ifndef RowBillTable_h
#define RowBillTable_h

#include "DB.h"
#include <Performer.h>

// Without this, TestBillRankerMain::EvaluateRowLatestSetAndClear gets "Winsock has already been included".  Thank goodness there is an internet!
#define WIN32_LEAN_AND_MEAN

// OTL_STL is defined in the project property sheet, to enable std::string <==> varchar/char
#include <OTL/otlv4.h>     // http://otl.sourceforge.net/otl3.htm

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/noncopyable.hpp>
#include <boost/serialization/string.hpp>
#include <string>
#include <sstream>

class DB;   // DB and RowBillTable include each other.  This avoids C2061.

class RowBillTable {
public:
   RowBillTable();
   RowBillTable(const std::string& measure_, const std::string& fileName_, const std::string& localPath_, const std::string& house_,
                const std::string& topic_,   const std::string& author_,   const std::string& coAuthor_,  const std::string& fileDate_,
                const int          score_m,  const int          extract_m, const int          extract_t,  const bool         latest_);
   RowBillTable(const RowBillTable&);
   RowBillTable(const std::string&, Performer&);   // The string is expected to be the contents of a bill file
   RowBillTable(otl_stream& is);
   ~RowBillTable();

   // Use OTL to operate on a table
   void WriteYourself(otl_connect& db, const std::string& tableName);
   void ReadYourself (otl_connect& db, const std::string& query);

   bool operator==(const RowBillTable& rhs) const;
   bool operator!=(const RowBillTable& rhs) const { return !(*this == rhs); }
   std::string ToString();

   std::string Measure  () const { return measure;          }
   std::string FileName () const { return fileName;         }
   std::string LocalPath() const { return localPath;        }
   std::string House    () const { return house;            }
   std::string Topic    () const { return topic;            }
   std::string Author   () const { return author;           }
   std::string CoAuthor () const { return coAuthor;         }
   std::string FileDate () const { return fileDate;         }
   int         ID       () const { return primaryKey;       }
   int         IsLatest () const { return latest;           }
   int         Score    () const { return score;            }
   int   ScoreLawChange () const { return score_law_change; }
   int   ScoreTitle     () const { return score_title;      }

   // Static methods
   static std::string AnswerString(DB& database, const std::string& query);
   static long CountRows(otl_connect& db, const std::string& tableName, const std::string& whereClause="");
   static std::vector<RowBillTable> RowSet(DB& database, const std::string& query);
   static void UpdateLatest(DB& database, const std::string& table, int id,                  int latest);
   static void UpdateScore (DB& database, const std::string& table, const std::string& term, const int bill_text_score, const int extracted_score, const int title_score);

private:
   int         primaryKey;
   std::string measure;             // Bill number, e.g. AB 1234
   std::string fileName;            // File name (less path), e.g. "ab_1234_bill_20110331_amended_asm_v98.html"
   std::string localPath;           // Path to local file, e.g. "D:\CCHR\pub\11-12\bill\asm\ab_1201-1250"
   std::string house;               // Asm or Sen
   std::string topic;               // Bill title, e.g. "Redevelopment agencies: financing.	"
   std::string author;              // Bill author, e.g. "Norby"
   std::string coAuthor;            // Bill co-authors, e.g. "Norby|Someone else"
   std::string fileDate;            // Date of this version, e.g. "20110331"
   int         score;               // Computed ranking, based on full text
   int         score_law_change;    // Computed ranking, changes to current law
   int         latest;              // Whether this is the latest version of the bill
   int         score_title;         // Computed ranking, title

public:
   // http://www.boost.org/doc/libs/1_52_0/libs/serialization/doc/serialization.html
   // Public because the line below doesn't build
   // friend class boost::serialization::access;
   template <typename Archive> void serialize(Archive& ar, const unsigned int version) {
      // Boost serialization defines the & operator as << for output archives and as >> input archives.
      ar & primaryKey; 
      ar & measure;
      ar & fileName;
      ar & localPath;
      ar & house;
      ar & topic;
      ar & author;
      ar & coAuthor;
      ar & fileDate;
      ar & score;
      ar & score_law_change;
      ar & latest;
      ar & score_title;
   }
};

#endif
