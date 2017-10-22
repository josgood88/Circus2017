#ifndef TextConverterUtilities_h
#define TextConverterUtilities_h

#include <Performer.h>

#include <regex>
#include <string>

namespace TextConverterUtilities {
   bool        EnsureTextFolderExists(const std::string& text_FileName);
   bool        IsBillFile            (const std::string& str);
   std::string Replace               (const std::string& input, const std::string& rx, const std::string& fmt);
   std::string TrimAndStore          (const std::string& fileName);
   bool        TrimAndStore          (const std::string& fileName, Performer&);
   std::string TransformPath         (const std::string HTML_FileName);
   std::string Trim                  (const std::string& fileName, const std::string& txtFilePath);
   std::string TrimText              (const std::string& s0, const std::string& txtFilePath = std::string());
}

#endif
