#ifndef HistoryCleanup_h
#define HistoryCleanup_h

#include <boost/filesystem/path.hpp>

namespace HistoryCleanup {
   void Perform(boost::filesystem::path basePath);
}

#endif
