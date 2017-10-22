#include "CAPublic.h"
#include "CreateBillReport.h"
#include "Logger.h"
#include "Update.h"
#include "Utility.h"

#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <regex>
#include <string>
#include <vector>

namespace fs = boost::filesystem;

namespace {
   std::vector<std::vector<std::string>> BillHistory(CAPublic& db, const std::string& house, const std::string& number) {
      std::stringstream ss1,ss2;
      ss1 << "Select BillId from BillRows Where MeasureType = '" << house << "' AND MeasureNum = " << number << ";";
      const auto bill_id(db.BillTableField(ss1.str()));
      return db.BillHistory(bill_id);
   }

   std::vector<std::string> ExtractFromReport(const std::vector<std::string>& report_contents, bool take_first, const std::string& start, const std::string& end) {
      std::vector<std::string> result;
      auto itr(report_contents.begin());
      while (itr != report_contents.end()) {
         if (itr->find(start) != std::string::npos) {
            if (take_first) result.push_back((*itr++)+"\n");
            else            itr++;
            while (itr != report_contents.end() && itr->find(end) == std::string::npos) { 
               result.push_back((*itr++)+"\n"); 
            }
            break;
         }
         itr++;
      }
      return result;
   }

   std::vector<std::string> ReplaceReportSection(const std::vector<std::string>& report_contents, const std::string& replace_line, const std::vector<std::string> replacement) {
      std::vector<std::string> result;
      auto replace_this_line(std::find_if(report_contents.begin(),report_contents.end(), [&](const std::string& line) {
         return line.compare(replace_line) == 0;
      }));
      if (replace_this_line > report_contents.begin())
         result.insert(result.begin(),report_contents.begin(),replace_this_line-1);
      result.insert(result.end(),replacement.begin(),replacement.end());
      result.insert(result.end(),replace_this_line+1,report_contents.end());
      return result;
   }

   // Determine whether a report needs to be updated.
   //
   bool ReportNeedsToBeUpdated(CAPublic& db, const fs::path& p, const std::string& house, const std::string& number) {
      // Extract the bill's current history, and the history as it was when the bill was last reviewed.
      const auto history_from_table(BillHistory(db,house,number));
      const auto report_contents(ReadFileLineByLine(p.string()));
      const auto history_from_report(ExtractFromReport(report_contents,false,"Bill History","</p>"));

      // If the bill history has changed, then the report needs to be updated.
      return history_from_table.size() != history_from_report.size();
   }

   // If so
   //    Save the report's summary and position sections.
   //    Regenerate the bill report, resulting in generic summary and position sections.
   //    Replace those generic summary & position sections with the saved sections.
   //
   //void ProcessReport(CAPublic& db, const fs::path& p, const std::string& report_files_folder) {
   //   // The orginating house, and the bill number, identify the bill.
   //   const auto house (ExtractHouseFromBillID(p));
   //   const auto number(ExtractBillNumberFromBillID(p));

   //   // Extract the bill's current history, and the history as it was when the bill was last reviewed.
   //   const auto history_from_table(BillHistory(db,house,number));
   //   const auto report_contents(ReadFileLineByLine(p.string()));
   //   const auto history_from_report(ExtractFromReport(report_contents,false,"Bill History","</p>"));

   //   // If the bill history has changed, then the report needs to be updated.
   //   if (history_from_table.size() != history_from_report.size()) {
   //      std::stringstream ss;
   //      ss << p.filename() << " has changed.";
   //      Logger::Log(ss.str());

   //      // Save the report's summary and position sections.
   //      const auto summary(ExtractFromReport(report_contents, true, "Summary", "</p>"));
   //      const auto position(ExtractFromReport(report_contents,true, "Position","</p>"));

   //      // Regenerate the bill report, resulting in generic summary and position sections.
   //      auto new_contents(CreateBillReport::ReportContents(db,house,number));

   //      // Replace those generic summary & position sections with the saved sections.
   //      new_contents = ReplaceReportSection(new_contents, review_marker, summary);
   //      new_contents = ReplaceReportSection(new_contents, reason_marker, position);
   //      WriteFileLineByLine(p.string(),new_contents);
   //   }
   //}
}

namespace Update {
   // Update all reports in the reports folder
   void UpdateHtmlFolder(CAPublic& db,const std::string& report_files_folder) {
      const auto reports(FolderContents(report_files_folder));
      std::for_each(reports.begin(),reports.end(), [&](const fs::path& p) {
         if (p.stem().string() != "WeeklyNewsMonitoredBills") UpdateSingleReport(db,p,false);
      });
   }

void UpdateAllReports  (CAPublic& db, const boost::filesystem::path& html_files_folder) {
   const auto reports(FolderContents(html_files_folder.string()));
   std::for_each(reports.begin(),reports.end(), [&](const fs::path& p) {
      if (p.stem().string() != "WeeklyNewsMonitoredBills") UpdateSingleReport(db,p,true);
   });
}

   void UpdateSingleReport(CAPublic& db,const fs::path& p, bool force_report_regeneration) {
      // The orginating house, and the bill number, identify the bill.
      const auto house (ExtractHouseFromBillID(p));
      const auto number(ExtractBillNumberFromBillID(p));

      if (force_report_regeneration || ReportNeedsToBeUpdated(db, p, house, number)) {
         std::stringstream ss;
         if (force_report_regeneration) {
            ss << "Updating " << p.filename();
         } else {
            ss << p.filename() << " has changed.";
         }
         LoggerNS::Logger::Log(ss.str());

         // Save the report's summary and position sections.
         const auto report_contents(ReadFileLineByLine(p.string()));
         const auto summary(ExtractFromReport(report_contents, true, "<b>Summary</b>:", "</p>"));
         const auto position(ExtractFromReport(report_contents,true, "<b>Position</b>:","</p>"));

         // Regenerate the bill report, resulting in generic summary and position sections.
         auto new_contents(CreateBillReport::ReportContents(db,house,number,summary,position));

         // Replace those generic summary & position sections with the saved sections.
         WriteFileLineByLine(p.string(),new_contents);
      }
   }
}
