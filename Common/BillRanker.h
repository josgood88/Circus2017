#pragma once

#include <BillRow.h>
#include "CAPublic.h"
#include <boost/weak_ptr.hpp>
#include <string>
#include <vector>

namespace BillRanker {
   struct BillRanking {
      BillRanking() : configuration(std::string()), path(std::string()), word_score(0) {}
      BillRanking(const std::string& c, const std::string& p, unsigned int w) : configuration(c), path(p), word_score(w) {}
      BillRanking(const BillRanking& rhs) : configuration(rhs.configuration), path(rhs.path), word_score(rhs.word_score) {}
      bool operator< (const BillRanking &rhs) const { return word_score > rhs.word_score; }
      std::string configuration;
      std::string path;
      unsigned int word_score;
   };

   std::vector<BillRanking> GenerateBillRankings(std::vector<BillRow>& bills,CAPublic& db,const std::string positive, const std::string negative);
}
