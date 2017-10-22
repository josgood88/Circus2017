#include <CAPublic.h>
#include "CreateBillReport.h"
#include <Logger.h>
#include "Utility.h"

#include <boost/filesystem/path.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "boost/date_time/gregorian/gregorian.hpp"
#include <ctime>
#include <ctype.h>
#include <regex>
#include <sstream>
#include <string>

namespace fs = boost::filesystem;

namespace {
   // Extract house and number from bill id, returning house and number through argument references
   bool ExtractHouseNumber(const std::string& bill, std::string& house, std::string& number) {
      std::regex rx("\\b([^\\d]+)(\\d+)");
      house  = std::regex_replace(bill,rx,std::string("$1"));
      number = std::regex_replace(bill,rx,std::string("$2"));
      bool correct = (house.compare(number) != 0) && isalpha(house[0]) && isdigit(number[0]);
      return correct;
   }

   // Extract bill Id, knowing house and number
   std::string GetBillID(CAPublic& db, const std::string& house, const std::string& number) {
      std::stringstream ss;
      ss << "Select BillId from BillRows Where MeasureType = '" << house << "' AND MeasureNum = " << number << ";";
      return db.BillField(ss.str());
   }

   // Extract bill Id, knowing house and number
   std::string GetBillVerID(CAPublic& db, const std::string& house, const std::string& number) {
      std::stringstream ss;
      ss << "Select BillVersionId from BillRows Where MeasureType = '" << house << "' AND MeasureNum = " << number << ";";
      return db.BillField(ss.str());
   }

   // Extract bill title, knowing house and number
   std::string GetBillTitle(CAPublic& db, const std::string& house, const std::string& number) {
      std::stringstream ss;
      ss << "Select Title from BillRows Where MeasureType = '" << house << "' AND MeasureNum = " << number << ";";
      return db.BillField(ss.str());
   }

   // Bill Current Location
   std::string GetBillLocation(CAPublic& db,const std::string& house,const std::string& number) {
      // TODO: This won't discover CS or CX locations until Main is updated
      //       Ensure bill_tbl is updated also, or else extend BillVersions to include location.  BR needs location data.
      std::string result;
      std::stringstream ss;
      ss << "Select current_location from bill_tbl Where measure_type = '" << house << "' AND measure_num = " << number << ";";
      result = db.BillTableField(ss.str());
      // Location isn't filled out -- use house and secondary location
      if ((result.length() == 0) || (result.compare("NULL")) == 0) {
         std::stringstream ss1;
         ss1 << "Select current_house from bill_tbl Where measure_type = '" << house << "' AND measure_num = " << number << ";";
         const std::string current_house = db.BillTableField(ss1.str());
         std::stringstream ss2;
         ss2 << "Select current_secondary_loc from bill_tbl Where measure_type = '" << house << "' AND measure_num = " << number << ";";
         const std::string secondary = db.BillTableField(ss2.str());
         result = ((current_house.length() == 0 || current_house == "NULL") ? "" : current_house) + " " + 
                  ((secondary    .length() == 0 || secondary     == "NULL") ? "" : secondary);
         // Location has a location code.  Translate it.
      } else if (result.length() >= 2) {
         std::string first_two = result.substr(0,2);
         if (first_two == std::string("CS") || first_two == std::string("CX")) {
            std::stringstream ss1;
            ss1 << "Select description from location_code_tbl Where location_code='" << result << "';";
            result = db.LocationField(ss1.str());
         }
      } else {
         std::stringstream ss;
         ss << "Unable to determine location for " << house << " " << number;
         LoggerNS::Logger::Log(ss.str());
      }
      if ((result.length() == 0) || (result.compare("NULL")) == 0) {
         std::stringstream ss;
         ss << "Location for " << house << " " << number << " is blank";
         LoggerNS::Logger::Log(ss.str());
      }
      return result;
   }

   // Bill Vote Required
   std::string GetBillVoteRequired(CAPublic& db, const std::string& bill_version_id) {
      std::stringstream ss;
      ss << "Select vote_required from bill_version_tbl Where bill_version_id = '" << bill_version_id << "';";
      return db.BillVersionField(ss.str());
   }

   // Appropriation
   std::string GetBillAppropriation(CAPublic& db, const std::string& bill_version_id) {
      std::stringstream ss;
      ss << "Select appropriation from bill_version_tbl Where bill_version_id = '" << bill_version_id<< "';";
      return db.BillVersionField(ss.str());
   }

   // Fiscal Committee
   std::string GetBillFiscal(CAPublic& db, const std::string& bill_version_id) {
      std::stringstream ss;
      ss << "Select fiscal_committee from bill_version_tbl Where bill_version_id = '" << bill_version_id<< "';";
      return db.BillVersionField(ss.str());
   }

   // Local Program
   std::string GetBillLocalProgram(CAPublic& db, const std::string& bill_version_id) {
      std::stringstream ss;
      ss << "Select local_program from bill_version_tbl Where bill_version_id = '" << bill_version_id<< "';";
      return db.BillVersionField(ss.str());
   }

   // Bill History
   std::vector<std::string> BillHistory(CAPublic& db, const std::string& bill_id) {
      // Fetch raw, unsorted bill history
      std::vector<std::vector<std::string>> two_fields = db.BillHistory(bill_id);
      // Sort in normal date order.  Vector filled with push_back, which will give reverse date order in the report.
      std::sort(two_fields.begin(),two_fields.end(), [&](const std::vector<std::string> lhs, const std::vector<std::string> rhs) -> bool {
         const auto greg_lhs = boost::gregorian::from_string(lhs[0]);
         const auto greg_rhs = boost::gregorian::from_string(rhs[0]);
         return greg_lhs < greg_rhs;
      });
      // Merge the two fields from each two_fields, creating result vector<string>
      std::vector<std::string> result;
      std::for_each(two_fields.begin(),two_fields.end(), [&](const std::vector<std::string>& entry) {
         const std::regex re("(\\d+-\\d+-\\d+).*");
         const auto just_date(std::regex_replace(entry[0],re,std::string("$1")));
         const auto greg_date = boost::gregorian::from_string(just_date);
         const auto ymd = greg_date.year_month_day();
         std::stringstream ss;
         ss << ymd.month.as_short_string() << " " << ymd.day << " " << ymd.year << "  " << entry[1];
         auto report_line(ss.str());
         UtilityRemoveHTML(report_line);
         result.push_back(report_line);
      });
      return result;
   }
}

namespace CreateBillReport {
   // Create a Bill Report, given a bill identifier such as AB12
   std::vector<std::string> ReportContents(CAPublic& db, const std::string& house, const std::string& number, 
         const std::vector<std::string>& passed_summary, const std::vector<std::string>& passed_position) {
      std::vector<std::string> result;

      const std::time_t t        = std::time(0);
      #pragma warning (push)
      #pragma warning (disable:4996)
      struct tm* now             = localtime(&t);
      #pragma warning (pop)

      const std::string bill_id       = GetBillID(db,house,number);
      const std::string vers_id       = GetBillVerID(db,house,number);
      const std::string author        = UtilityRemoveHTML(db.Author(vers_id));
      const std::string title         = UtilityRemoveHTML(GetBillTitle(db,house,number));
      const std::string location      = GetBillLocation(db,house,number);
      const std::string vote          = GetBillVoteRequired(db,vers_id);
      const std::string appropriation = GetBillAppropriation(db,vers_id);
      const std::string fiscal        = GetBillFiscal(db,vers_id);
      const std::string local_pgm     = GetBillLocalProgram(db,vers_id);

      std::vector<std::string> history = BillHistory(db,bill_id);
      const std::string last_action    = *history.rbegin();

      // With all necessary data obtained, generate the report file template.  This sets things up for entering the report manually.
      result.push_back("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n");
      result.push_back("<html xmlns=\"http://www.w3.org/1999/xhtml\" > \n");
      result.push_back("<head>\n");
      result.push_back(std::string("   <title> ") + house + "-" + number + " (" + author + ") " + title + "</title>\n");
      result.push_back("</head>\n");
      result.push_back("<body>\n");
      result.push_back("<p>\n");
      result.push_back(std::string("<b>Title</b>: ") + house + "-" + number + " (" + author + ") " + title + "\n");
      result.push_back("</p>\n");

      // Review
      result.push_back("<p>\n");
      if (passed_summary.size() == 0) {
         std::stringstream ss;
         ss << 1+now->tm_mon << "/" << now->tm_mday << "/" << 1900+now->tm_year;
         result.push_back(std::string("<b>Summary</b>: (Reviewed ") + ss.str() + ")\n");
         result.push_back("   <br /> (Quotations taken directly from the bill's language, or from current code)\n");
         result.push_back("   <br />\n");
         result.push_back("   <br /> This is my review\n");
      } else {
         std::for_each(passed_summary.begin(),passed_summary.end(), [&](const std::string& line) { result.push_back(line); });
      }
      result.push_back("</p>\n");
      
      // Position
      result.push_back("<p>\n");
      if (passed_position.size() == 0) {
         result.push_back("   <b>Position</b>: \n");
         result.push_back("   <br /> This is my reason.\n");
      } else {
         std::for_each(passed_position.begin(),passed_position.end(), [&](const std::string& line) { result.push_back(line); });
      }
      result.push_back("</p>\n");

      // Status, Location, etc
      result.push_back("<p>\n");
      result.push_back("<b>Status</b>:\n");
      result.push_back(std::string("<br /> Location: ") + location + "\n");
      result.push_back(std::string("<br /> Last Action:  ") + last_action + "\n");
      result.push_back("<table cellspacing=\"0\" cellpadding=\"0\">\n");
      result.push_back(std::string("   <tr><td> Vote: ") + vote + "             </td><td> &nbsp; &nbsp; Appropriation: " + appropriation + "</td></tr>\n");
      result.push_back(std::string("   <tr><td> Fiscal committee: ") + fiscal + " </td><td> &nbsp; &nbsp; State-mandated local program: " + local_pgm + " </td></tr>\n");
      result.push_back("</table>\n");
      result.push_back("</p>\n");

      // Bill History
      result.push_back("<p>\n");
      result.push_back("   <b>Bill History</b>:\n");
      std::for_each(history.rbegin(),history.rend(), [&](const std::string& display_line) {
         result.push_back(std::string("   <br /> ") + display_line + "\n");
      });
      result.push_back("</p>\n");
      result.push_back("</body>\n");
      result.push_back("</html>\n");
      return result;
   }

   // Create a Bill Report, given a bill identifier such as AB12
   bool Create(CAPublic& db, const std::string& bill, const std::string& report_files_folder,bool is_verbose) {
      bool result = false;
      std::string house, number;
      if (ExtractHouseNumber(bill,house,number)) {
         std::stringstream path;
         path << report_files_folder << "/" << house << number << ".html";
         auto report(ReportContents(db,house,number));
         WriteFileLineByLine(path.str(),report);
         result = true;
      }
      return result;
   }
}
