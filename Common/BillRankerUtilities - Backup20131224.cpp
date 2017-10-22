#include "BillRankerUtilities.h"
#include "Database/DB.h"
#include "LegInfo.h"
#include "Performer.h"
//
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/foreach.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/thread.hpp>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <string>
#include <sstream>
#include <time.h>

namespace {
   int score;                          // Accumulate the bill's score
}

// Define the argument passed to each worker thread's Run method
struct WorkerArgs {
   const std::string textToProcess; // Making this a reference nearly halves the speed (double the time) for AB383
   const std::string term;
   const int value;
   WorkerArgs(const std::string& f, const std::string& t, int v) : textToProcess(f), term(t), value(v) { }
};

// Conceptually, spawn one worker thread to process each member of rankingTerms and let them run in parallel.
// Practically, that gives poor performance, so limit the number of threads that run concurrently.
// By observation, processing each rankingTerms member serially is faster than processing all in parallel on the current hardware.
// Note that each rankingTerms member defines a WorkerArgs.
class Workers {
public:
   Workers(int numberOfThreads, const std::string& textToProcess, const std::map<std::string, std::pair<std::regex, int>> rankingTerms, bool _showDetails)
       : showDetails(_showDetails)
    {
      // By test, 2 threads has been found to be faster than 3 threads, which is slower than 1 thread (!)
      // Recheck this when the hardware is next upgraded.
      const unsigned int max_concurrent_threads(2);
      auto itr(rankingTerms.cbegin());             // std::map<std::string, std::pair<std::regex, int>>::const_iterator
      std::for_each(rankingTerms.cbegin(),rankingTerms.cend(), [&] (const std::pair<std::string, std::pair<std::regex, int>> term) { 
         const WorkerArgs w(textToProcess, term.first, term.second.second);
         m_threads.create_thread(boost::bind(&Workers::Run, this, w));
         if (m_threads.size() % max_concurrent_threads == 0) m_threads.join_all();
      });
   }
   ~Workers() {
      m_io.stop();
      m_threads.join_all();
   }
   void Run(const WorkerArgs w) {
      if (showDetails) std::cout <<  w.term << " started." << std::endl;
      const boost::posix_time::ptime now1 = boost::posix_time::microsec_clock::universal_time();
      const std::regex re(w.term);
      const std::ptrdiff_t match_count(std::distance(
         std::sregex_iterator(w.textToProcess.cbegin(), w.textToProcess.cend(), re), 
         std::sregex_iterator()));
      if (match_count > 0) {
         if (showDetails) {
            std::stringstream ss;
            const boost::posix_time::ptime now2 = boost::posix_time::microsec_clock::universal_time();
            const boost::posix_time::time_duration dur = now2 - now1;
            ss << w.term << " : " << " Count=" << match_count << ", value=" << w.value << ", Ranking time = " << dur << std::endl;
            std::cout << ss.str();
         }
         boost::unique_lock<boost::mutex> lock(mut_score);
         score += match_count * w.value;
      }
   }
   boost::asio::io_service m_io;
private:
   boost::thread_group m_threads;
   bool showDetails;
   boost::mutex mut_score;             // Single-threaded access to the bill's score
};

//
//*****************************************************************************
/// \brief Rank a single file.
/// \brief fileName -- rank this file
//*****************************************************************************
//
int BillRankerUtilities::Rank(const std::string& fileName, const std::map<std::string, std::pair<std::regex, int>>& rankingTerms, bool showDetails) {
   std::string source(ReadFile(fileName));         // Read the file in.
   // Remove standard bill prefix, including Legislative Counsel's digest
   const std::string exp("THE PEOPLE OF THE STATE OF CALIFORNIA DO ENACT AS FOLLOWS:\n");
   const size_t splitHere(source.find(exp));
   if (splitHere != source.npos) source = std::string(source.c_str()+splitHere+exp.length());
   return RankText(source,rankingTerms,showDetails);
}

int BillRankerUtilities::RankText(const std::string& source, const std::map<std::string, std::pair<std::regex, int>>& rankingTerms, bool showDetails) {
   int abc = 0;
   score = 0;
   std::map<std::string, std::pair<int,int>> wordScores;

   if (abc == 1) {
      const std::regex splitOn("[^\\w]+");
      const std::sregex_token_iterator endOfSequence;
      std::sregex_token_iterator itr(source.begin(), source.end(), splitOn, -1);
      while (itr != endOfSequence) {
         auto token = (*itr++).str();              // Need string, not sub_match
         if (token.length() > 0) {
            auto match = std::find_if(rankingTerms.begin(), rankingTerms.end(), 
               [&] (const std::pair<std::string, std::pair<std::regex, int>>& item) -> bool {
                  if (token.length() < item.first.length()) return false;
                  auto potential_match = token.substr(0,item.first.length());
                  return item.first.compare(potential_match) == 0;
            });
            if (match != rankingTerms.end()) {
if (token.find("ehav") != token.npos) std::cout << token << std::endl;
               auto pair = wordScores[match->first];
               pair.first++;
               pair.second += match->second.second;   // Count number of matches
               wordScores[match->first] = pair;       // Accumulate score
               //wordScores[match->first].second.first++;                       // Count number of matches
               //wordScores[match->first].second += match->second.second;// Accumulate score
            }
         }
      }

      std::for_each(wordScores.begin(), wordScores.end(), [](const std::pair<std::string, std::pair<int,int>>& arg) {
         score += arg.second.second;
         std::cout << arg.first << ", Count=" << arg.second.first << ", value=" << arg.second.second << std::endl;
      });

   } else {
      {
         Workers workers(rankingTerms.size(), source, rankingTerms, showDetails);
      }
   }

   return score;
}
//
//*****************************************************************************
/// \brief Read a single file.
/// \brief fileName -- read this file
//*****************************************************************************
//
std::string BillRankerUtilities::ReadFile(const std::string& fileName) {
   std::ifstream is(fileName);
   std::stringstream ss;
   ss << is.rdbuf();
   return ss.str();
}
//
//*****************************************************************************
/// \brief Read the ranking terms from the XML file that defines them.
///        This is very simple code that does no error checking and is not production quality.
/// \brief fileName -- path to file containing ranking terms
//*****************************************************************************
//
namespace {
   std::string ExtractTerm(const std::string& matchElement, const std::string& regexString) {
      const std::regex rx(regexString);
      const std::string fmt("$1");
      return std::regex_replace(matchElement,rx,fmt);
   }
}

std::map<std::string, std::pair<std::regex, int>> BillRankerUtilities::ReadRankingTerms(const std::string& fileName) {
   std::map<std::string, std::pair<std::regex, int>> result;

   // Read the file into a string
   std::string source(BillRankerUtilities::ReadFile(fileName));

   // Extract the Regex and Score terms -- they come in pairs.
   std::smatch matches;
   const std::regex rx("<Regex>.*?</Regex>|<Score>.*?</Score>");
   std::string regexMember;                                    // Save Regex half of the pair here
   while (std::regex_search(source, matches, rx)) {
      const std::string member(matches[0]);                    // Here is the current match
      // Current match is Regex, save the string
      if (member.find("Regex") != std::string::npos) {
         regexMember = ExtractTerm(member,"<Regex>(.*?)</Regex>");
      } else {
         // Current match is Score, build the map entry
         const std::string score = ExtractTerm(member,"<Score>(.*?)</Score>");
         std::stringstream ss2;
         ss2 << score;
         int intScore;
         ss2 >> intScore;
         const std::regex rx(regexMember, std::regex::icase);
         result[regexMember] = std::make_pair(rx,intScore);
      }
      // Prepare for the next substring match
      source = matches.suffix().str();
   }
   return result;
}
//
//*****************************************************************************
/// \brief Evaluate which table row needs Latest set and which needs it cleared
/// \brief [in]  db        -- Database, connection has been established
/// \brief [in]  fn        -- Filename (e.g. ab_10) of the bill
/// \brief [out] clearThis -- Clear Latest on row with this ID
/// \brief [out] setThis   -- Set Latest on row with this ID
/// \brief clearThis and setThis specify a row iff set to a positive value
//*****************************************************************************
//
void BillRankerUtilities::EvaluateRowLatestSetAndClear(DB& database, const std::string& table, const std::string& fn, boost::optional<unsigned int>& clearThis, boost::optional<unsigned int>& setThis) {
   clearThis = setThis = boost::none;
   
   std::string measure(boost::to_upper_copy(fn));              // Database measures are in upper case
   const size_t pos = measure.find('_');                       // Bills have underscore separator
   if (pos != std::string::npos) measure.replace(pos,1," ");   // ..change to a space

   std::stringstream whereClause;
   whereClause << " Where Measure = '" << measure << "' And "
	            << "(FileName Like '%introduced%' Or FileName Like '%amended%' Or "
               << "FileName  Like '%enrolled%'   Or FileName Like '%chaptered%' ) ";
   const long matchSize = database.CountRows(table,whereClause.str());
   if (matchSize <= 0) return;                           // No matches, so nothing to do

   std::stringstream query;
   query << "Select * from " << table << " " << whereClause.str();
   std::vector<RowBillTable> rowset = RowBillTable::RowSet(database,query.str());

   unsigned int latestDate(0);
   BOOST_FOREACH (RowBillTable row, rowset) {
      if (row.IsLatest()) clearThis = row.ID();          // Can't have duplicate primary keys, no need to check
      unsigned int newDate(0);
      std::stringstream convert;
      convert << row.FileDate();
      convert >> newDate;
      if (newDate > latestDate) { latestDate = newDate; setThis = row.ID(); }
   }
   if (clearThis == setThis) clearThis = setThis = boost::none;    // If latest is already correctly indicated, do nothing  
}
//
//*****************************************************************************
/// \brief Extract bill name from a string.  String is probably something like "AB383", but make sure.
/// \brief [in]  input -- the "AB 383" or whatever
//*****************************************************************************
//
std::string BillRankerUtilities::ExtractBillName(const std::string& input) {
   // For bills from an extraordinary session, expect the user to enter the bill correctly, e.g. ABX1_2
   std::string upper;
   std::transform(input.begin(), input.end(), std::back_inserter(upper), std::toupper);
   std::string sub(upper.substr(0,3));
   if (sub == "SBX" || sub == "ABX") return input;
   
   // Remove spaces
   std::string noSpaces = std::regex_replace(input, std::regex("(\\s+)"),std::string());
   // Remove underscores
   std::string clean = std::regex_replace(noSpaces, std::regex("(_+)"),std::string());

   const std::regex rx(".*?(\\D+)(\\d+)");
   const std::string fmt("$1_$2");
   std::string result(std::regex_replace(clean,rx,fmt).c_str());
   return result;
}
//
//*****************************************************************************
/// \brief Determine range part, e.g. 0351-0400, for a bill name.
/// \brief [in]  input -- the "AB_383" or whatever
//*****************************************************************************
//
std::string BillRankerUtilities::RangeText(const std::string& bill) {
   const std::regex rx(".*?(\\d+)");
   const std::string fmt("$1");
   const std::string numericPart(std::regex_replace(bill,rx,fmt).c_str());
   std::stringstream numberStr;   // Can't be const, >> operator is used
   int numeric;
   numberStr << numericPart; numberStr >> numeric;
   const int lowerBound((numeric-1)/50 * 50 + 1);
   const int upperBound(lowerBound + 50-1);
   std::stringstream result;
   result << std::setw(4) << std::setfill('0') << lowerBound << "-" 
          << std::setw(4) << std::setfill('0') << upperBound;
   return result.str();
}
//
//*****************************************************************************
/// \brief CurrentLegSession returns a string specifying the current legislative session.
///        The session is based on the current date.
///        The string is "yy-yy", e.g. "11-12" for the 2011-2012 session.
//*****************************************************************************
//
std::string BillRankerUtilities::CurrentLegSession() {
   boost::gregorian::date today = boost::gregorian::day_clock::local_day();
   if (today.is_not_a_date()) { } // throw a TBD exception
   return LegInfo_Utility::CurrentLegSession(today.year());
}
//
//*****************************************************************************
/// \brief TextFolder answers the folder containing the text version of a bill.
///        It creates the file, if necessary.
///        For example, for an input argument of AB_383 \n
///        this method answers \n
///        "D:\CCHR\pub\13-14\bill\asm\ab_0351-0400\text"
/// \param[in] bill   AB_383 or similar
//*****************************************************************************
//
std::string BillRankerUtilities::TextFolder(const std::string& bill) {
   std::string result;
   const boost::filesystem::path base(LegInfo_Utility::BaseLocalFolder());
   const std::string currentSession(CurrentLegSession());
   const bool isAsm(std::tolower(bill[0]) == 'a');
   const bool isSen(std::tolower(bill[0]) == 's');
   if (isAsm || isSen) {
      const std::string house(isAsm ? "asm" : "sen");
      const std::string prefix(isAsm ? "ab_" : "sb_");
      const std::string stem(std::string(prefix + RangeText(bill)));
      boost::filesystem::path p(base / "pub" / currentSession / "bill" / house / stem);
      result = p.string();
   }
   return Performer::ConvertBackslash(result);
}
//
//*****************************************************************************
/// \brief Get the filename for the latest version of a bill.
/// \param[in] bill   AB_383 or similar
//*****************************************************************************
//
std::string BillRankerUtilities::FilenameForBill(DB& database, const std::string& bill) {
   std::string billnameWithSpace(bill);
   if (bill.find('_') != std::string::npos) {      // if bill name contains underscore
      const std::regex rx(".*?(\\w+)_(\\d+)");
      const std::string fmt("$1 $2");
      billnameWithSpace = std::regex_replace(bill,rx,fmt);
   }

   std::stringstream query;
   query << "Select FileName  from " << Performer::TableName() << " Where Latest = 1 And Measure = '" << billnameWithSpace << "'";
   std::string result = RowBillTable::AnswerString(database, query.str());
   return result;
}
