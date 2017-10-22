#pragma warning (disable:4996)      // 'std::_Copy_backward': Function call with parameters that may be unsafe
// OTL_STL is defined in the project property sheet, to enable std::string <==> varchar/char

#include "RowBillTable.h"
#include <Performer.h>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/filesystem/path.hpp>
#include <OTL/otlv4.h>

#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <algorithm> 
#include <functional> 

namespace {
   // http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring -- Thanks to the poster
   // trim from end
   static inline std::string &rtrim(std::string &s) {
      //std::replace_if(s.rbegin(), s.rend(), [&](int i) { return i > 0x7f; }, 
      //                                      [&](int i) -> char { return i & 0x7f; } );
      
//    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
      s.erase(std::find_if(s.rbegin(), s.rend(), [&](int i) { return i != ' '; } ).base(), s.end());
      return s;
   }

   static inline std::string trim(const std::string& s) {
      auto start(s.find_first_not_of(" "));
      if (start == std::string::npos) return std::string();
      auto end  (s.find_last_not_of (" "));
      std::string result(s.substr(start,end+1));
      return result;
   }
   //
   //*****************************************************************************
   /// \brief Report exceptions to std::cout
   //*****************************************************************************
   //
   void Report(const std::string& message) {
      std::cout << message << std::endl;
   }
}

//
//*****************************************************************************
/// \brief RowBillTable represents a single row in the Bills table.
//*****************************************************************************
//
RowBillTable::RowBillTable() 
   : primaryKey(0), 
     measure(""), fileName(""), localPath(""), house(""), topic(""), author(""), coAuthor(""), fileDate(""), score(0), score_law_change(0), latest(0), score_title(0)
   { }

RowBillTable::~RowBillTable() {
}

RowBillTable::RowBillTable(
   const std::string& measure_, const std::string& fileName_, const std::string& localPath_, const std::string& house_,
   const std::string& topic_,   const std::string& author_,   const std::string& coAuthor_,  const std::string& fileDate_, 
   const int score_,            const int score_law_change_,  const int score_title_,        const bool         latest_) 
   : primaryKey(0), 
     measure(measure_), fileName(fileName_), localPath(localPath_), score(score_), score_law_change(score_law_change_), score_title(score_title_), latest(latest_),
     house(house_), topic(topic_), author(author_), coAuthor(coAuthor_), fileDate(fileDate_)
   { }

RowBillTable::RowBillTable(const RowBillTable& rhs)
   : primaryKey(rhs.primaryKey), 
     measure(rhs.measure), fileName(rhs.fileName), localPath(rhs.localPath), score(rhs.score), score_law_change(rhs.score_law_change), latest(rhs.latest), 
     house(rhs.house), topic(rhs.topic), author(rhs.author), coAuthor(rhs.coAuthor), fileDate(rhs.fileDate), score_title(rhs.score_title)
   { }

RowBillTable::RowBillTable(otl_stream& is) {
   is >> primaryKey >> measure >> fileName >> localPath >> house >> topic >> author >> coAuthor >> fileDate >> score >> score_law_change >> latest >> score_title;
   measure   = rtrim(measure);
   fileName  = rtrim(fileName);
   localPath = rtrim(localPath);
   house     = rtrim(house);
   topic     = trim(topic);
   author    = rtrim(author);
   coAuthor  = rtrim(coAuthor);
   fileDate  = rtrim(fileDate);
}
//
//*****************************************************************************
/// \brief Construct RowBillTable from a bill.
//*****************************************************************************
//
namespace {
   void ReportException(const std::string& announce, const otl_exception& ex) {
      Report(announce);
      Report(reinterpret_cast<const char*>(ex.msg));        // Report error message
      Report(ex.stm_text);                                  // Report SQL that caused the error
      Report(ex.var_info);                                  // Report the variable that caused the error
   }

   std::string BillContents(const std::string& filePath) {
      std::ifstream is(filePath);
      std::stringstream contents;
      contents << is.rdbuf();
      return contents.str();
   }

   std::string Extract(const std::string& source, const std::string& regexStr, const std::string captureNum) {
      const std::regex rx(regexStr);
      const std::string fmt(captureNum);
      const std::string str(std::regex_replace(source,rx,fmt));
      // Return empty string if unable to find the field
      return str == source ? "" : str;
   }

   std::string FetchTopic(const std::string& contents, Performer& performer) {
      // Return the second sentence in the Legislative Counsel's digest.
      const std::string s1(Extract(contents,".*LEGISLATIVE COUNSEL'S DIGEST.*?\\.(.*?\\.).*","$1"));
      const std::string s2(std::regex_replace(s1, std::regex("^\\s+"),    std::string()));
      const std::string s3(std::regex_replace(s2, std::regex("\\s+$"),    std::string()));
      const std::string s4(std::regex_replace(s3, std::regex("\\s+"),     std::string(" ")));
      const std::string s5(std::regex_replace(s4, std::regex("\\s?-\\s?"),std::string("-")));
      return s5;
   }

   std::string CoAuthorCleanup(std::string& str, const std::string& house) {
      // Isolate the list of coauthors
      std::string s1;
      if (house == "Assembly") {
         if (str.find("oauthor:") == std::string::npos) return std::string(); // No coauthors if none mentioned
         s1 = std::regex_replace(str,std::regex(".*?Coauthor.*?Members?\\s*(.*?)"),std::string("$1"));
      } else if (house == "Senate") {
         if (str.find("Senators") == std::string::npos) return std::string(); // No coauthors if none mentioned
         s1 = std::regex_replace(str,std::regex(".*?Senators\\s+\\w+\\s+and\\s+(.*?)"),std::string("$1"));
      }
      // Trim multiple consecutive spaces
      std::string s2 = std::regex_replace(s1, std::regex("(\\s+)"),std::string(" "));
      // For multiple coauthors, last one is ", and SomeOne"
      std::string s3 = std::regex_replace(s2, std::regex("(, and)"),std::string(","));
      // For multiple coauthors, last character is closing parenthesis
      std::replace(s3.begin(),s3.end(),')',' ');
      return rtrim(s3);
   }
}

RowBillTable::RowBillTable(const std::string& filePath, Performer& performer) {
   std::string contents(BillContents(filePath));                           // Read the bill file
   std::replace(contents.begin(),contents.end(),'\n',' ');                 // std::regex doesn't have multiline switch
   std::replace(contents.begin(),contents.end(),'\t',' ');                 // Tabs aren't any help, either
   
   // Remove HTML
   const std::string s1(std::regex_replace(contents,std::regex("<em>(.*?)</em>"),  std::string(" $1 ")));
   const std::string s2(std::regex_replace(s1,std::regex("<strike>(.*?)</strike>"),std::string()));
   const std::string normalized(s2);

   // Fetch and Normalize the header
   const std::string header(Extract(normalized,"<head>(.*?)</head>.*","$1"));    // Isolate the bill's header text

   // These items come from the bill header
   measure  = rtrim(Extract(header,".*?MEASURE\\\"\\s+(content|CONTENT)?=\\\"(.*?)\\\".*",  "$2"));
   house    = rtrim(Extract(header,".*?HOUSE\\\"\\s+(content|CONTENT)?=\\\"(.*?)\\\".*",    "$2"));
   author   = rtrim(Extract(header,".*?AUTHOR\\\"\\s+(content|CONTENT)?=\\\"(.*?)\\\".*",   "$2"));
   fileDate = rtrim(Extract(header,".*?FILEDATE\\\"\\s+(content|CONTENT)?=\\\"(.*?)\\\".*", "$2"));
   // Enclose author in quotes if embedded comma...Committee on Business, Professions and Economic Development.
   if (author.find(",") != std::string::npos) {
      author = std::string("\"") + author + "\"";
      performer.LogThis(std::string("Quoted comma: " + author));

   }

   // Topic is taken from the Legislative Counsel's Digest
   topic = FetchTopic(normalized,performer);

   // Latest and Score will be filled in by the BillRanker
   latest = 0;
   score = 0;
   score_law_change = 0;
   score_title = 0;

   // Co-authors take special processing
   std::string temp = rtrim(Extract(header,".*?COAUTHOR\\\"\\s+(content|CONTENT)?=\\\"(.*?)\\\".*", "$2"));
   coAuthor = CoAuthorCleanup(temp,house);

   // These items come from the full path to the bill text file
   const boost::filesystem::path path(filePath);
   fileName  = path.filename().string();
   localPath = path.parent_path().string();
}
//
//*****************************************************************************
/// \brief Write self to table
/// \param[in] Connection to the relevant database
/// \param[in] Insert into this table
//*****************************************************************************
//
void RowBillTable::WriteYourself(otl_connect& db, const std::string& tableName) {
   std::string colValues("(:1<int>,:2<char[10]>,:3<char[70]>,:4<char[50]>,:5<char[10]>,:6<char[400]>,:7<char[100]>,:8<char[1300]>,:9<char[10]>,:10<int>, :11<int>, :12<int>, :13<int>)");
   std::stringstream ss;
   ss << "Insert Into " << tableName << " Values" << colValues;
   otl_stream os(1, ss.str().c_str(), db);                  // Inserting a single row, specify 1 row
   primaryKey = CountRows(db,tableName) + 1;
   try {
      os << primaryKey << measure  << fileName << localPath << house << topic << author 
         << coAuthor   << fileDate << score    << score_law_change   << latest << score_title;
   } catch (const otl_exception& ex) {
      ReportException("OTL exception in RowBillTable::WriteYourself", ex);
   }
}
//
//*****************************************************************************
/// \brief Read self from table
/// \param[in] Connection to the relevant database
/// \param[in] Query that returns the row defining new RowBillTable contents
//*****************************************************************************
//
void RowBillTable::ReadYourself(otl_connect& db, const std::string& query) {
   otl_stream is(10, query.c_str(), db);               // Define the SQL command that this otl_stream uses
   try {
      is >> primaryKey >> measure >> fileName >> localPath >> house >> topic >> author 
         >> coAuthor >> fileDate >> score >> score_law_change >> latest >>  score_title;
   } catch (const otl_exception& ex) {
      ReportException("OTL exception in RowBillTable::ReadYourself", ex);
   }
   measure   = rtrim(measure);
   fileName  = rtrim(fileName);
   localPath = rtrim(localPath);
   house     = rtrim(house);
   topic     = rtrim(topic);
   author    = rtrim(author);
   coAuthor  = rtrim(coAuthor);
   fileDate  = rtrim(fileDate);
}
//
//*****************************************************************************
/// \brief Count number of table rows that match a query
/// \param[in] db -- Connection to the relevant database
/// \param[in] tableName -- Count rows in this table
/// \param[in] where -- Count rows satisfying this Where clause.  Clause must have leading space.
//*****************************************************************************
//
long RowBillTable::CountRows(otl_connect& db, const std::string& tableName, const std::string& whereClause) {
   std::stringstream ss;
   ss << "Select Count(*) from " << tableName << whereClause;
   long result(-1);
   try {
      otl_stream is(1, ss.str().c_str(), db);                  // Count has a single result, 1 row required
      is >> result;
   } catch (const otl_exception& ex) {
      ReportException("OTL exception in RowBillTable::CountRows", ex);
   }
   return result;
}
//
//*****************************************************************************
/// \brief Update the bill ranking (score) for a bill
/// \param[in] database  - The relevant database
/// \param[in] tableName - Update goes in this table
/// \param[in] term      - Use this file name to select the row
/// \param[in] score     - New score for the row
/// \param[in] score     - New score for the changes to current law
//*****************************************************************************
//
void RowBillTable::UpdateScore(DB& database, const std::string& table, const std::string& term, 
                               const int bill_text_score, const int score_law_change, const int score_title) {
   // Build the pattern for the update command.  OTL will fill in the :f2 and :f1 data.
   std::stringstream ss;
   ss << "Update " << table << " Set Score = :f2<int>, Score_Extracted = :f3<int>, Score_Title = :f4<int> Where FileName = :f1<char[70]";

   // Do the update.  OTL fills in the Update command and then performs it.
   try {
      otl_stream o(1, ss.str().c_str(),database.db);        // buffer size should be == 1 always on Update
      o << bill_text_score << score_law_change << score_title << term;     // Update the row, inputs in order of appearance in the update command
   } catch (const otl_exception& ex) {
      ReportException("OTL exception in RowBillTable::UpdateScore", ex);
   }
}
//
//*****************************************************************************
/// \brief Update the "Latest" setting for a bill
/// \param[in] database  - The relevant database
/// \param[in] tableName - Update goes in this table
/// \param[in] term      - Use this measure to select the row
/// \param[in] score     - New "Latest" setting
//*****************************************************************************
//
void RowBillTable::UpdateLatest(DB& database, const std::string& table, int id, int latest) {
   // Build the pattern for the update command.  OTL will fill in the :f2 and :f1 data.
   std::stringstream ss;
   ss << "Update " << table << " Set Latest = :f2<int> Where ID = :f1<int>";

   // Do the update.  OTL fills in the Update command and then performs it.
   try {
      otl_stream o(1, ss.str().c_str(),database.db);  // buffer size should be == 1 always on Update
      o << latest << id;                              // Update the row, inputs in order of appearance in the update command
   } catch (const otl_exception& ex) {
      ReportException("OTL exception in RowBillTable::UpdateLatest", ex);
   }
}
//
//*****************************************************************************
/// \brief Answer a rowset of RowBillTable 
/// \param[in] database - The relevant database
/// \param[in] query    - Selects the rowset
//*****************************************************************************
//
std::vector<RowBillTable> RowBillTable::RowSet(DB& database, const std::string& query) {
   std::vector<RowBillTable> result;
   otl_stream is(10, query.c_str(), database.db);        // Stream contains rowset entries
   while (!is.eof()) {
      RowBillTable entry(is);
      result.push_back(entry);
   }
   return result;
}
//
//*****************************************************************************
/// \brief Answer a std::string column entry satisfying a query
/// \param[in] database - The relevant database
/// \param[in] query    - Selects the column entry
//*****************************************************************************
//
std::string RowBillTable::AnswerString(DB& database, const std::string& query) {
   otl_stream is(1, query.c_str(), database.db);        // Stream contains results
   std::string result;
   try {
      is >> result;
   } catch (const otl_exception& ex) {
      ReportException("OTL exception in RowBillTable::AnswerString", ex);
   }
   return result;
}
//
//*****************************************************************************
/// \brief Equivalence operator
//*****************************************************************************
//
bool RowBillTable::operator== (const RowBillTable& rhs) const {
   return (primaryKey       == rhs.primaryKey)       &&     // Comparing integers can shortcut comparing strings
          (score            == rhs.score)            &&
          (score_law_change == rhs.score_law_change) &&
          (score_title      == rhs.score_title)      &&
          (latest           == rhs.latest)           &&
          (measure          == rhs.measure)          &&
          (fileName         == rhs.fileName)         &&
          (localPath        == rhs.localPath)        &&
          (house            == rhs.house)            &&
          (topic            == rhs.topic)            &&
          (author           == rhs.author)           &&
          (coAuthor         == rhs.coAuthor)         &&
          (fileDate         == rhs.fileDate);
}
//
//*****************************************************************************
/// \brief Export to std::string
//*****************************************************************************
//
std::string RowBillTable::ToString() {
   std::stringstream ss;
   ss <<   "ID = "              << primaryKey
      << ", Score = "           << score
      << ", Score Law Change= " << score_law_change
      << ", Score Title= "      << score_title
      << ", Latest = "          << latest
      << ", Measure = "         << rtrim(measure)
      << ", FileName = "        << rtrim(fileName)
      << ", LocalPath = "       << rtrim(localPath)
      << ", House = "           << rtrim(house)
      << ", Topic = "           << rtrim(topic)
      << ", Author = "          << rtrim(author)
      << ", CoAuthor = "        << rtrim(coAuthor)
      << ", FileDate = "        << rtrim(fileDate);
   return ss.str();
}
