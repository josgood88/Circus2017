#ifndef HTTP_h
#define HTTP_h

#ifdef HTTP_EXPORTS  
#define HTTP_API __declspec(dllexport)   
#else  
#define HTTP_API __declspec(dllimport)   
#endif 

#include <string>
#include <Poco/Net/HTTPClientSession.h>

//
//*****************************************************************************
/// \brief HTTP encapsulates communications with the California Legislature website.
//*****************************************************************************
//

class HTTP {
public:
   HTTP_API HTTP(const std::string& url);
   HTTP_API ~HTTP() {}
   bool Connect(const std::string& url);
private:
   Poco::Net::HTTPClientSession session;
};

#endif
