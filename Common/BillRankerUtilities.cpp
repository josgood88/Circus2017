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
#include <iomanip>
#include <iostream>
#include <regex>
#include <string>
#include <sstream>
#include <time.h>
#include <vector>

typedef std::map<std::string, std::pair<std::regex, int>> RankWordMap;
typedef std::map<std::string, std::pair<std::regex, int>> RankPhraseMap;
typedef RankWordMap::iterator RankWordItr;
typedef RankWordMap::const_iterator RankWordCItr;
typedef std::vector<std::string> WordVector;
typedef std::vector<std::string>::const_iterator WordItr;

namespace {
   int score;                          // Accumulate the bill's score

   // Convert a string to lower case.
   std::string ToLowerCase(const std::string& source) {
      std::string result(source);
      std::transform(result.begin(), result.end(), result.begin(), ::tolower);
      return result;
   }

   // Fill a vector with the search terms (sorted) contained in rankingTerms.
   void AlphabetizedListOfRankingTerms(const RankWordMap rankingTerms, WordVector& rankingStrings) {
      rankingStrings.clear();
      rankingStrings.reserve(rankingTerms.size());
      std::for_each(rankingTerms.begin(), rankingTerms.end(), [&](const std::pair<std::string, std::pair<std::regex, int>>& item) {
         rankingStrings.push_back(item.first);
      });
      std::sort(rankingStrings.begin(),rankingStrings.end(), [&](const std::string& a, const std::string& b) -> bool { 
         const std::string a_lower_case(ToLowerCase(a));
         const std::string b_lower_case(ToLowerCase(b));
         return a_lower_case.compare(b_lower_case) < 0; 
      });
   }

   // Fill a vector with the words in a string.
   void TokenizeStringIntoVector(const std::string& source, WordVector& words) {
      const std::regex splitOn("[^\\w]+");
      std::sregex_token_iterator itr(source.begin(), source.end(), splitOn, -1);
      const std::sregex_token_iterator endOfSequence;
      words.clear();
      words.reserve(250000);                             // TODO: Pass this value
      while (itr != endOfSequence) {
         const auto token((*itr++).str());               // Need string, not sub_match
         if (token.length() > 0) words.push_back(token);
      }
   }

   // Convert vector contents to lower case and then sort the vector 
   // /arg words - the vector to convert.  On exit the vector is converted.
   void MakeLowercaseAndSortedInPlace(WordVector& words) {
      // Transform the strings in the vector
      std::transform(words.begin(), words.end(), words.begin(), [](std::string str) -> std::string { 
         return ToLowerCase(str);
      });
      std::sort(words.begin(),words.end());
   }

   // Search through the words vector for the first instance of a term in the ranking terms
   // /arg r_start - looking for a match for this term
   // /arg w_start - start searching the words vector at this point
   // /arg w_end   - stop  searching the words vector at this point
   std::vector<std::string>::const_iterator FindFirstMatch(
      const std::regex& want_match, std::vector<std::string>::const_iterator w_start,std::vector<std::string>::const_iterator w_end) 
   {
      std::smatch m;
      return std::find_if(w_start, w_end, 
         [&](const std::string& item) { return std::regex_search(item,m,want_match);
      });
   }

   // Count words vector matches to a term in the ranking terms
   // /arg r_start - looking for a match for this term
   // /arg w_start - follows first match in the words vector
   // /arg w_end   - stop  searching the words vector at this point
   unsigned int CountMatches(
      const std::regex& want_match, std::vector<std::string>::const_iterator w_start, std::vector<std::string>::const_iterator w_end) 
   {
      std::smatch m;
      // Count matches, stopping on the first term that doesn't match.
      const std::vector<std::string>::const_iterator w_first_match(w_start);
      const std::vector<std::string>::const_iterator w_first_non_match(std::find_if(w_start, w_end, 
         [&](const std::string& item) { return !std::regex_search(item,m,want_match);
      }));

      // Return count of matches (one already found before entering this function)
      return std::distance(w_first_match,w_first_non_match);
   }

   typedef std::pair<std::string, std::pair<std::regex, int>> RANKING_TERM_TYPE;

   void RankByPhraseSubr2(unsigned int& match_count, std::string& s2, std::smatch& m) {
         match_count++;
         s2 = m.suffix().str();
   }

   void RankByPhraseSubr(const std::string& source, const RANKING_TERM_TYPE& entry) {
      unsigned int match_count(0);
      std::string s2(source);
      std::smatch m;
      while (std::regex_search(s2,m,entry.second.first)) {
         RankByPhraseSubr2(match_count,s2,m);
      }
      if (match_count > 0) {
         const unsigned int worth(match_count * entry.second.second);
         score += worth;
         std::cout << "   " << match_count << " instances of " << entry.first 
            << " (" << entry.second.second << ")"
            << ", worth = " << worth 
            << ", score = " << score 
            << std::endl;
      }
   }

   void RankByPhrase(const std::string& source, const RankPhraseMap& phraseRankingTerms, bool showDetails) {
      std::cout << "Phrase count" << std::endl;
      if (phraseRankingTerms.size() > 0) {
         std::for_each(phraseRankingTerms.begin(), phraseRankingTerms.end(), [&](const RANKING_TERM_TYPE& entry) {
            RankByPhraseSubr(source,entry);
         });
      }
   }

   void RankByWord(const std::string& source, const RankWordMap& wordRankingTerms, bool showDetails) {
      std::cout << "Word count" << std::endl;
                  // 0) Sort ranking terms in alphabetical order
      WordVector rankingStrings;
      AlphabetizedListOfRankingTerms(wordRankingTerms,rankingStrings);

      // 1) Collect all words into a vector
      const boost::posix_time::ptime now1 = boost::posix_time::microsec_clock::universal_time();
      WordVector words;
      TokenizeStringIntoVector(source,words);

      // 2) Make vector contents lower case and then sort the vector
      //MakeLowercaseAndSortedInPlace(words);
      std::sort(words.begin(),words.end());

      // 3) Search through the words vector for each instance of a term in the ranking terms
      std::vector<std::string>::const_iterator current_words_starting_point(words.cbegin());
      // rankingStrings and lower_case_rankingStrings differ only in case
      std::for_each(rankingStrings.begin(), rankingStrings.end(), [&](const std::string& ranking_str) {
         if (current_words_starting_point != words.cend()) {
            const RankWordCItr looking_for(wordRankingTerms.find(ranking_str));
            if (looking_for != wordRankingTerms.end()) {
               const std::regex want_match(looking_for->second.first);
               std::vector<std::string>::const_iterator w_itr = FindFirstMatch(want_match, current_words_starting_point, words.cend());
               if (w_itr == words.cend()) {
                  //std::cout << "   No match for " << ranking_str << std::endl; 
               } else {
                  // 4) Increment score by number-of-matches time match-value
                  unsigned int match_count(CountMatches(want_match, w_itr, words.cend()));
                  const unsigned int worth(match_count * looking_for->second.second);
                  score += worth;
                  std::cout << "   " << match_count << " instances of " << ranking_str 
                     << " (" << looking_for->second.second << ")"
                     << ", worth = " << worth 
                     << ", score = " << score 
                     << std::endl;
                  current_words_starting_point = w_itr + match_count;
               }
            }
         }
      });
   }
}

// Define the argument passed to each worker thread's Run method
struct WorkerArgs {
   const std::string textToProcess; // Making this a reference nearly halves the speed (double the time) for AB383
   const std::regex re;
   const int value;
   WorkerArgs(const std::string& f, const std::regex& t, int v) : textToProcess(f), re(t), value(v) { }
};

// Conceptually, spawn one worker thread to process each member of rankingTerms and let them run in parallel.
// Practically, that gives poor performance, so limit the number of threads that run concurrently.
// By observation, processing each rankingTerms member serially is faster than processing all in parallel on the current hardware.
// Note that each rankingTerms member defines a WorkerArgs.
class Workers {
public:
   Workers(int numberOfThreads, const std::string& textToProcess, const RankWordMap rankingTerms, bool _showDetails)
       : showDetails(_showDetails)
    {
      // By test, 2 threads has been found to be faster than 3 threads, which is slower than 1 thread (!)
      // Recheck this when the hardware is next upgraded.
      //
      // Also tested extracting all textToProcess matches to terms in rankingTerms and then adding up the value for each match.
      // Extracting the matches is incredibly slow -- a matter of hours.  The approach below is demonstrably fastest.
      const unsigned int max_concurrent_threads(2);
      auto itr(rankingTerms.cbegin());             // std::map<std::string, std::pair<std::regex, int>>::const_iterator
      std::for_each(rankingTerms.cbegin(),rankingTerms.cend(), [&] (const std::pair<std::string, std::pair<std::regex, int>> term) { 
         const WorkerArgs w(textToProcess, term.second.first, term.second.second);
         m_threads.create_thread(boost::bind(&Workers::Run, this, w));
         if (m_threads.size() % max_concurrent_threads == 0) m_threads.join_all();
      });
   }
   ~Workers() {
      m_io.stop();
      m_threads.join_all();
   }
   void Run(const WorkerArgs w) {
      //showDetails = true;
      //if (showDetails) std::cout <<  w.re << " started." << std::endl;
      //const boost::posix_time::ptime now1 = boost::posix_time::microsec_clock::universal_time();
      const std::ptrdiff_t match_count(std::distance(
         std::sregex_iterator(w.textToProcess.cbegin(), w.textToProcess.cend(), w.re), 
         std::sregex_iterator()));
      if (match_count > 0) {
         //if (showDetails) {
         //   std::stringstream ss;
         //   const boost::posix_time::ptime now2 = boost::posix_time::microsec_clock::universal_time();
         //   const boost::posix_time::time_duration dur = now2 - now1;
         //   ss << " Count=" << match_count << ", value=" << w.value << ", Ranking time = " << dur << std::endl;
         //   std::cout << ss.str();
         //}
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
/// \brief wordRankingTerms -- define the value of each search term
/// \brief phraseRankingTerms -- define the value of each search phrase
//*****************************************************************************
//
int BillRankerUtilities::Rank(const std::string& fileName, const RankWordMap& wordRankingTerms, 
                                                           const RankPhraseMap& phraseRankingTerms, bool showDetails) {
   std::string source(ReadFile(fileName));         // Read the file in.
   // Remove standard bill prefix, including Legislative Counsel's digest
   const std::string exp("THE PEOPLE OF THE STATE OF CALIFORNIA DO ENACT AS FOLLOWS:\n");
   const size_t splitHere(source.find(exp));
   if (splitHere != source.npos) source = std::string(source.c_str()+splitHere+exp.length());
   // In particular, remove \r\n from the bill text.  Remove everything less than space.
   std::for_each(source.begin(),source.end(), [](char& c) { if (c < ' ') c = ' '; });
   return RankText(source,wordRankingTerms,phraseRankingTerms,showDetails);
}

int BillRankerUtilities::RankText(const std::string& source, 
                                  const RankWordMap& wordRankingTerms, const RankPhraseMap& phraseRankingTerms,
                                  bool showDetails) {
   score = 0;
   const boost::posix_time::ptime now1 = boost::posix_time::microsec_clock::universal_time();
   RankByWord  (source, wordRankingTerms,   showDetails);
   RankByPhrase(source, phraseRankingTerms, showDetails);
   const boost::posix_time::ptime now2 = boost::posix_time::microsec_clock::universal_time();
   const boost::posix_time::time_duration dur = now2 - now1;
   std::cout << "Ranking time = " << dur << std::endl;
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

   std::string SingleLine(std::string& source) {
      std::for_each(source.begin(),source.end(),[](char& c) { if (c <= ' ') c = ' '; }); // Multiline not supported by std::regex
      return source;
   }

   std::vector<std::string> ExtractRawTerms(const std::string& fileName, const std::regex& rx) {
      std::vector<std::string> result;
      std::string source(SingleLine(BillRankerUtilities::ReadFile(fileName)));   // Read the file into a string
      std::smatch matches;
      while (std::regex_search(source, matches, rx)) {
         const std::string member(matches[0]);                       // Current match
         result.push_back(member);
         source = matches.suffix().str();                            // Prepare for the next substring match
      }
      return result;
   }

   RankWordMap ExtractRankingTerms(const std::vector<std::string>& rawTerms, const std::regex& rx) {
      RankWordMap result;
      std::for_each(rawTerms.begin(), rawTerms.end(), [&](std::string source) {
         std::smatch matches;
         std::string keyMember, regexMember;                         // Save Key and Regex tuple members here
         while (std::regex_search(source, matches, rx)) {
            const std::string member(matches[0]);                    // Cycles through Key, Regex, and Score
            // This is a Key definition, save the string
            if (member.find("Key") != std::string::npos) { keyMember = ExtractTerm(member,"<Key>(.*?)</Key>");
            // This is a Regex definition, save the string
            } else if (member.find("Regex") != std::string::npos) { regexMember = ExtractTerm(member,"<Regex>(.*?)</Regex>");
            // This is a Score definition, have all three members, build the map entry
            } else {
               const std::string score = ExtractTerm(member,"<Score>(.*?)</Score>");
               std::stringstream ss2;
               ss2 << score;
               int intScore;
               ss2 >> intScore;
               const std::regex rx(regexMember, std::regex::icase);
               result[keyMember] = std::make_pair(rx,intScore);
            }
            // Prepare for the next substring match
            source = matches.suffix().str();
         }
      });
      return result;
   }
}

void BillRankerUtilities::ReadRankingTerms(const std::string& fileName, std::map<std::string, std::pair<std::regex, int>>& wordMap, 
                                                                        std::map<std::string, std::pair<std::regex, int>>& phraseMap) {
   const std::regex rx1("<Pair>.*?</Pair>");
   const std::regex rx2("<Phrase>.*?</Phrase>");
   const std::regex rx3("<Key>.*?</Key>|<Regex>.*?</Regex>|<Score>.*?</Score>");
   const std::vector<std::string> pairs1(ExtractRawTerms(fileName,rx1));
   const std::vector<std::string> pairs2(ExtractRawTerms(fileName,rx2));
   wordMap   = ExtractRankingTerms(pairs1,rx3);
   phraseMap = ExtractRankingTerms(pairs2,rx3);
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
   std::transform(input.begin(), input.end(), std::back_inserter(upper), ::toupper);
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
