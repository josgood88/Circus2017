// Configuration.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#define Configuration_EXPORTS
#include "Configuration.h"

#include <string>

#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>


namespace {
   std::string version;
   std::string user;
   std::string password;
   std::string site;
   std::string negative;
   std::string positive;
   std::string biennium;
   std::string bills_folder;
   std::string results_folder;
}

struct bad_pointer : std::exception { 
   const char* what() const { return "Bad Pointer"; } 
};

// Read a string value from the property tree
std::string read_value(const boost::property_tree::ptree& pt, std::string key) {
   boost::optional<std::string> result(pt.get_optional<std::string>(key));
   return result ? result.get() : std::string();
}

Configuration::Configuration(const std::string& path) {
   boost::property_tree::ptree pt;

   // Load the XML file into the property tree. If reading fails, (cannot open file, parse error), an exception is thrown.
   read_xml(path, pt);

   // Tree location is specified with a dot notation, e.g. 
   //    <Circus>
   //    <version>1</version>
   // is referenced as Circus.version
   version        = read_value(pt, "Circus.version");
   user           = read_value(pt, "Circus.leg_site.user");
   password       = read_value(pt, "Circus.leg_site.pwd");
   site           = read_value(pt, "Circus.leg_site.site");
   negative       = read_value(pt, "Circus.keyword_files.negative");
   positive       = read_value(pt, "Circus.keyword_files.positive");
   biennium       = read_value(pt, "Circus.biennium");
   bills_folder   = read_value(pt, "Circus.local_data.bills_folder");
   results_folder = read_value(pt, "Circus.local_data.results_folder");
}


const std::string Configuration::Version()       { return version.c_str();        }
const std::string Configuration::User()          { return user.c_str();           }
const std::string Configuration::Password()      { return password.c_str();       }
const std::string Configuration::Site()          { return site.c_str();           }
const std::string Configuration::Negative()      { return negative.c_str();       } 
const std::string Configuration::Positive()      { return positive.c_str();       }
const std::string Configuration::Biennium()      { return biennium.c_str();       }
const std::string Configuration::BillsFolder()   { return bills_folder.c_str();   }
const std::string Configuration::ResultsFolder() { return results_folder.c_str(); }


