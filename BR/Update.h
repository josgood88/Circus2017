#pragma once
#include "CAPublic.h"
#include <boost/filesystem.hpp>

namespace Update {
   void UpdateHtmlFolder  (CAPublic& db, const std::string& report_files_folder);
   void UpdateSingleReport(CAPublic& db, const boost::filesystem::path& bill_path, bool force_report_regeneration);
   void UpdateAllReports  (CAPublic& db, const boost::filesystem::path& raw_lob_files_folder);
}