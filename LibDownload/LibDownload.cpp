#include "Poco/URIStreamOpener.h"
#include "Poco/StreamCopier.h"
#include "Poco/Path.h"
#include "Poco/URI.h"
#include "Poco/Exception.h"
#include "Poco/Net/HTTPStreamFactory.h"
#include "Poco/Net/FTPStreamFactory.h"
#include <memory>
#include <iostream>

using Poco::URIStreamOpener;
using Poco::StreamCopier;
using Poco::Path;
using Poco::URI;
using Poco::Exception;
using Poco::Net::HTTPStreamFactory;
using Poco::Net::FTPStreamFactory;

int main(int argc, char** argv)
{
  HTTPStreamFactory::registerFactory(); // Must register the HTTP factory to stream using HTTP
  FTPStreamFactory::registerFactory(); // Must register the FTP factory to stream using FTP

  string url = "http://somefile.mp3";
  string filePath = "C:\\somefile.mp3";

  // Create and open a file stream
  std::ofstream fileStream;
  fileStream.open(filePath, ios::out | ios::trunc | ios::binary);

  // Create the URI from the URL to the file.
  URI uri(url);

  // Open the stream and copy the data to the file. 
  std::auto_ptr<std::istream> pStr(URIStreamOpener::defaultOpener().open(uri));
  StreamCopier::copyStream(*pStr.get(), fileStream);

  fileStream.close();
}
// MathFuncsLib.cpp
// compile with: cl /c /EHsc MathFuncsLib.cpp
// post-build command: lib MathFuncsLib.obj

#include "LibDownload.h"

#include <stdexcept>
#include <string>

namespace {
   static std::string source_url("http://downloads.leginfo.legislature.ca.gov/");
}

namespace LibDownload {
   void Impl::FetchFolder(std::string target_folder) {
   }
}