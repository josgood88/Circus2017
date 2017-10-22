#pragma once

#include <string>
#include <vector>

namespace CreateBillReport {
   bool Create(CAPublic& db, const std::string& bill, const std::string& report_files_folder, bool is_verbose);
   std::vector<std::string> ReportContents(CAPublic& db, const std::string& house, const std::string& number,
      const std::vector<std::string>& passed_summary = std::vector<std::string>(), 
      const std::vector<std::string>& passed_position = std::vector<std::string>());
}

namespace {
      const std::string review_marker("   <br /> Review\n");
      const std::string reason_marker("   <br /> This is my reason.\n");
}
