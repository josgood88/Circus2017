#include "FileDiscovery.h"
#include <hdr/FilesLib.h>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <iostream>
#include <regex>
#include <vector>

namespace {
   // Recursive descent over folders to find all files whose names match a pattern.
   void FolderRecursiveDescent(fs::path path, const std::regex& filter, std::vector<fs::path>& result) {
      fs::directory_iterator end_iter;
      for (fs::directory_iterator dir_itr(path); dir_itr != end_iter; ++dir_itr) {
         if (fs::is_regular_file(dir_itr->status())) {
            const std::string s(dir_itr->path().filename().string());
            if (std::regex_match(s,filter)) {
               result.push_back(dir_itr->path());
            }
         } else if (fs::is_directory(dir_itr->status())) {
            FolderRecursiveDescent(dir_itr->path(),filter,result);
         }
      }
   }

   // Trim duplicate files -- only want the latest for any one bill
   void TrimDuplicates(std::vector<fs::path>& source, std::vector<fs::path>& target) {
      target.reserve(source.size());               // target is no larger than source
      std::sort(source.begin(),source.end());      // last entry for a bill is the latest entry
      std::pair<std::string, fs::path> currently_tracking;           // This is the bill we're currently looking at
      std::for_each(source.begin(),source.end(), [&](const fs::path& file_path) {
         const std::string this_match(BillIdFromPath(file_path));    // e.g. "ab_6" from "full_path\ab_6_bill_20130514_status.html"
         if (this_match.length() > 0) {
            if (this_match != currently_tracking.first) {            // If this is a new bill
               if (currently_tracking.first.length() > 0) {          // If not first time through
                  target.push_back(currently_tracking.second);       // We have the latest version of a bill
               }
            }
            currently_tracking.first = this_match;                   // Latest version so far
            currently_tracking.second = file_path;
         } else {
            std::cout << "No match in " << file_path.string() << std::endl; // Should never take this branch
         }
      });
      // 
      if (currently_tracking.first.length() > 0) {          // Don't lose the last entry
         target.push_back(currently_tracking.second);
      }
   }

   // Wrap FolderRecursiveDescent, providing the target collection, the base search path, and the filter.
   std::vector<fs::path> AllCurrentFiles(const fs::path& pub_root, const std::regex& filter) {
      std::vector<fs::path> result0, result1;
      result0.reserve(20*1000);   // Avoid expanding and copying the vector (at least for a while)
      try {
         FolderRecursiveDescent(pub_root,filter,result0);   // All paths matching the regex
         TrimDuplicates(result0, result1);                  // Filter out all but latest for each bill
      } catch (const fs::filesystem_error& ex) {
         std::cout << ex.what() << std::endl;
      }
      return result1;
   }
}

std::vector<fs::path> FileDiscovery::AllCurrentBillFiles(const fs::path& pub_root) {
   std::cout << "Gathering bill file paths..." << std::endl;
   // Bill files...must contain "introduced", "enrolled", "chaptered" or "amended" in the file name
   const std::regex bill_files("[as]b_.*?(introduced.html|enrolled.html|chaptered.html|amended_.*?.html)$");
   std::vector<fs::path> paths(AllCurrentFiles(pub_root,bill_files));
   return paths;
}

std::vector<fs::path> FileDiscovery::AllCurrentStatusFiles(const fs::path& pub_root) {
   std::cout << "Gathering status file paths..." << std::endl;
   const std::regex status_files("[as]b_.*?status.html$");    // Status files for actual bills...must end with "status.html"
   std::vector<fs::path> paths(AllCurrentFiles(pub_root,status_files));
   return paths;
}

bool FileDiscovery::IsSameBillNumber(const fs::path& path1, const fs::path& path2) {
   const std::string prefix1(BillIdFromPath(path1)), prefix2(BillIdFromPath(path2));
   return prefix1 == prefix2;
}

