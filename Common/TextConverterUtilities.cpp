#include <Performer.h>
#include "TextConverterUtilities.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
//
//*****************************************************************************
/// \brief Answer whether a file is one that we'll want a text copy of.
/// \brief str -- file name
/// \return True if a text copy is wanted
//*****************************************************************************
//
bool TextConverterUtilities::IsBillFile(const std::string& str) { 
   bool result(false);
   result = std::string::npos != str.find("_bill_");                    // Want text copy of actual bills
   if (result) result = std::string::npos == str.find("_history");      // ..not bill histories
   if (result) result = std::string::npos == str.find("_status");       // ..not bill status
   return result;
}
//
//*****************************************************************************
/// \brief Trim HTML formatting from a file.
/// \brief fileName -- convert this file
/// \return std::string containing the trimmed text
//*****************************************************************************
//
std::string TextConverterUtilities::Replace(const std::string& input, const std::string& rx, const std::string& fmt) {
   return std::regex_replace(input,std::regex(rx),fmt); 
}

std::string TextConverterUtilities::TrimText(const std::string& inputText, const std::string& txtFilePath) {
   bool haveStrike(false), haveEm(false);
   std::string s0(inputText);   
   std::replace(s0.begin(),s0.end(),'\n','?');                          // std::regex doesn't have multiline switch
   if (s0.find("<strike>") != s0.npos) {                                // If the text contains strikeout sections
      haveStrike = true;
      s0 = Replace(s0," ?<strike>.*?</strike> ?"," ");                  // ..Remove strikeout sections
   }
   if (s0.find("<em>") != s0.npos) {                                    // If the text contains addition sections
      haveEm = true;
      s0 = Replace(s0," ?(<em>) ?(.*?) ?(</em>) ?","$1$2$3");           // ..Make canonical form
      s0 = Replace(s0,"<em>(.*?)</em>"," $1 ");                         // ..Remove <em></em>
   }
   if (haveStrike || haveEm) {
      s0 = Replace(s0,"\\? ","\?");                                     // ..Remove space at start of line
   }
   if (haveEm) {
      s0 = Replace(s0," \\?","\?");                                     // ..Remove space at end of line
   }
   std::replace(s0.begin(),s0.end(),'?','\n');                          // Restore newlines
   // For some reason the string needs to be written here.  Writing it in TrimAndStore
   // just doesn't work -- the file is truncated.
   if (txtFilePath.length() > 0) {
      std::ofstream os(txtFilePath);  
      os << s0;
   }
   return s0;
}

std::string TextConverterUtilities::Trim(const std::string& fileName, const std::string& txtFilePath) {
   std::ifstream is(fileName);
   std::stringstream ss;
   ss << is.rdbuf();
   return TrimText(ss.str(), txtFilePath);
}
//
//*****************************************************************************
/// \brief Transform HTML file path to text file path.
/// \brief fileName -- HTML file path
/// \return text file path
//*****************************************************************************
//
std::string TextConverterUtilities::TransformPath(const std::string HTML_FileName) {
   const boost::filesystem::path inputPath(HTML_FileName);
   const boost::filesystem::path filePath(inputPath.parent_path());
   const boost::filesystem::path fileName(inputPath.stem());
   const boost::filesystem::path result(Performer::BaseLocalFolder()/filePath/"text"/fileName);
   return result.string() + ".txt";
}
//
//*****************************************************************************
/// \brief Ensure text file folder exists.
/// \brief fileName -- Text file path
//*****************************************************************************
//
bool TextConverterUtilities::EnsureTextFolderExists(const std::string& text_FileName) {
   boost::filesystem::path inputPath(text_FileName);
   boost::filesystem::path folderPath(inputPath.parent_path());
   if (!boost::filesystem::exists(folderPath)) {
      if (!boost::filesystem::create_directory(folderPath)) {
         /// \todo: Convert std::cout << "Failed to create " to an exception
         std::cout << "Failed to create " << folderPath.string() << std::endl;
         return false;
      }
   }
   return true;
}
//
//*****************************************************************************
/// This version is called by BillRankerMain testing
/// \brief Remove HTML formatting and store the result.
/// \brief fileName -- convert this file
//*****************************************************************************
//
std::string TextConverterUtilities::TrimAndStore(const std::string& fileName) {
   std::string result;
   if (IsBillFile(fileName)) {
      const std::regex rx("\\w:/\\w+[\\\\/]");
      const std::string fmt("");
      const std::string htmlFilePath(std::regex_replace(fileName,rx,fmt).c_str());
      const std::string txtFilePath = TransformPath(htmlFilePath); // Path for text file derived from HTML files
      if (EnsureTextFolderExists(txtFilePath)) {                   // Create the text file's folder
         std::ofstream os(txtFilePath);  
         os << Trim(htmlFilePath, txtFilePath);                    // Read the file, handle strikeouts/insertions, write resulting text
         result = txtFilePath;
      }
   }
   return result;
}
//
//*****************************************************************************
/// \brief Remove HTML formatting and store the result.
/// \brief fileName -- convert this file
//*****************************************************************************
//
bool TextConverterUtilities::TrimAndStore(const std::string& fileName, Performer& performer) {
   bool result = false;
   if (IsBillFile(fileName)) {
      std::string htmlFilePath = boost::filesystem::path(Performer::BaseLocalFolder()/fileName).string();
      std::string txtFilePath = TransformPath(fileName);    // Path for text file derived from HTML files
      if (EnsureTextFolderExists(txtFilePath)) {            // Create the text file's folder
         std::ofstream os(txtFilePath);  
         os << Trim(htmlFilePath, txtFilePath);             // Read the file, handle strikeouts/insertions, write resulting text
         result = true;
      } else {
         std::stringstream ss;
         ss << "TextConverter was unable to create " << txtFilePath;
         performer.LogThis(ss.str());
      }
   } else {
      std::stringstream ss;
      ss << "TextConverter ignored " << fileName << " because it is not a bill";
      performer.LogThis(ss.str());
   }
   return result;
}
