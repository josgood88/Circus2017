#ifndef LocalFileLocation_h
#define LocalFileLocation_h
#include "Database/DB.h"
#include <string>
namespace LocalFileLocation {
   std::string ExtractBillName(const std::string& input);
   std::string FilenameForBill(DB& database, const std::string& bill);
   std::string TextFolder(const std::string& bill);
}

#endif
