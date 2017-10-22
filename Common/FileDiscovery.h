// Facilities for discovering files, e.g. most current status files for all bills before the legislature

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <vector>

namespace FileDiscovery {
   std::vector<fs::path> AllCurrentBillFiles  (const fs::path& pub_root);
   std::vector<fs::path> AllCurrentStatusFiles(const fs::path& pub_root);
   bool IsSameBillNumber(const fs::path& path1, const fs::path& path2);
}
