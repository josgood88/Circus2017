#ifndef TextConverterUtilities_h
#define TextConverterUtilities_h

#include <string>
#include <vector>

namespace TextManipulation {
   std::vector<std::string> Extract  (const std::vector<std::string>& input, const std::string matchUnknownCase);
   std::string              LowerCase(const std::string& input);
   std::vector<std::string> ReadFile (const std::string& fileName);
   void                     WriteFile(const std::string& filePath, const             std::string & fileContents);
   void                     WriteFile(const std::string& filePath, const std::vector<std::string>& fileContents);
}

#endif
