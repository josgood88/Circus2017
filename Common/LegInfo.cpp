#pragma warning (disable:4996)      // 'std::_Copy_backward': Function call with parameters that may be unsafe
#include "LegInfo.h"
#include "Performer.h"
#include "TextManipulation.h"
#include <Configuration.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>

namespace {
   const std::string ftpBase("ftp://leginfo.public.ca.gov/");
   const boost::filesystem::path baseLocalFolder("D:/CCHR");
}

namespace LegInfo_Utility {
   //
   //*****************************************************************************
   /// \brief BaseLocalFolder returns a std::string defining the root of the folder containing local copies of legislative information.
   ///        If Assembly bills are kept at "D:\CCHR\pub\11-12\bill\asm", then this method returns "D:\CCHR".
   //*****************************************************************************
   //
   boost::filesystem::path BaseLocalFolder() { return baseLocalFolder; }
   //
   //*****************************************************************************
   /// \brief CurrentLegSession returns a string specifying the current legislative session.
   ///         Legislative sessions begin on odd year and run for two years.
   ///        The session is based on the current date.
   ///        The string is "yy-yy", e.g. "11-12" for the 2011-2012 session.
   /// \param[in] year  want the legislature session that included this year 
   /// \return std::string specifying the two years of the session
   //*****************************************************************************
   //
   std::string CurrentLegSession(const std::string& config_file_path) {
      Configuration config(config_file_path);
      boost::gregorian::date today = boost::gregorian::day_clock::local_day();
      if (today.is_not_a_date()) { } // throw a TBD exception
      return LegInfo_Utility::CurrentLegSession(today.year());
   }
   std::string CurrentLegSession(unsigned int year) {
      // Sessions begin on odd years
      unsigned int noCenturies = year % 100;
      unsigned int startYear = (noCenturies % 2) ? noCenturies : (noCenturies-1 > 99) ? 99 : noCenturies-1;
      unsigned int endYear = startYear == 99 ? 0 : startYear + 1;
      std::stringstream result;
      result << std::setw(2) << std::setfill('0') << startYear << "-" 
             << std::setw(2) << std::setfill('0') << endYear;
      return result.str();
   }
   //
   //*****************************************************************************
   /// \brief StartingFolder returns the ftp site path for the first bills folder for a house of the legislature.
   ///        The path for the Assembly in the 2011-2012 session is "ftp://leginfo.public.ca.gov/pub/11-12/bill/asm/ab_0001-0050/".
   /// \param[in] house   "asm" or "sen" (not checked) indicating which house
   /// \param[in] session  e.g., "11-12" for the 2011-2012 legislative session.
   /// \return std::string specifying the ftp site path for the first bills folder for a house of the legislature
   //*****************************************************************************
   //
   std::string StartingFolder(const std::string& house, const std::string& session) {
      std::stringstream result;
      result << ftpBase << "pub/" << session << "/bill/" << house << "/" << (house[0] == 'a' ? "ab_" : "sb_") << "0001-0050/";
      return result.str();
   }
   //
   //*****************************************************************************
   /// \brief NextFolder returns the ftp site path for the next bills folder, used while iterating over the bills for a house of the legislature.
   ///        For example, if the passed-in string ends in "0001-0050", then the returned string ends in "0051-0100". \n
   ///        Entry:. "ftp://leginfo.public.ca.gov/pub/11-12/bill/asm/ab_0001-0050/" \n
   ///        Return: "ftp://leginfo.public.ca.gov/pub/11-12/bill/asm/ab_0051-0100/" \n
   /// \param[in] currentFolder   "asm" or "sen" (not checked) indicating which house
   /// \return std::string specifying the ftp site path for the next bills folder
   //*****************************************************************************
   //
   std::string NextFolder(const std::string& lastFolder) {
      const boost::regex select_first_last(".*(\\d{4})-(\\d{4}).*");    // capture first and last numbers independently
      boost::smatch match;
      if (boost::regex_search(lastFolder,match,select_first_last)) {    // Doesn't match unless both subselect_first_lasts found
         // Get first/last values and increment them
         std::stringstream firstStr, lastStr;
         firstStr << match[1]; lastStr << match[2];
         int first, last;
         firstStr >> first; lastStr >> last;
         first += 50; last += 50;
         // Get path prefix and then append the new values
         const boost::regex select_prefix("(.*?)\\d{4}-.*");
         boost::smatch match_prefix;
         if (boost::regex_search(lastFolder,match_prefix,select_prefix)) {
            std::stringstream result;
            result << match_prefix[1] << std::setfill('0') << std::setw(4) << first << "-" 
                                      << std::setfill('0') << std::setw(4) << last << "/";
            return result.str();
         } else {
            std::stringstream s;
            s << "LegInfo_Utility::NextFolder(" << lastFolder << ") -- Unable to extract prefix";
            /// \todo Who catches throw "Unable to extract prefix" ?
            throw std::exception(s.str().c_str());
         }
      } else {
         std::stringstream s;
         s << "LegInfo_Utility::NextFolder(" << lastFolder << ") -- Invalid last folder";
         /// \todo Who catches throw "Invalid last folder" ?
         throw std::exception(s.str().c_str());
      }
   }
   //
   //*****************************************************************************
   /// \brief Determine range part, e.g. 0351-0400, for a bill name.
   /// \brief [in]  input -- the "AB_383" or whatever
   //*****************************************************************************
   //
   std::string RangeText(const std::string& bill) {
      std::stringstream result;
      const boost::regex rx(".*?(\\d+)");
      boost::smatch match;
      if (boost::regex_search(bill,match,rx)) {
         std::stringstream numberStr;   // Can't be const, >> operator is used
         int numeric;
         numberStr << match[1]; numberStr >> numeric;
         const int lowerBound((numeric-1)/50 * 50 + 1);
         const int upperBound(lowerBound + 50-1);
         result << std::setw(4) << std::setfill('0') << lowerBound << "-" 
            << std::setw(4) << std::setfill('0') << upperBound;
      }
      return result.str();
   }
   //
   //*****************************************************************************
   /// \brief DirectFolder returns the ftp site path for a bill.
   /// \param[in] bill    e.g., "AB383"
   /// \param[in] session e.g., "11-12" for the 2011-2012 legislative session.
   /// \return std::string specifying the ftp site path for the bill
   //*****************************************************************************
   //
   std::string DirectFolder(const std::string& bill) {
      std::stringstream result;
      const boost::gregorian::date today = boost::gregorian::day_clock::local_day();
      if (!today.is_not_a_date()) {
         const std::string session(CurrentLegSession(today.year()));
         const bool isAsm(std::tolower(bill[0]) == 'a');
         const bool isSen(std::tolower(bill[0]) == 's');
         if (isAsm || isSen) {
            const std::string house(isAsm ? "asm" : "sen");
            const std::string prefix(isAsm ? "ab_" : "sb_");
            result << ftpBase << "pub/" << session << "/bill/" << house << "/" << prefix << RangeText(bill) << "/";
         }
      }
      return result.str();
   }
   //
   //*****************************************************************************
   /// \brief ExtractFolder removes the base folder from a path of folders on the ftp site and returns the result.
   ///        For example, \n
   ///        "ftp://leginfo.public.ca.gov/pub/11-12/bill/asm/ab_0001-0050/" becomes \n
   ///        "pub/11-12/bill/asm/ab_0001-0050/" \n
   ///        This method exists because the local folder structure follows the folder structure on the ftp site,
   ///        meaning that the same set of folder names are used, regardless of whether the root is the local root or the ftp site root.
   //*****************************************************************************
   //
   std::string ExtractFolderFromFtp(const std::string& fullPath) {
      const boost::regex select_folder(ftpBase + "(.*)");
      boost::smatch match_folder;
      if (boost::regex_search(fullPath,match_folder,select_folder)) {
         return match_folder[1];
      } else {
         std::stringstream s;
         s << "LegInfo_Utility::ExtractFolder(" << fullPath << ") -- Can't extract folder";
         /// \todo Someone needs to catch exception "Can't extract folder".  Need definitions of all exceptions.
         throw std::exception(s.str().c_str());
      }
   }
   //
   //*****************************************************************************
   /// \brief IsFilePresent answers whether a file on the ftp site is also present in the local folder structure.
   ///        For example, \n
   ///        Entry:. "pub/11-12/bill/asm/ab_0001-0050/ab_10_bill_20101206_introduced.html" \n
   ///        requires a check to determine whether \n
   ///        "D:/CCHR\pub/11-12/bill/asm/ab_0001-0050/ab_10_bill_20101206_introduced.html" \n
   ///        exists
   /// \param[in] path   path (less ftp site root) to the file on the ftp site
   //*****************************************************************************
   //
   bool IsFilePresentLocally(const std::string& path) {
      boost::filesystem::path fullPath(baseLocalFolder/path);
      bool result = boost::filesystem::exists(fullPath);
      return result;
   }
   //
   //*****************************************************************************
   /// \brief EndsWith answers whether a string has a specific ending regex.
   /// \param[in] checkThis    does this string end as specified?
   /// \param[in] endsWithThis the ending specification
   //*****************************************************************************
   //
   bool EndsWith(const std::string& checkThis, const std::string& endsWithThis) {
      std::stringstream regexBuilder;
      regexBuilder << ".*?(" << endsWithThis << ")$";
      const boost::regex re(regexBuilder.str());
      boost::smatch match;
      return boost::regex_search(checkThis,match,re);
   }
   //
   //*****************************************************************************
   /// \brief LocalFolderCorrespondingToRemoteFolder translates a remote folder to a local folder.
   ///        For example, for an input argument of \n
   ///        "pub/11-12/bill/asm/ab_0001-0050/ab_10_bill_20101206_introduced.html" \n
   ///        this method returns \n
   ///        "D:/CCHR/pub/11-12/bill/asm/ab_0001-0050/" \n
   /// \param[in] inputPath   path (less ftp site root) to the file on the ftp site
   //*****************************************************************************
   //
   std::string LocalFolderCorrespondingToRemoteFolder(const std::string& remotePath) {
      std::string folder(remotePath);
      const std::string extension = boost::filesystem::path(remotePath).extension().string();
      if (extension == ".html" || extension == ".htm") {                // Assume files end in .html or .htm.  Anything else considered to be folder.
         folder = (boost::filesystem::path(remotePath)).remove_filename().string();
      }
      boost::filesystem::path fullPath(baseLocalFolder/folder);         // Make the full path
      return fullPath.string();
   }
   //
   //*****************************************************************************
   /// \brief EnsureFolderPresent ensures a folder is present on the local filesystem for receiving a file.
   ///        It creates the file, if necessary.
   ///        For example, for an input argument of \n
   ///        "pub/11-12/bill/asm/ab_0001-0050/ab_10_bill_20101206_introduced.html" \n
   ///        this method ensures that \n
   ///        "D:/CCHR/pub/11-12/bill/asm/ab_0001-0050/" \n
   ///        exists \n
   ///        Note:  Circus only deals with HTML files, so if the extension is something other than .html, the leaf is considered to be a folder.
   /// \param[in] inputPath   path (less ftp site root) to the file on the ftp site
   //*****************************************************************************
   //
   void EnsureFolderPresent(const std::string& inputPath) {
      const boost::filesystem::path fullPath(LocalFolderCorrespondingToRemoteFolder(inputPath));
      if (!boost::filesystem::exists(fullPath)) {                       // If the full folder path doesn't exist
         boost::filesystem::path currentPath;
         for (boost::filesystem::path::iterator it = fullPath.begin(); it != fullPath.end(); ++it) {
            currentPath /= *it;
            if (!boost::filesystem::exists(currentPath)) {
               if (!boost::filesystem::create_directory(currentPath)) {
                  /// \todo: Convert std::cout << "Failed to create " to an exception
                  std::cout << "Failed to create " << currentPath.string() << std::endl;
               }
            }
         }
      }
   }
   //
   //*****************************************************************************
   /// \brief BillNumberFromFullPath extracts the bill number from the full path.
   ///        For example, for an input argument of \n
   ///        "pub/11-12/bill/asm/ab_0001-0050/ab_10_bill_20101206_introduced.html" \n
   ///        this method returns \n
   ///        "ab_10" \n
   /// \param[in] inputPath   path (less local or ftp site root) to the file
   //*****************************************************************************
   //
   std::string BillNumberFromFullPath(const std::string& inputPath) {
      boost::filesystem::path fsPath(inputPath);
      const std::string filename(fsPath.filename().string());
      const boost::regex rx("[a-z]*_\\d+");
      boost::smatch match;
      if (boost::regex_search(filename,match,rx)) {
         return match[0].str();
      } else {
         return std::string();
      }
   }
   //
   //*****************************************************************************
   /// \brief EnsureSingleCopy ensures there is only one copy of a file type.
   ///        Status and History files come with embedded dates.  The leg site will have only one copy.
   ///        The local folder will accumulate new status and history files as time goes along, because
   ///        revisions to these files have different names (different dates) and so appear to be different
   ///        files unless some intelligence is applied.
   /// \param[in,out] folderListing   paths to files for a single bill on the ftp site
   /// \param[in] subStringIdentifier this string identifies the file type of interest
   //*****************************************************************************
   //
   // Logic:
   //    1) Find all leg site files of the type we want -- history or status
   //    2) Find all local files of the same type
   //    3) For each leg site file (which means for each bill, since typically each bill has one such file)
   //    4)    Collect the local files for that bill from the list generated in step 2.
   //    5)    Find the most recent such local file
   //    6)    If the leg site has a more recent file then it needs to be copied
   //    7)       Delete all the local files -- we don't need any of them
   //    8)    Else
   //    10)      Keep the latest local file, but delete all earlier ones
   //    11)   End If
   //    12) End For
   //
   void EnsureSingleCopy(std::vector<std::string> folderListing, const std::string& subStringIdentifier) {
      if (folderListing.size() == 0) return;
      if (subStringIdentifier.length() == 0) return;

      // 1) Find all leg site files of the type we want -- history or status
      // Extract those files whose names contain 'subStringIdentifier'
      const std::string id(TextManipulation::LowerCase(subStringIdentifier));
      std::vector<std::string> filesOfInterest;
      std::copy_if(folderListing.begin(),folderListing.end(), std::back_inserter(filesOfInterest), [&](const std::string& entry) -> bool {
         const std::string lcEntry(TextManipulation::LowerCase(entry));
         return lcEntry.find(id) != std::string::npos;
      });

      // 2) Find all local files of the same type
      // Collect files in the local folder whose names contain 'subStringIdentifier'.  Do not recur to lower levels in the file system.
      const boost::filesystem::path localPath(LocalFolderCorrespondingToRemoteFolder(filesOfInterest[0]));
      std::vector<std::string> localMatchingFiles;
      boost::filesystem::directory_iterator itr(localPath);
      boost::filesystem::directory_iterator end;
      for (; itr != end; ++itr) {
         if (!boost::filesystem::is_directory(*itr)) { 
            const std::string lcEntry(TextManipulation::LowerCase(itr->path().string()));
            if (lcEntry.find(id) != std::string::npos) {
               localMatchingFiles.push_back(lcEntry); 
            }
         }
      }
      
      // 3) For each leg site file (which means for each bill, since typically each bill has one such file)
      BOOST_FOREACH (std::string remoteFile, filesOfInterest) {

         // 4)    Collect the local files for that bill from the list generated in step 2.
         // Isolate 'subStringIdentifier' local files relevant to the current bill.
         std::vector<std::string> currentBillFilesContainingID;
         const std::string lcCurrentBill(TextManipulation::LowerCase(BillNumberFromFullPath(remoteFile)));
         std::copy_if(localMatchingFiles.begin(),localMatchingFiles.end(), std::back_inserter(currentBillFilesContainingID), [&](const std::string& entry) -> bool {
            const std::string lcEntry(TextManipulation::LowerCase(entry));
            return BillNumberFromFullPath(lcEntry) == lcCurrentBill;
         });

         //    5)    Find the most recent such local file
         // Answer whether latest copy on the local folder is earlier than the latest on the leg site
         std::sort(currentBillFilesContainingID.begin(),currentBillFilesContainingID.end(), [](const std::string& a, const std::string& b) {
            return a.compare(b) > 0;
         });
         if (currentBillFilesContainingID.size() == 0) continue;
         const std::string mostRecentLocalFile(currentBillFilesContainingID[0]);
         const std::string siteFileName (boost::filesystem::path(remoteFile)         .filename().string());
         const std::string localFileName(boost::filesystem::path(mostRecentLocalFile).filename().string());
         const bool needToCopyFileFromLegSite(siteFileName.compare(localFileName) > 0);

         //    6)    If the leg site has a more recent file then it needs to be copied
         //    7)       Delete all the local files -- we don't need any of them
         auto itr2(currentBillFilesContainingID.begin());
         //    8)    Else
         //    10)      Keep the latest local file, but delete all earlier ones
         if (!needToCopyFileFromLegSite) ++itr2;
         std::for_each(itr2,currentBillFilesContainingID.end(), [](const std::string& entry) {
            boost::filesystem::remove(boost::filesystem::path(entry));
         });
      }
   }
   //
   //*****************************************************************************
   /// \brief WriteFile creates a file on the local filesystem, removing any previous copy, and writes its contents.
   /// \param[in] filePath     path to the file being created
   /// \param[in] fileContents contents to be written to the file
   //*****************************************************************************
   //
   void WriteFile(const std::string& filePath, const std::string& fileContents) {
      boost::filesystem::path fullPath(baseLocalFolder/filePath);
      std::ofstream os(fullPath.string());
      os << fileContents;
   }
}

