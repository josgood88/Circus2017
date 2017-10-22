#include "HTTP.h"
#include <Poco/Net/HTTPClientSession.h>

HTTP::HTTP(const std::string& url) : session(Poco::Net::HTTPClientSession(url)) {}
