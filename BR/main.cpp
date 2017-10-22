#include <CAPublic.h>
#include "Configuration.h"
#include "ConfigurationFilePath.h"
#include "CreateBillReport.h"
#include "Logger.h"
#include "ScopedElapsedTime.h"
#include "Update.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include "boost/program_options.hpp"
#include <boost/scoped_ptr.hpp>

namespace fs = boost::filesystem;

namespace {
   const std::string html_files_folder("D:/CCHR/2017-2018/Html");
   boost::scoped_ptr<Configuration> config(new Configuration(path_config_file));
   const size_t SUCCESS(0);
   const size_t ERROR_IN_COMMAND_LINE(1);
   std::string str_bill_id;                     // Bill ID as a string, e.g. "AB12"
   bool is_verbose(false);
   bool force_report_regeneration(false);
   bool update_all(false);

   int ParseCommandLine(int argc, char** argv) {
      // http://www.radmangames.com/programming/how-to-use-boost-program_options
      namespace po = boost::program_options; 
      po::options_description desc("Options"); 
      desc.add_options() 
         ("help,h",                                          "Help message")
         ("verbose,v", po::value<bool>(&is_verbose),         "Verbose output")            // "--v true" turns verbose reporting on
         ("bill,b",    po::value<std::string>(&str_bill_id), "generate bill report")      // "--bill AB12" generates report for AB 12, regardless of whether it needs to be updated
         ("update,u",  po::value<bool>(&update_all),         "update all reports");       // "--update true" re-generates all reports
      po::variables_map vm;
      try { 
         po::store(po::parse_command_line(argc, argv, desc), vm);       // throws on error
         if (vm.count("help")) { 
            std::stringstream ss;
            ss << "Usage: [--bill bill_ID, e.g., AB12]" << std::endl << std::endl; 
            LoggerNS::Logger::Log(ss.str());
         } 
         po::notify(vm);                                                // throws on error, so do after help
      } catch(po::required_option& e) { 
         std::stringstream ss;
         ss << "ERROR: " << e.what() << std::endl << std::endl; 
         LoggerNS::Logger::Log(ss.str());
         return ERROR_IN_COMMAND_LINE; 
      } catch(po::error& e) { 
         std::stringstream ss;
         ss << "ERROR: " << e.what() << std::endl << std::endl; 
         LoggerNS::Logger::Log(ss.str());
         return ERROR_IN_COMMAND_LINE; 
      } 

      // Some settings have cascading effects
      if (str_bill_id.length() > 0) force_report_regeneration = true;      // "--bill NNNN" regenerates report, regardless of whether it needs to be updated
      if (update_all) force_report_regeneration = true;                    // "--update true" regenerates report, regardless of whether it needs to be updated
      return SUCCESS; 
   }

   std::string BillPath(const std::string& bill) {
      std::stringstream s;
      s << html_files_folder << "/" << bill << ".html";
      return s.str();
   }

   bool HtmlReportExists(const std::string& bill) {
      return fs::exists(BillPath(bill));
   }
}

int main(int argc, char** argv) {
   LoggerNS::Logger::LogFileLocation("D:/CCHR/Projects/Circus2017/Logs/BR_LogFile.txt");
   ScopedElapsedTime elapsed_time("Starting BR","BR Run Time: ");
   ParseCommandLine(argc,argv);        // Extract command line arguments
   CAPublic db(false);                 // Do not import leg data.  That has already been done.

   // Update all reports, regardless of whether they need it
   if (update_all) { Update::UpdateAllReports(db,fs::path(html_files_folder)); }

   // Update a report if the report already exists
   else if ((str_bill_id.length() > 0) && HtmlReportExists((str_bill_id))) { Update::UpdateSingleReport(db,fs::path(BillPath(str_bill_id)),force_report_regeneration); }

   // Create a new report
   else if ((str_bill_id.length() > 0) && !HtmlReportExists((str_bill_id))) { CreateBillReport::Create(db,str_bill_id,html_files_folder,is_verbose); }
   
   // Update the entire Html folder (str_bill_id is empty)
   else Update::UpdateHtmlFolder(db,html_files_folder);
}
