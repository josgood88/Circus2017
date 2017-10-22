//
/// \page BillRouter BillRouter
/// \remark Bill Router receives new bill information from Synchronize and routes it appropriately.
//
///   <h1>Responsibilities</h1>
///   BillRouter's responsibilities are:
///   -# Route .html files to text conversion
///   -# Route .txt files to bill ranking
//
#pragma warning (disable:4996)      // 'std::_Copy_backward': Function call with parameters that may be unsafe
#include "Database/DB.h"
#include "Database/RowBillTable.h"
#include <MessageTypes.h>
#include <Performer.h>
#include <QueueNames.h>

#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <algorithm>
#include <regex>
#include <string>
#include <vector>

namespace {
   boost::scoped_ptr<Performer> billRouter;
   std::vector<std::string> routeHTML;                // HTML file router table
   std::vector<std::string> routeText;                // Text file router table
   DB db;                                             // Connect to the database
   //
   //*****************************************************************************
   /// \brief Route a file for processing
   //*****************************************************************************
   //
   void Route_MsgFileName(const std::string& filePath) {
      // Route HTML files
      if (filePath.find(".html") != filePath.npos) {
         boost::filesystem::path fullPath(Performer::BaseLocalFolder()/filePath);  // Make the full path
         RowBillTable row(fullPath.string());                     // Create RowBillTable from bill file header information
         db.InsertBillRow(row,Performer::TableName());            // Add the row to the database

         // Route the HTML file to its recepients
         BOOST_FOREACH(const std::string& queue, routeHTML) {
            billRouter->Send(MsgFileName, filePath, queue);
         }

      // Route text files
      } else if (filePath.find(".txt") != filePath.npos) {
         BOOST_FOREACH(const std::string& queue, routeText) {
            billRouter->Send(MsgFileName, filePath, queue);
         }
      }
   }
   //
   //*****************************************************************************
   /// \brief Handle end-of-queue for the HTML case
   //*****************************************************************************
   //
   void HandleEndOfHtmlQueue() {
      BOOST_FOREACH(const std::string& queue, routeHTML) {
         billRouter->Send(MsgLastInSequence, "End of queue", queue);
      }
   }
   //
   //*****************************************************************************
   /// \brief Handle end-of-queue for the text case
   //*****************************************************************************
   //
   void HandleEndOfTextQueue() {
      BOOST_FOREACH(const std::string& queue, routeText) {
         billRouter->Send(MsgLastInSequence, "End of queue", queue);
      }
      // Text files are processed last, after HTML files.
      // Tell RingMaster that BillRouter has processed all messages received from Synchronize
      billRouter->LogThis("BillRouter sending completion message");
      billRouter->Send(MsgCountCompletedProcess,"BillRouter completed",Name_RingMasterQueue);
   }
   //
   //*****************************************************************************
   /// \brief Route messages based on the message type.
   //*****************************************************************************
   //
   void Handler_MessageRouting(MessageType type, const std::string& message, bool&) {
      // Remove leading message type identifier (isolate the file path).
      const std::string filePath = Performer::RemoveTypeIdentifierFromMessage(message);
      
      // Ignore files that aren't bill text
      const std::string filename = boost::filesystem::path(filePath).stem().string(); // e.g., "ab_1_bill_20110114_amended_asm_v98"
      const std::string ignoreThese[] = { "_vote_", "_vt_", "_status", "_history" };
      BOOST_FOREACH (const std::string& fragment, ignoreThese) if (filename.find(fragment) != filename.npos) return;

      switch (type) {
         case MsgFileName:
            Route_MsgFileName(filePath);
            break;
         case MsgLastInSequence:       // Relay end-of-queue to performers on the HTML routing table
            HandleEndOfHtmlQueue();
            break;
         case MsgLastInSequenceText:   // Relay end-of-queue to performers on the text routing table
            HandleEndOfTextQueue();
            break;
         default:
            billRouter->LogThis("BillRouterMain::Handler_MessageRouting received unknown type " + type);
      }
   }
   //
   //*****************************************************************************
   /// \brief Handle Shutdown command from RingMaster.
   //*****************************************************************************
   //
   void Handler_Shutdown(MessageType, const std::string&, bool& setToExitProcess) {
      billRouter->LogThis("BillRouter received shutdown message");
      setToExitProcess = true;
   }
   //
   //*****************************************************************************
   /// \brief Iterate over the local bills, sending .HTML files for processing.
   /// This is used in testing (case StandAlone).
   //*****************************************************************************
   //
   void SendInterestingBills(const std::string& rootFolder) {
      boost::filesystem::path p(rootFolder);
      boost::filesystem::directory_iterator end;
      for (boost::filesystem::directory_iterator itr(rootFolder); itr != end; ++itr) {
         if (boost::filesystem::is_directory(itr->status())) SendInterestingBills(itr->path().string());
         else if (boost::filesystem::is_regular_file(itr->status())) {
            std::string filePath = itr->path().string();
            std::string s1 = boost::filesystem::extension(itr->path().leaf());
            if (s1 == ".html") {
               bool b;
               size_t pub(filePath.find("pub"));
               Handler_MessageRouting(MsgFileName, filePath.substr(pub), b);
            }
         }
      }
   }
}
//
//*****************************************************************************
/// \brief BillRouter entry point
//*****************************************************************************
//
void main(int argc, char* argv[]) {
   billRouter.reset(new Performer("BillRouter",Name_BillRouterQueue));
   billRouter->CommonInitialization();

   // Prepare routing tables
   routeHTML.push_back(Name_TextConverterQueue);
   routeText.push_back(Name_BillRankerQueue);

   switch (Performer::executionType) {
      case SingleFile:                                   // Test routing a single file
         {
         bool b;
//       std::string filePath("D:/CCHR/pub/11-12/bill/asm/ab_0001-0050/ab_1_bill_20110114_amended_asm_v98.html");
         std::string message("13,pub/13-14/bill/asm/ab_0001-0050/ab_1_bill_20121203_introduced.html");
         Handler_MessageRouting(MsgFileName, message, b);
         }
         break;
      case StandAlone:                                   // Test routing all files currently on the local file system
         db.NonQuery(std::string("Delete from ") + Performer::TableName());   // Clear the datbase before starting
         std::cout << boost::posix_time::to_simple_string(boost::posix_time::second_clock::local_time()) << std::endl;
         SendInterestingBills("D:/CCHR/pub/13-14");
         std::cout << boost::posix_time::to_simple_string(boost::posix_time::second_clock::local_time()) << std::endl;
         std::cout << "The bills table contains " << db.NonQuery(std::string("Select Count(*) from ") + Performer::TableName()) << " rows." << std::endl;
         break;
      case Normal:                                       // Normal production processing
         // Prepare message dispatching
         billRouter->AddHandler(MsgFileName,          Handler_MessageRouting);
         billRouter->AddHandler(MsgLastInSequence,    Handler_MessageRouting);
         billRouter->AddHandler(MsgLastInSequenceText,Handler_MessageRouting);
         billRouter->AddHandler(MsgTypeQueueShutdown, Handler_Shutdown);

         // Handle incoming messages until told to shut down.
         billRouter->MessageDispatcher();
         std::cout << "BillRouter is completing its run." << std::endl;
         break;
      }
   // Shutdown and exit
   billRouter->CommonShutdown();
}
