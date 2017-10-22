#include "LocalFileLocation.h"
#include "Database/DB.h"
#include "LegInfo.h"
#include <Performer.h>
#include "RowBillTable.h"

#include "boost/date_time/posix_time/posix_time.hpp"
#include <iomanip>
#include <regex>
#include <sstream>
#include <string>
//
//*****************************************************************************
/// \brief Extract bill name from a string.  String is probably something like "AB383", but make sure.
/// \brief [in]  input -- the "AB 383" or whatever
//*****************************************************************************
//
std::string LocalFileLocation::ExtractBillName(const std::string& input) {
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
namespace {
   std::string RangeText(const std::string& bill) {
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
}
//
//*****************************************************************************
/// \brief CurrentLegSession returns a string specifying the current legislative session.
///        The session is based on the current date.
///        The string is "yy-yy", e.g. "11-12" for the 2011-2012 session.
//*****************************************************************************
//
namespace {
   std::string CurrentLegSession() {
      boost::gregorian::date today = boost::gregorian::day_clock::local_day();
      if (today.is_not_a_date()) { } // throw a TBD exception
      return LegInfo_Utility::CurrentLegSession(today.year());
   }
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
std::string LocalFileLocation::TextFolder(const std::string& bill) {
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
std::string LocalFileLocation::FilenameForBill(DB& database, const std::string& bill) {
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
