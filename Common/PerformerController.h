#ifndef performer_tester_h
#define performer_tester_h

#include <MessageTypes.h>
#include <Performer.h>

#include <Poco/Process.h>
#include <boost/optional.hpp>

class PerformerController : public Performer {
public:
   PerformerController(const std::string& name_, const std::string& queueName_);
   ~PerformerController();
   boost::optional<Poco::ProcessHandle> StartProcess(const std::string& path);
   bool EndProcess(Poco::ProcessHandle);
   bool SingleMessageDispatcher(MessageType type, MessageHandler handler, unsigned int waitThisManySeconds);
   boost::optional<std::string> MessageCheck(MessageType expected, boost::optional<MessageType> actual);
   boost::optional<std::string> MessageCheck(std::string expected, std::string actual);

   struct PerformerIdentifier {
      std::string exeName;                                  // Name (not path) of executable file
      std::string qeueueName;                               // Name of the executable's input message queue
      PerformerIdentifier(const std::string& exe, const std::string& q) : exeName(exe), qeueueName(q) { }
   };

   struct PerformerAttributes {
      boost::optional<Poco::ProcessHandle> ph;              // Process handle
      PerformerAttributes() : ph(boost::optional<Poco::ProcessHandle>()) { }
      PerformerAttributes( boost::optional<Poco::ProcessHandle> ph_) : ph(ph_) { }
   };
};

#endif
