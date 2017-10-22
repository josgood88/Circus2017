#include "TextManipulation.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
//
//*****************************************************************************
/// \brief Answer a lowercase copy of the input string
//*****************************************************************************
//
std::string TextManipulation::LowerCase(const std::string& input) {
   std::string lc(input);
   std::transform(lc.begin(),lc.end(),lc.begin(),::tolower);
   return lc;
}
//
//*****************************************************************************
/// \brief Extract a subset of std::vector<std::string> matching a string that std::string::find can handle
///        Comparisons are made without regard to case 
//*****************************************************************************
//
std::vector<std::string> TextManipulation::Extract(const std::vector<std::string>& input, const std::string matchUnknownCase) {
   std::vector<std::string> result;
   std::string match(LowerCase(matchUnknownCase));
   std::copy_if(input.begin(), input.end(), std::back_inserter(result), [&](const std::string& str) -> bool {
      const std::string item(LowerCase(str));
      return item.find(match) != std::string::npos; 
   });
   return result;
}
//
//*****************************************************************************
/// \brief WriteFile creates a file on the local filesystem, removing any previous copy, and writes its contents.
/// \param[in] filePath     path to the file being created
/// \param[in] fileContents contents to be written to the file
//*****************************************************************************
//
void TextManipulation::WriteFile(const std::string& filePath, const std::vector<std::string>& fileContents) {
   std::ofstream os(filePath);
   std::for_each(fileContents.begin(),fileContents.end(), [&](const std::string& str) { os << str.c_str() << std::endl; });
}
//
//*****************************************************************************
/// \brief WriteFile creates a file on the local filesystem, removing any previous copy, and writes its contents.
/// \param[in] filePath     path to the file being created
/// \param[in] fileContents contents to be written to the file
//*****************************************************************************
//
void TextManipulation::WriteFile(const std::string& filePath, const std::string& fileContents) {
   std::ofstream os(filePath);
   os << fileContents;
}
//
//*****************************************************************************
/// \brief Read a single file.
/// \brief fileName -- read this file
//*****************************************************************************
//
std::vector<std::string> TextManipulation::ReadFile(const std::string& filePath) {
   std::ifstream is(filePath);
   std::string one_line;
   std::vector<std::string> result;
   while (is.good()) {
      std::getline(is,one_line);
      if (is.good()) result.push_back(one_line);
   }
   return result;
}
