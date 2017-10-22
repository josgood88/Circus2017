#pragma once
#include <CommonTypes.h>

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <vector>

namespace fs = boost::filesystem;

namespace {
   std::string ReadFile(const std::string& fileName) {
      std::ifstream ifs(fileName);
      std::stringstream ss;
      ss << ifs.rdbuf();
      return ss.str();
   }

   std::vector<std::string> ReadFileLineByLine(const std::string& fileName) {
      std::vector<std::string> result;
      std::ifstream ifs(fileName);
      std::string line;
      while (std::getline(ifs,line)) { result.push_back(line); }
	  ifs.close();
      return result;
   }

   void WriteFile(const std::string& filePath, const std::string& fileContents) {
      std::ofstream ofs(filePath);
      ofs << fileContents;
   }

   void WriteFileLineByLine(const std::string& filePath,const std::vector<std::string>& fileContents) {
      std::ofstream ofs(filePath,std::ofstream::trunc);
      if (ofs.is_open()) {
         std::for_each(fileContents.begin(),fileContents.end(),[ & ](const std::string& line) {
            ofs << line;
         });
      }
   }

   // Obtain contents of a folder
   std::vector<fs::path> FolderContents(const std::string& folder) {
      std::vector<fs::path> result;
      for (fs::directory_iterator itr(folder); itr != fs::directory_iterator(); ++itr) {
          if (fs::is_regular_file(itr->status())) result.push_back(itr->path());
      }      
   return result;
   }

   // Extract house from file path
   std::string ExtractHouseFromBillID(const fs::path& bill) {
      const std::regex rx("\\b([A-Z]+).*");
      const std::string house(std::regex_replace(bill.filename().string(),rx,std::string("$1")));
      return house;
   }
   // Extract bill number from file path
   std::string ExtractBillNumberFromBillID(const fs::path& bill) {
      const std::regex rx("\\b[^\\d]+(\\d+).*");
      const std::string number(std::regex_replace(bill.filename().string(),rx,std::string("$1")));
      return number;
   }

   // Find where last left off importing lob files, so as to start processing the next one.
   // Revisions to bills occur most weeks.  Want to import data only for the new revisions.
   unsigned int lob_number(const fs::path& file_path) {
      fs::path as_path(file_path);
      auto file_name(as_path.filename().string());             // Trim extension
      auto first(file_name.find_first_of("0123456789"));       // File name ends with digits
      if (first != std::string::npos) {
         auto substr(file_name.substr(first));                 // Extract those digits
         std::stringstream ss;
         ss << substr;
         unsigned int result;
         ss >> result;
         return result;
      } else {
         return 0;
      }
   }

   // Does String contain a String?
   bool Contains(const std::string& enclosing, const std::string& enclosed) {
      return enclosing.find((enclosed)) != std::string::npos;
   }

   std::map<unsigned char, char> translation_table {
      { 0x80, 'A' }, { 0x81, 'A' }, { 0x82, 'A' }, { 0x83, 'A' }, { 0x84, 'A' }, { 0x85, 'A' }, 
      { 0x87, 'C' }, 
      { 0x88, 'E' }, { 0x89, 'E' }, { 0x8a, 'E' }, { 0x8b, 'E' }, { 0x8c, 'I' }, { 0x8d, 'I' }, 
      { 0x8e, 'I' }, { 0x8f, 'I' }, 
      { 0x91, 'N' }, 
      { 0x92, 'O' }, { 0x93, 'O' }, { 0x94, 'O' }, { 0x95, 'O' }, { 0x96, 'O' }, { 0x98, 'O' }, 
      { 0x99, 'U' }, { 0x9a, 'U' }, { 0x9b, 'U' }, { 0x9c, 'U' }, 
      { 0x9d, 'Y' }, 
      { 0x9f, 'S' }, 
      { 0xa0, 'a' }, { 0xa1, 'a' }, { 0xa2, 'a' }, { 0xa3, 'a' }, { 0xa4, 'a' }, { 0xa5, 'a' }, 
      { 0xa7, 'c' }, 
      { 0xa8, 'e' }, { 0xa9, 'e' }, { 0xaa, 'e' }, { 0xab, 'e' }, 
      { 0xac, 'i' }, { 0xad, 'i' }, { 0xae, 'i' }, { 0xaf, 'i' }, 
      { 0xb1, 'n' }, 
      { 0xb2, 'o' }, { 0xb3, 'o' }, { 0xb4, 'o' }, { 0xb5, 'o' }, { 0xb6, 'o' }, { 0xb8, 'o' }, 
      { 0xb9, 'u' }, { 0xba, 'u' }, { 0xbb, 'u' }, { 0xbc, 'u' }, 
      { 0xbd, 'y' }, { 0xbf, 'y'	}
   };
   
   // Translate from UTF-8
   void UtilityTranslateFromUTF8(std::string& input) {
      const unsigned char UTF8_marker(0xc3);
      unsigned int index;
      do {
         index = input.find(UTF8_marker);                      // Do nothing if no UTF-8
         if (index != std::string::npos) {                     // C-style because iterators can be invalidated
            for (int i = index; input[i] != 0; ++i) {          // ...Shift string left one character
               input[i] = input[i+1];
            }
            input[index] = translation_table[input[index]];    // Translate UTF-8 character to ASCII character
         }
      } while (index != std::string::npos);
   }

   // Remove HTML from a string
   std::string UtilityRemoveHTML(std::string& input) {
      // Remove HTML
      std::string s1(std::regex_replace(input,std::regex("</?caml(.*?)>"),       std::string()));
      std::string s2(std::regex_replace(s1,std::regex("<em>(.*?)</em>"),         std::string(" $1 ")));
      std::string s3(std::regex_replace(s2,std::regex("<strike>(.*?)</strike>"), std::string()));
      std::string s4(std::regex_replace(s3,std::regex("<p>(.*?)</p>"),           std::string(" $1 ")));
      std::string s5(std::regex_replace(s4,std::regex("<span class=.EnSpace./>"),std::string()));        // std::string() instead of ""
      input = s5;

      //Translate from UTF-8
      UtilityTranslateFromUTF8(input);

      // Remove everything less than space.
      std::for_each(input.begin(),input.end(), [](char& c) { if (c < ' ') c = ' '; });
      return input;
   }
}
