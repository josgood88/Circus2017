#include "HistoryCleanup.h"
#include <boost/filesystem.hpp>

#include <algorithm>
#include <iostream>
#include <regex>
#include <string>
#include <sstream>
#include <vector>

namespace {
   std::vector<boost::filesystem::path> Extract(const std::vector<boost::filesystem::path>& source, const std::string& selector) {
      std::vector<boost::filesystem::path> result;
      std::copy_if(source.begin(),source.end(), std::back_inserter(result), 
         [&] (const boost::filesystem::path& path) ->bool {
            const std::string str(path.string());
            return str.find(selector) != std::string::npos;
         }
      );
      return result;
   }

   std::vector<std::string> GetUniqueMeasures(const std::vector<boost::filesystem::path>& files) {
      std::vector<std::string> result;
      // Extract the bill names from the file names
      std::for_each(files.begin(), files.end(), [&](const boost::filesystem::path& p) {
         result.push_back(std::regex_replace(p.filename().string(), std::regex("(.+)(_bill.*?html)"), std::string("$1"))); // regex needs explicit full coverage, .*? doesn't cut it
      });
      // Remove duplicate file names
      std::sort(result.begin(),result.end());
      result.erase(std::unique(result.begin(),result.end()),result.end());
      return result;
   } 

   void RemoveObsoleteVersionsForOneMeasure(const std::vector<boost::filesystem::path>& files, const std::string& measure) {
      // Extract the files relevant to the passed-in measure
      std::vector<boost::filesystem::path> filesThisMeasure;
      std::stringstream fmt;
      fmt << ".+" << measure << "_.+?html";
      std::copy_if(files.begin(), files.end(), std::back_inserter(filesThisMeasure), [&](const boost::filesystem::path& p) -> bool {
         bool result = std::regex_match(p.string(), std::regex(fmt.str()));
         return result;
      });
      
      // Erase all files for this measure except the latest one
      std::sort(filesThisMeasure.begin(), filesThisMeasure.end());   // Sort in ascending date order
      // Remove the latest file so it doesn't get erased
      filesThisMeasure.erase(std::max_element(filesThisMeasure.begin(),filesThisMeasure.end()));
      std::for_each(filesThisMeasure.begin(),filesThisMeasure.end(), [](const boost::filesystem::path& p) {
         boost::filesystem::remove(p);
      });
   }

   void RemoveObsoleteVersions(const std::vector<boost::filesystem::path>& files) {
      std::vector<std::string> uniqueBills(GetUniqueMeasures(files));
      std::for_each(uniqueBills.begin(), uniqueBills.end(), [&](const std::string& measure) {
         RemoveObsoleteVersionsForOneMeasure(files,measure);
      });
   }

   void Cleanup(boost::filesystem::path basePath, std::string append) {
      const boost::filesystem::path path(basePath / append);
      boost::filesystem::directory_iterator itr(path);
      std::vector<boost::filesystem::path> filesInCurrentFolder;

      // Obtain paths to all files in this folder, recuring as necessary (depth-first)
      std::for_each(itr, boost::filesystem::directory_iterator(), [&] (boost::filesystem::path path2) {
            if (boost::filesystem::is_directory(path2)) Cleanup(path2,std::string());
            else if (path2.extension() == ".html") filesInCurrentFolder.push_back(path2);
      });

      if (filesInCurrentFolder.size() > 0) {
         std::cout << "\tClearing " << path << std::endl;
         // Separate out the history and status files
         std::vector<boost::filesystem::path> historyFiles(Extract(filesInCurrentFolder,"history.html"));
         std::vector<boost::filesystem::path> statusFiles (Extract(filesInCurrentFolder,"status.html" ));

         // Remove obsolete history and status files
         RemoveObsoleteVersions(historyFiles);
         RemoveObsoleteVersions(statusFiles);
      }
   }
}

// Iterate over all folders, removing obsolete history and status files
void HistoryCleanup::Perform(boost::filesystem::path basePath) {
   Cleanup(basePath,"pub/13-14/bill");
}
