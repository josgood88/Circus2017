#include "Configuration.h"

#include <sstream>
#include <stdexcept>
#include <string>

#include <boost/filesystem/operations.hpp>                        // includes boost/filesystem/path.hpp
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/property_tree/xml_parser.hpp>

Configuration::Configuration() {
}

Configuration::Configuration(const std::string& path) {
   if (boost::filesystem::exists(path)) {                         // If the configuration file exists
      boost::property_tree::xml_parser::read_xml(path, pt);       // ..read it into the property tree
   } else {
      std::stringstream msg;
      msg << "Configuration file " << path << " doesn't exist.";
      throw std::exception(msg.str().c_str());
   }
}

boost::optional<std::string> Configuration::ReadValue(const std::string& key) {
   boost::property_tree::ptree::const_iterator end = pt.end();
   for (boost::property_tree::ptree::const_iterator it = pt.begin(); it != end; it++) {
      std::stringstream msg;
      msg << "Key: " << it->first << ", Value: "; // << it->second;
      std::string a(msg.str());
      std::string b(msg.str());
   }
   return boost::optional<std::string>();
}

