// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the Configuration_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// Configuration_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#ifdef Configuration_EXPORTS
#define Configuration_API __declspec(dllexport)
#else
#define Configuration_API __declspec(dllimport)
#endif

#include <string> 

class Configuration_API Configuration {
public:
   explicit Configuration();
   Configuration(const std::string& path);
   const std::string Biennium();
   const std::string BillsFolder();
   const std::string Negative();
   const std::string Password();
   const std::string Positive();
   const std::string ResultsFolder();
   const std::string Site();
   const std::string User();
   const std::string Version();
};
