#include "BillRanker.h"
#include "BillRow.h"
#include "Logger.h"
#include "ScopedElapsedTime.h"
#include "Utility.h"
#include <boost/weak_ptr.hpp>

namespace {
   typedef std::map<std::string,std::pair<std::regex,int>> RankWordMap;
   typedef std::map<std::string,std::pair<std::regex,int>> RankPhraseMap;
   typedef RankWordMap::iterator RankWordItr;
   typedef RankWordMap::const_iterator RankWordCItr;
   typedef std::vector<std::string> WordVector;
   typedef std::vector<std::string>::const_iterator WordItr;
   RankWordMap neg_wordMap,pos_wordMap;
   RankWordMap neg_phraseMap,pos_phraseMap;

   // Convert a string to lower case.
   std::string ToLowerCase(const std::string& source) {
      std::string result(source);
      std::transform(result.begin(),result.end(),result.begin(),::tolower);
      return result;
   }

   std::string ExtractTerm(const std::string& matchElement,const std::string& regexString) {
      const std::regex rx(regexString);
      const std::string fmt("$1");
      return std::regex_replace(matchElement,rx,fmt);
   }

   std::string SingleLine(std::string& source) {
      std::for_each(source.begin(),source.end(),[](char& c) { if (c <= ' ') c = ' '; }); // Multiline not supported by std::regex
      return source;
   }
   std::vector<std::string> ExtractRawTerms(const std::string& fileName,const std::regex& rx) {
      std::vector<std::string> result;
      std::string source(SingleLine(ReadFile(fileName)));            // Read the file into a string
      std::smatch matches;
      while (std::regex_search(source,matches,rx)) {
         const std::string member(matches[0]);                       // Current match
         result.push_back(member);
         source = matches.suffix().str();                            // Prepare for the next substring match
      }
      return result;
   }

   RankWordMap ExtractRankingTerms(const std::vector<std::string>& rawTerms,const std::regex& rx) {
      RankWordMap result;
      std::for_each(rawTerms.begin(),rawTerms.end(),[&](std::string source) {
         std::smatch matches;
         std::string keyMember,regexMember;                         // Save Key and Regex tuple members here
         while (std::regex_search(source,matches,rx)) {
            const std::string member(matches[0]);                    // Cycles through Key, Regex, and Score
            // This is a Key definition, save the string
            if (member.find("Key") != std::string::npos) {
               keyMember = ExtractTerm(member,"<Key>(.*?)</Key>");
               // This is a Regex definition, save the string
            } else if (member.find("Regex") != std::string::npos) {
               regexMember = ExtractTerm(member,"<Regex>(.*?)</Regex>");
               // This is a Score definition, have all three members, build the map entry
            } else {
               const std::string score = ExtractTerm(member,"<Score>(.*?)</Score>");
               std::stringstream ss2;
               ss2 << score;
               int intScore;
               ss2 >> intScore;
               const std::regex rx(regexMember,std::regex::icase);
               result[keyMember] = std::make_pair(rx,intScore);
            }
            // Prepare for the next substring match
            source = matches.suffix().str();
         }
      });
      return result;
   }

   // Fill a vector with the search terms (sorted) contained in rankingTerms.
   void AlphabetizedListOfRankingTerms(const RankWordMap rankingTerms,WordVector& rankingStrings) {
      rankingStrings.clear();
      rankingStrings.reserve(rankingTerms.size());
      std::for_each(rankingTerms.begin(),rankingTerms.end(),[&](const std::pair<std::string,std::pair<std::regex,int>>& item) {
         rankingStrings.push_back(item.first);
      });
      std::sort(rankingStrings.begin(),rankingStrings.end(),[&](const std::string& a,const std::string& b) -> bool {
         const std::string a_lower_case(ToLowerCase(a));
         const std::string b_lower_case(ToLowerCase(b));
         return a_lower_case.compare(b_lower_case) < 0;
      });
   }

   // Fill a vector with the words in a string.
   void TokenizeStringIntoVector(const std::string& source,WordVector& words) {
      const std::regex splitOn("[^\\w]+");
      std::sregex_token_iterator itr(source.begin(),source.end(),splitOn,-1);
      const std::sregex_token_iterator endOfSequence;
      words.clear();
      words.reserve(250000);                             // TODO: Pass this value
      while (itr != endOfSequence) {
         const auto token((*itr++).str());               // Need string, not sub_match
         if (token.length() > 0) words.push_back(token);
      }
   }

   // Count words vector matches to a term in the ranking terms
   // /arg r_start - looking for a match for this term
   // /arg w_start - follows first match in the words vector
   // /arg w_end   - stop  searching the words vector at this point
   unsigned int CountMatches(
      const std::regex& want_match,std::vector<std::string>::const_iterator w_start,std::vector<std::string>::const_iterator w_end) {
      std::smatch m;
      // Count matches, stopping on the first term that doesn't match.
      const std::vector<std::string>::const_iterator w_first_match(w_start);
      const std::vector<std::string>::const_iterator w_first_non_match(std::find_if(w_start,w_end,
         [&](const std::string& item) { return !std::regex_search(item,m,want_match);
      }));

      // Return count of matches (one already found before entering this function)
      return std::distance(w_first_match,w_first_non_match);
   }

   // Search through the words vector for the first instance of a term in the ranking terms
   // /arg r_start - looking for a match for this term
   // /arg w_start - start searching the words vector at this point
   // /arg w_end   - stop  searching the words vector at this point
   std::vector<std::string>::const_iterator FindFirstMatch(
      const std::regex& want_match,std::vector<std::string>::const_iterator w_start,std::vector<std::string>::const_iterator w_end) {
      std::smatch m;
      return std::find_if(w_start,w_end,
         [&](const std::string& item) { return std::regex_search(item,m,want_match);
      });
   }

   // Remove HTML from a string containing the text of a bill
   void RemoveHTML(std::string& contents) {
      // Remove standard bill prefix, including Legislative Counsel's digest
      const std::string exp("The people of the State of California do enact as follows:");
      const size_t splitHere(contents.find(exp));
      if (splitHere != contents.npos) contents = std::string(contents.c_str()+splitHere+exp.length());

      // Remove HTML
      std::string s1(std::regex_replace(contents,std::regex("</?caml(.*?)>"),std::string()));
      std::string s2(std::regex_replace(s1,std::regex("<em>(.*?)</em>"),std::string(" $1 ")));
      std::string s3(std::regex_replace(s2,std::regex("<strike>(.*?)</strike>"),std::string()));
      std::string s4(std::regex_replace(s3,std::regex("<p>(.*?)</p>"),std::string(" $1 ")));
      std::string s5(std::regex_replace(s4,std::regex("<span class=.EnSpace./>"),std::string()));        // . instead of ""
      contents = s5;

      // Remove everything less than space.
      std::for_each(contents.begin(),contents.end(),[](char& c) { if (c < ' ') c = ' '; });
   }

   // Rank a single bill by words.  The input word vector has been sorted.
   unsigned int RankByWord(WordVector& words,const RankWordMap& wordRankingTerms,bool showDetails) {
      unsigned int score(0);                          // Accumulate the bill's score
      // Sort ranking terms in alphabetical order
      WordVector rankingStrings;
      AlphabetizedListOfRankingTerms(wordRankingTerms,rankingStrings);

      // Search through the words vector for each instance of a term in the ranking terms
      std::vector<std::string>::const_iterator current_words_starting_point(words.cbegin());
      // rankingStrings and lower_case_rankingStrings differ only in case
      std::for_each(rankingStrings.begin(),rankingStrings.end(),[&](const std::string& ranking_str) {
         if (current_words_starting_point != words.cend()) {
            const RankWordCItr looking_for(wordRankingTerms.find(ranking_str));
            if (looking_for != wordRankingTerms.end()) {
               const std::regex want_match(looking_for->second.first);
               std::vector<std::string>::const_iterator w_itr = FindFirstMatch(want_match,current_words_starting_point,words.cend());
               if (w_itr == words.cend()) {
                  //std::cout << "   No match for " << ranking_str << std::endl; 
               } else {
                  // Increment score by number-of-matches time match-value
                  unsigned int match_count(CountMatches(want_match,w_itr,words.cend()));
                  const unsigned int worth(match_count * looking_for->second.second);
                  score += worth;
                  if (showDetails) {
                     std::cout << "   " << match_count << " instances of " << ranking_str
                        << " (" << looking_for->second.second << ")"
                        << ", worth = " << worth
                        << ", score = " << score
                        << std::endl;
                  }
                  current_words_starting_point = w_itr + match_count;
               }
            }
         }
      });
      return score;
   }
}

namespace BillRanker {

   // Read positive and negative ranking terms from the configuration files
   void ReadRankingTerms(const std::string& neg_fileName,const std::string& pos_fileName) {
      const std::regex rx1("<Pair>.*?</Pair>");
      const std::regex rx2("<Phrase>.*?</Phrase>");
      const std::regex rx3("<Key>.*?</Key>|<Regex>.*?</Regex>|<Score>.*?</Score>");
      const std::vector<std::string> neg_pairs1(ExtractRawTerms(neg_fileName,rx1));
      const std::vector<std::string> neg_pairs2(ExtractRawTerms(neg_fileName,rx2));
      const std::vector<std::string> pos_pairs1(ExtractRawTerms(pos_fileName,rx1));
      const std::vector<std::string> pos_pairs2(ExtractRawTerms(pos_fileName,rx2));
      neg_wordMap   = ExtractRankingTerms(neg_pairs1,rx3);
      neg_phraseMap = ExtractRankingTerms(neg_pairs2,rx3);
      pos_wordMap   = ExtractRankingTerms(pos_pairs1,rx3);
      pos_phraseMap = ExtractRankingTerms(pos_pairs2,rx3);
   }

   // Generate bill rankings or read cached bill rankings
   // Report each bill's rankings to the log file.
   std::vector<BillRanking> GenerateBillRankings(std::vector<BillRow>& bills,CAPublic& db,const std::string positive,const std::string negative) {
      ScopedElapsedTime elapsed_time("Starting Rankings","Ranking Run Time: ");
      BillRanker::ReadRankingTerms(negative,positive);
      std::vector<BillRanker::BillRanking> rankings;
      // Generate fresh ranking for each bill in bills
      std::for_each(bills.begin(),bills.end(),[&](BillRow entry) {
         const std::string raw_lob_files_folder("D:/CCHR/2017-2018/LatestDownload/Bills");
         const std::string lob_path = (fs::path(raw_lob_files_folder) / entry.lob).string();
         if (lob_path.length() > 0) {
            std::string contents(ReadFile(lob_path));
            RemoveHTML(contents);
            // Collect all words into a vector, then sort them
            WordVector words;
            TokenizeStringIntoVector(contents,words);
            std::sort(words.begin(),words.end());                    // Profiler shows the sort is not expensive -- less than 2%
            // Generate positive and negative scores
            entry.pos_score = RankByWord(words,pos_wordMap,false);
            entry.neg_score = RankByWord(words,neg_wordMap,false);
            if (entry.bill.length() == 0) entry.bill = entry.bill_version_id;
            std::stringstream ss1,ss2;
            ss1 << "Update BillRows Set NegativeScore=" << entry.neg_score << ", PositiveScore=" << entry.pos_score << ", BillVersionId='" << entry.bill
                << "' Where MeasureType='" << entry.measure_type << "' and MeasureNum='" << entry.measure_num << "';";
            if (db.ExecuteSQL(ss1.str())) {
               ss2 << entry.measure_type << " " << entry.measure_num << " Negative = " << entry.neg_score << ", Positive = " << entry.pos_score;
               LoggerNS::Logger::Log(ss2.str());
            } else {
               ss2 << "Unable to execute SQL" << ss1.str();
               LoggerNS::Logger::Log(ss2.str());
            }
         }
      });
      return rankings;
   }
}