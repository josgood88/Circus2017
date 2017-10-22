//
/// \page Synchronize Synchronize
/// \remark Synchronize's purpose is to maintain local copies of the bill information on leginfo.public.ca.gov.
//
///   <h1>Responsibilities</h1>
///   Synchronize's responsibilities are:
///   -# Create the necessary folder structure
///   -# Ensure the current session's folder structure contains up-to-date information.
///   -# Signal each bill scanned on the leg site (one signal per bill)
///   -# Signal each bill downloaded from the leg site (one signal per download)
//
#pragma warning (disable:4996)      // 'std::_Copy_backward': Function call with parameters that may be unsafe

#include <Configuration.h>
#include <ConfigurationFilePath.h>
#include "LegInfo.h"
#include "LocalFileLocation.h"
#include <MessageTypes.h>
#include <Performer.h>
#include <QueueNames.h>
#include "TextManipulation.h"

#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <Poco/Net/FTPClientSession.h>
#include <Poco/Net/NetException.h>
#include <Poco/StreamCopier.h>

#include <iostream>
#include <regex>
#include <string>
#include <vector>

namespace {
   std::string CurrentLegSession();
   boost::scoped_ptr<Poco::Net::FTPClientSession> session;
   boost::scoped_ptr<Configuration> config;

   std::vector<std::string> latestLocalVersions;
   std::vector<std::string> allVersionsonLegSite;
   bool exitSynchronize(false);

   boost::scoped_ptr<Performer> synchronize;
   bool running_stand_alone = false;                                 // Stand-alone testing has no interprocess queues

   //
   //*****************************************************************************
   /// \brief GetFtpFile fetches a single bill from the ftp site, returning the contents in a std::string
   /// \param[in] siteFile  defines the file to be fetched
   /// \return std::string containing the contents of the file.
   //*****************************************************************************
   //
   std::string GetFtpFile(const std::string& siteFile) {
      // Download the file from the ftp site
      std::istream& istr = session->beginDownload(siteFile);
      std::ostringstream oStr;
      Poco::StreamCopier::copyStream(istr, oStr);
      session->endDownload();
      return oStr.str();
   }
   //*****************************************************************************
   // Fetch folder listing (all file names) into a std:string
   //*****************************************************************************
   std::string Listing(const std::string& siteFolder) {
      std::istream& istr = session->beginList(siteFolder);
      std::ostringstream oStr;
      Poco::StreamCopier::copyStream(istr, oStr);
      session->endList();
      std::string listing(oStr.str());
      return listing;
   }

   void LogEither(const std::string& s) {
      if (!running_stand_alone) synchronize->LogThis(s);
      else std::cout << s << std::endl;
   }
   void LogEither(const std::stringstream& ss) { LogEither(ss.str()); }
   //
   //*****************************************************************************
   //  Support functions for SynchronizeFiles
   //*****************************************************************************
   //
   /// Extract bill name from path to the local bill file
   std::string Stem(const std::string& filePath) {
      const size_t pos2(filePath.find_last_of('/'));
      const size_t pos1(filePath.find("_bill",pos2-1));
      const std::string fn(filePath.substr(pos2+1,pos1-pos2-1));
      return fn;
   }

   /// Report each bill as scanned.  Each bill is represented by multiple files.  Report each bill once.
   std::string lastReported("Nothing");
   void ReportLegSiteScan(const std::string filePath) {
      if (filePath.find(lastReported) == filePath.npos) {
         const std::string fn(Stem(filePath));
         lastReported = fn + "_";   // Need underscore, else 4-digit file name ab_1001 can match folder pub/15-16/bill/asm/ab_1001-1050
         if (!running_stand_alone && Normal == Performer::executionType) synchronize->SendLegSiteProg(fn); // Report file name to GUI
         else                                                            std::cout << fn << std::endl;     // ...or to std::cout
      }
   }

   /// Report each bill fetched.  OK to report the bill multiple times -- once for each fetch.
   void ReportLegBillFetch(const std::string filePath) {
      const std::string fn(Stem(filePath));
      if (!running_stand_alone && Normal == Performer::executionType) synchronize->SendFileProg(fn);    // Report file name to GUI
      else                                                            std::cout << fn << std::endl;     // ...or to std::cout
   }
   //
   //*****************************************************************************
   /// \brief SynchronizeFiles fetches all bills named in a passed vector<string>.
   ///        All bills are assumed to be in the same folder (not checked).
   ///        Bills already present in local storage are not fetched again.
   ///        Namespace LegInfo_Utility is responsible for knowing where the local folder is.
   /// \param[in] listing    defines the files to be fetched
   //*****************************************************************************
   //
   void SynchronizeFiles(std::vector<std::string> listing) {
      LegInfo_Utility::EnsureFolderPresent(listing[0]);                    // Ensure local folder is present
      std::vector<std::string> htmlOnly(TextManipulation::Extract(listing,".html")); // Only care about html files
      BOOST_FOREACH(std::string path, htmlOnly) {
         if (exitSynchronize) break;                                       // Exit if the thread has been told to stop
         ReportLegSiteScan(path);                                          // Show file name in "Leg Site Scan" progress display
         if (!LegInfo_Utility::IsFilePresentLocally(path)) {               // If the file does not exist
            ReportLegBillFetch(path);                                      // Show bill fetch
            std::string fileContents(GetFtpFile(path));                    // Fetch the file from the ftp site
            LegInfo_Utility::WriteFile(path,fileContents);                 // Create local copy of the file
            if (!running_stand_alone) {
               std::stringstream ss;
               ss << "To BillRouter: " << path;
               synchronize->LogThis(ss.str());
               synchronize->Send(MsgHTMLFileName,path,Name_BillRouterQueue);  // Tell the BillRouter about it
            }
            //TODO: If Synchronize can't send to BillRouter, then shut the circus down
         } else {
            // Comment out unless you want to see every file we've already fetched
            //std::stringstream ss;
            //ss << "Already present: " << path;
            //LogEither(ss.str());
         }
      }
   }
   //
   //*****************************************************************************
   /// \brief FolderContents returns a vector containing a listing of all files in a folder.
   /// \param[in] sourceFolder  defines the folder
   /// \return std::vector<std::string> containing a listing of all files in the folder.
   //*****************************************************************************
   //
   std::vector<std::string> FolderContents(std::string sourceFolder) {
      std::vector<std::string> result;
      std::string siteFolder(LegInfo_Utility::ExtractFolderFromFtp(sourceFolder));  // Trim ftp root from sourceFolder
      try {
         // Fetch folder listing (all file names) into a std:string
         std::string listing(Listing(siteFolder));

         // Split the folder listing into individual lines
         if (listing.length() > 0) {
            boost::regex re("\\n+");
            boost::sregex_token_iterator i(listing.begin(), listing.end(), re, -1);
            boost::sregex_token_iterator j;
            while (i != j) result.push_back(*i++);
            std::stringstream ss;
            ss << sourceFolder << " contains " << result.size() << " files.";
            LogEither(ss.str());
         }
      } catch (const Poco::Net::NetException& ex) {
         if (ex.code() != 550) { // "NLST command failed: 550 pub/11-12/bill/asm/ab_2701-2750/: No such file or directory."
            /// \todo: Handle exceptions.  Likely handling is to send them out on an error queue, notify that we're exiting, and then exit.
            LogEither("FolderContents: ");
            int a = 1;
         }
      } catch (...) {
         int a = 1;
      }
      return result;
   }
   //
   //*****************************************************************************
   /// \brief CurrentLegSession returns a string specifying the current legislative session.
   ///        The session is based on the current date.
   ///        The string is "yy-yy", e.g. "11-12" for the 2011-2012 session.
   //*****************************************************************************
   //
   std::string CurrentLegSession() {
      const std::string biennium(config->Biennium());
      if (biennium.length() != 0) return biennium;
      // Here configuration has not specified which biennium to use.  Use the current biennium.
      boost::gregorian::date today = boost::gregorian::day_clock::local_day();
      if (today.is_not_a_date()) { } // throw a TBD exception
      return LegInfo_Utility::CurrentLegSession(today.year());
   }
   //
   //*****************************************************************************
   /// \brief UpdateBills fetches all bills that are in one house of the legislature.
   ///        Bills already present in local storage are not fetched again.
   /// \param[in] folderPath   
   //*****************************************************************************
   //
   void ReportFolderProgess(const std::string folderPath) {
      size_t pos2(folderPath.find_last_of('/'));
      size_t pos1(folderPath.find_last_of('/',pos2-1) + 1);
      std::string legSiteFolder(folderPath.substr(pos1,pos2-pos1));
      std::stringstream ss;
      ss << "Syncronize: " << legSiteFolder;
      LogEither(ss.str());                                        // Report current folder to status bar
   }
   //
   void UpdateBills(const std::string& house) {
      if (exitSynchronize) return;
      std::string legSiteFolder(LegInfo_Utility::StartingFolder(house,CurrentLegSession()));  // Start with 0001-0050 folder
      try {
         std::vector<std::string> listing;
         do {
            ReportFolderProgess(legSiteFolder);
            listing = FolderContents(legSiteFolder);              // Fetch folder contents (directory listing)
            if (listing.size() == 0) return;                      // Empty contents taken to mean end of leg site folders
            SynchronizeFiles(listing);                            // Synchronize local folder to ftp site's folder
            legSiteFolder = LegInfo_Utility::NextFolder(legSiteFolder);
         } while (!exitSynchronize && listing.size() > 0);
      } catch (const Poco::Net::NetException& ex) {
         /// \todo: Handle exceptions.  Likely handling is to send them out on an error queue, notify that we're exiting, and then exit.
         std::stringstream ss;
         ss << "Synchronize::UpdateBills: " << ex.message();
         LogEither(ss.str());
      } catch (...) {
         std::string s("Synchronize::UpdateBills: ellipsis exception");
         LogEither(s);
      }

   }
   //
   //*****************************************************************************
   /// \brief Iterate over the leg site, fetching all bills that aren't already present locally.
   //*****************************************************************************
   //
   void FetchBills() {
      try {
         UpdateBills("asm");                                      // Fetch all Assembly bills in the current legislative session
         UpdateBills("sen");                                      // Fetch all Senate bills in the current legislative session
      } catch (const Poco::Net::NetException& ex) {
         std::stringstream ss;
         ss << "FetchBills, Poco::Net::NetException: " << ex.message() << std::endl;
         LogEither(ss.str());
      } catch (...) {
         std::stringstream ss;
         ss << "FetchBills, ellipsis exception: " << std::endl;
         LogEither(ss.str());
      }
      if (!running_stand_alone) {
         synchronize->Send(MsgLastInSequence,"Synchronize Last Msg",Name_BillRouterQueue);
         synchronize->LogThis("Synchronize sending completion message");
         synchronize->Send(MsgCountCompletedProcess,"Synchronize completed",Name_RingMasterQueue);
      } else {
         exitSynchronize = true;
      }
   }
   //
   //*****************************************************************************
   /// \brief Handle Shutdown command from RingMaster.
   //*****************************************************************************
   //
   void Handler_Shutdown(MessageType /*type*/, const std::string& /*message*/, bool& setToExitProcess) {
      setToExitProcess = true;
      exitSynchronize = true;
   }
   //
   //*****************************************************************************
   /// \brief Extract a subset of std::vector<std::string> matching a string that std::string::find can handle
   ///        Comparisons are not case-sensitive
   //*****************************************************************************
   //
   std::vector<std::string> Extract(const std::vector<std::string>& input, const std::string matchUnknownCase) {
      std::vector<std::string> result;
      std::string match(TextManipulation::LowerCase(matchUnknownCase));
      std::copy_if(input.begin(), input.end(), std::back_inserter(result), [&](const std::string& str) -> bool {
         const std::string item(TextManipulation::LowerCase(str));
         return item.find(match) != std::string::npos; 
      });
      return result;
   }
   //
   //*****************************************************************************
   /// \brief Running in SingleFile mode. Fetch the files for a single bill.
   //*****************************************************************************
   //
   void FetchSingleBill(const std::string singleBill) {
      try {
         const std::string legSiteFolder(LegInfo_Utility::DirectFolder(singleBill));      // Folder on the leg site
         const std::vector<std::string> listing(FolderContents(legSiteFolder));           // Fetch folder contents (directory listing)
         if (listing.size() > 0) {
            // Make a lowercase copy of 'singleBill'.  
            const std::string lowerBill(TextManipulation::LowerCase(singleBill));
            std::regex rgx("([a-z]+)(\\d+)");
            const std::string lower_bill = std::regex_replace(lowerBill,rgx,std::string("$1_$2"));
            // Extract leg site files related to the current bill
            const std::vector<std::string> filtered(TextManipulation::Extract(listing,lower_bill));
            // Process the result as though it represented the entire leg site folder.
            SynchronizeFiles(filtered);
         }
      } catch (const Poco::Net::NetException& ex) {
         std::stringstream ss;
         ss << "FetchSingleBill, Poco::Net::NetException: " << ex.message() << std::endl;
         LogEither(ss.str());
      } catch (...) {
         std::stringstream ss;
         ss << "FetchSingleBill, ellipsis exception: " << std::endl;
         LogEither(ss.str());
      }
   }
}
//
//*****************************************************************************
/// Synchronize is responsible for
/// -# Fetching all bills from the leg site that have been updated since the last time Circus scanned them
/// -# Reporting each leg site folder as it is scanned
/// -# Accepting a shutdown message from the RingMaster and terminating the scan in a reasonable time
//*****************************************************************************
//
void main(int argc, char* argv[]) {
   // Synchronize can be run 
   //    1) Manually to get data on a single bill, e.g., Synchronize AB383
   //    2) Manually to remove old status and history files on the local drive
   //    3) Automatically as part of the normal weekly CCHR processing
   Performer::executionType = Normal;
   if (argc > 1) {
      Performer::executionType = std::string(argv[1]) == "StandAlone" ? StandAlone : SingleFile;
   }

   std::stringstream err_msg;
   try {
      config.reset(new Configuration(path_config_file));
      session.reset(new Poco::Net::FTPClientSession(config->Site()));
      session->login(config->User(),config->Password());

      // There is an interprocess queue only when running normally
      running_stand_alone = true;
      if (Performer::executionType == Normal) {
         running_stand_alone = false; 
         try {
            synchronize.reset(new Performer("Synchronize",Name_SynchronizeQueue));
         } catch (boost::interprocess::interprocess_exception ex) {
            std::string msg(std::string("Synchronize start: ") + ex.what());
            LogEither(msg);
         } catch (std::exception ex) {
            std::string msg(std::string("Synchronize start: ") + ex.what());
            LogEither(msg);
         } catch (...) {
            std::string msg("Synchronize start: Ellipsis exception");
            LogEither(msg);
         }
         synchronize->CommonInitialization();
      }

      switch (Performer::executionType) {
         case SingleFile:                                   // Test synchronizing a single bill
            err_msg << "Fetching files for " << argv[1];
            LogEither(err_msg.str());
            FetchSingleBill(argv[1]);                       // e.g., AB_383
            break;
         case StandAlone:                                   // Synchronize with legislative site, fetching all necessary files
            FetchBills();
            break;
         case Normal:                                       // Normal production processing

            // FetchBills iterates over the leg site until through or told to stop.
            boost::asio::io_service m_io;
            boost::thread_group m_threads;
            m_threads.create_thread(&FetchBills);

            // Prepare message dispatching.  Wait for iteration completion or stop command.
            if (!running_stand_alone) {
               synchronize->AddHandler(MsgTypeQueueShutdown,Handler_Shutdown);
               synchronize->MessageDispatcher();
            } else {
               while (exitSynchronize == false) {
                  boost::this_thread::sleep(boost::posix_time::seconds(1));   // check every second
               }
            }

            // Shutdown and exit
            LogEither("Synchronize is completing its run.");
            m_io.stop();
            m_threads.join_all();
            break;
      }

      // Shutdown and exit
      if (Performer::executionType != Normal) {
         std::cout << "Synchronize run is complete.  Press character and Enter to shut down.";
         char c;
         std::cin >> c;
      } else {
         synchronize->CommonShutdown();
      }

   // Catch exceptions thrown during configuration and session establishment
   } catch (const boost::exception& e) {
      err_msg << "Configuration error: " << boost::diagnostic_information(e);
      LogEither(err_msg);
   } catch (const std::exception& e) {
      err_msg << "Configuration error: " << e.what();
      LogEither(err_msg);
   } catch (...) {
      err_msg << "Configuration error: Unknown exception.";
      LogEither(err_msg);
   }
   // Close the ftp session
   session->close();
}
