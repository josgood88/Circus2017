#pragma once 

#include <string>
#include <boost/filesystem.hpp>

//
/// Free Functions used by Synchonize
//
namespace LegInfo_Utility {
   boost::filesystem::path BaseLocalFolder();
   std::string CurrentLegSession   (const std::string& config_file_path);
   std::string CurrentLegSession   (unsigned int year);
   bool        EndsWith            (const std::string& checkThis, const std::string& endsWithThis);
   std::string ExtractFolderFromFtp(const std::string& fullPath);
   bool        IsFilePresentLocally(const std::string& path);
   void        EnsureFolderPresent (const std::string& path);
   void        EnsureSingleCopy    (std::vector<std::string> folderListing, const std::string& subStringIdentifier);
   std::string NextFolder          (const std::string& lastFolder);
   std::string StartingFolder      (const std::string& house, const std::string& session);
   std::string DirectFolder        (const std::string& bill);
   void        WriteFile           (const std::string& filePath, const std::string& fileContents);
}

