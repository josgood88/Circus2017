#include <BillRanker.h>
#include <CAPublic.h>
#include <CommonTypes.h>
#include "Configuration.h"
#include "ConfigurationFilePath.h"
#include "Logger.h"
#include "ScopedElapsedTime.h"
#include "Utility.h"

#include <boost/filesystem/path.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/optional.hpp>
#include "boost/program_options.hpp"
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

#define RecordBillRowTablesForInspection false

namespace fs = boost::filesystem;
namespace {
	const std::string raw_lob_files_folder("D:/CCHR/2017-2018/LatestDownload/Bills");
	const std::string cache_file_location("../Results/capublic.db");
	bool import_leg_data(true);                  // If false, don't import leg site data into database
	int bill_processing_limit(0);
	bool limit_bill_processing(false);
	int bill_processing_counter(0);
	bool process_all_bills(false);
   std::string process_single_bill;
}

#if RecordBillRowTablesForInspection 
void PrintBillRows(const std::string& title,const std::vector<BillRow>& input) {
   std::ofstream of(std::string("D:/CCHR/Projects/Circus2017/Logs/") + title + ".txt",std::ofstream::trunc);
   LoggerNS::Logger::Log("=====================================================");
   LoggerNS::Logger::Log(title);
   const std::string header("Type\tNum\tNeg\tPos\tLob\t\tBillID\t\tVersionID\tAuthor\tTitle");
   LoggerNS::Logger::Log(header);
   of << header << std::endl;
   std::for_each(input.begin(),input.end(),[&](const BillRow& row) {
      std::stringstream ss;
      ss << row.measure_type     << "\t"
         << row.measure_num      << "\t"
         << row.neg_score        << "\t"
         << row.pos_score        << "\t"
         << row.lob              << "\t"
         << row.bill_id          << "\t"
         << row.bill_version_id  << "\t"
         << row.author           << "\t"
         << row.title;
      LoggerNS::Logger::Log(ss.str());
      of << ss.str() << std::endl;
   });
}
#endif 

namespace {
   boost::scoped_ptr<Configuration> config(new Configuration(path_config_file));
   const size_t SUCCESS(0);
   const size_t ERROR_IN_COMMAND_LINE(1);

   int ParseCommandLine(int argc,char** argv) {
      // http://www.radmangames.com/programming/how-to-use-boost-program_options
      namespace po = boost::program_options;
      po::options_description desc("Options");
      desc.add_options()
         ("help,h","Help message")
         ("all,a",po::value<bool>(&process_all_bills),"Process all bills")                               // "--all true"    causes all bills to be freshly evaluated
         ("bill,b",po::value<std::string>(&process_single_bill),"Process single bill")                   // "--bill AB123"  causes AB 123 (only) to be freshly evaluated
         ("import,i",po::value<bool>(&import_leg_data),"Whether to import leg data")                     // "--import true" causes data to be imported
         ("limit,l",po::value<int>(&bill_processing_limit),"Limit bills processed");                     // "--limit 5"     limits to 5 bills processed
      po::variables_map vm;
      try {
         po::store(po::parse_command_line(argc,argv,desc),vm);       // throws on error
         if (vm.count("help")) {
            std::cout << "Usage: [-import]" << std::endl << std::endl;
         }
         po::notify(vm);                                                // throws on error, so do after help
      } catch (boost::program_options::required_option& e) {
         std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
         return ERROR_IN_COMMAND_LINE;
      } catch (boost::program_options::error& e) {
         std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
         return ERROR_IN_COMMAND_LINE;
      }

      // Some settings have cascading effects
      if (bill_processing_limit > 0) limit_bill_processing = true;
      bill_processing_counter = bill_processing_limit;

      if (process_single_bill.length() > 0) {
         process_all_bills = import_leg_data = limit_bill_processing = false;
      }

      return SUCCESS;
   }

   bool IsBillProcessingEnabled() {
      return !limit_bill_processing || (limit_bill_processing && bill_processing_counter > 0) || (process_single_bill.length() > 0);
   }

   // std::vector<BillRow> bills contains all versions of all bills currently before the legislature.
   // Remove all but the most current version
   std::vector<BillRow> RemoveObsoleteVersions(const std::vector<BillRow>& all_versions_of_all_bills) {
      // Sort bill versions to simplify extracting latest version.  Instances of the same bill are grouped together, with the latest listed first.
      std::vector<BillRow> mutuable_all_versions(all_versions_of_all_bills);  // Sorting requires non-const
      std::sort(mutuable_all_versions.begin(),mutuable_all_versions.end());
      // Copy only the latest version of each bill
      std::vector<BillRow> latest_version_of_each_bill;
      std::unique_copy(mutuable_all_versions.begin(),mutuable_all_versions.end(),std::back_inserter(latest_version_of_each_bill),[&](const BillRow& lhs,const BillRow& rhs) {
         return lhs.bill_id == rhs.bill_id;
      });
      return latest_version_of_each_bill;
   }
}

std::vector<BillRow> SelectAllVersionsOfAllBills(CAPublic& db) {
   ScopedElapsedTime elapsed("Selecting all versions of all bills","All versions of all bills selected: ");
   return db.ReadVersionTable();
}

std::vector<BillRow> SelectMostRecentVersionOfEachBill(const std::vector<BillRow>& all_versions) {
   ScopedElapsedTime elapsed_time_selection("Selecting most recent version of all bills","Most recent version of all bills selected: ");
   return RemoveObsoleteVersions(all_versions);
}

std::vector<BillRow> SelectUnevaluatedBills(std::vector<BillRow>& completed_bill_versions, std::vector<BillRow>& most_recent_versions) {
   ScopedElapsedTime elapsed_time_selection("Selecting all unevaluated bill versions","Unevaluated bills selected: ");
   std::vector<BillRow> result;
   std::sort(completed_bill_versions.begin(),completed_bill_versions.end());
   auto itr = std::set_difference(most_recent_versions.begin(),most_recent_versions.end(),
      completed_bill_versions.begin(),completed_bill_versions.end(),
      std::back_inserter(result));
   return result;
}

int main(int argc,char** argv) {
   ScopedElapsedTime elapsed_time("Starting Circus","Circus Run Time: ");
   ParseCommandLine(argc,argv);        // Extract command line arguments

   // Constructor handles importing leg site data files into database.
   // If 'import_leg_data' is false, then the current database contents are used.
   CAPublic db(import_leg_data);

   if (IsBillProcessingEnabled()) {
      std::vector<BillRow> bills_to_process,all_bill_versions,unevaluated_bill_versions,completed_bill_versions;
      // TODO: Ensure bill_tbl is updated also, or else extend BillVersions to include location.  BR needs location data.
      std::stringstream ss;
      if (process_single_bill.length() > 0) {
         const std::string measure_type = ExtractHouseFromBillID(process_single_bill);
         const std::string measure_num = ExtractBillNumberFromBillID(process_single_bill);
         ss << "Processing " << measure_type << " " << measure_num << " only.";
         LoggerNS::Logger::Log(ss.str());
         bills_to_process.push_back(db.ReadSingleBillRow(measure_type,measure_num));
      } else {
         if (process_all_bills) {
            {  ScopedElapsedTime elapsed_time("Reading contents of BillRows","Contents read: ");
               bills_to_process = db.ReadBillRows();
            }
            ss << "Processing all " << bills_to_process.size() << " bills currently in the database.";
            LoggerNS::Logger::Log(ss.str());
         } else {
            all_bill_versions = SelectAllVersionsOfAllBills(db);
            unevaluated_bill_versions = SelectMostRecentVersionOfEachBill(all_bill_versions);
            bills_to_process = SelectUnevaluatedBills(completed_bill_versions,unevaluated_bill_versions);
            ss << bills_to_process.size() << " bills have changed since the last run.";
            LoggerNS::Logger::Log(ss.str());
         }
      }

   #if RecordBillRowTablesForInspection 
      PrintBillRows("bills_to_process",bills_to_process);
      PrintBillRows("completed_bills",completed_bill_versions);
      PrintBillRows("unevaluated_bills",unevaluated_bill_versions);
   #endif

      // Rank those bills that have changed
      BillRanker::GenerateBillRankings(bills_to_process,db,config->Password(), config->Negative());
      }
   return 0;
   }
