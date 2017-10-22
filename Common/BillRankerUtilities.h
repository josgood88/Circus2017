#ifndef BillRankerUtilities_h
#define BillRankerUtilities_h

#include "Database/DB.h"
#include <boost/optional.hpp>

#include <map>
#include <regex>
#include <string>

namespace BillRankerUtilities {
   int         Rank    (const std::string& fileName, const std::map<std::string, std::pair<std::regex, int>>&,
                                                     const std::map<std::string, std::pair<std::regex, int>>&, bool showDetails);
   int         RankText(const std::string& source,   const std::map<std::string, std::pair<std::regex, int>>&, 
                                                     const std::map<std::string, std::pair<std::regex, int>>&, bool showDetails);
   std::string ReadFile(const std::string& fileName);
   void        EvaluateRowLatestSetAndClear(DB& db, const std::string& table, const std::string& measure, 
                                            boost::optional<unsigned int>& clearThis, boost::optional<unsigned int>& setThis);
   std::string CurrentLegSession();
   std::string ExtractBillName(const std::string& input);
   std::string FilenameForBill(DB& database, const std::string& bill);
   std::string RangeText(const std::string& bill);
   std::string TextFolder(const std::string& bill);
   void ReadRankingTerms(const std::string& fileName, std::map<std::string, std::pair<std::regex, int>>& wordMap, 
                                                      std::map<std::string, std::pair<std::regex, int>>& phraseMap);
}

#endif
