#ifndef CircusMessageNS_h
#define CircusMessageNS_h

#include <MessageTypes.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <sstream>
#include <boost/cstdint.hpp>

class CircusMessageNS {
///todo CircusMessageNS serialization methods need to be thread safe
public:
   std::string Serialize(MessageType type) { 
      std::stringstream result;
      result << type;
      return result.str();
   }
   
   MessageType Serialize(std::string serialized) { 
      std::stringstream ss;
      ss << serialized;
      int result;
      ss >> result;
      return (MessageType)result; 
   }
//
//   std::string Serialize(MessageType type, const std::string& message) {
//      std::stringstream ss;
//      ss << Serialize(type) << "," << message;
//      return ss.str();
//   }
//
//   std::string SerializeRegistrationRequest(const std::string& performer, const std::string& messageQueueName) {
//      std::stringstream ss;
//      ss << Broker_RegisterPerformer << ','  << performer << ',' << messageQueueName;
//      return ss.str();
//   }
//
//   std::string SerializePerformerLookupRequest(const std::string& performer, const std::string& whichPerformerIsAsking) {
//      std::stringstream ss;
//      ss << Broker_LookupPerformer << ','  << performer << ',' << whichPerformerIsAsking;
//      return ss.str();
//   }
//   ///todo Rationalize the messaging -- CircusMessage.h.  Need a standard packet that goes across the wire.
//   std::string SerializeNotCircusMessage     (const std::string& message)   { return Serialize(MsgTypeNotCircusMessage,      message); }
//   std::string SerializeQueueShutdown        (const std::string& message)   { return Serialize(MsgTypeQueueShutdown,         message); }
//   std::string SerializeLogMessage           (const std::string& message)   { return Serialize(MsgTypeLogMessage,            message); }
//   std::string SerializeError                (const std::string& message)   { return Serialize(MsgTypeError,                 message); }
//   std::string SerializePerformerStarted     (const std::string& message)   { return Serialize(MsgTypePerformerStarted,      message); }
//   std::string SerializePerformerFinished    (const std::string& message)   { return Serialize(MsgTypePerformerFinished,     message); }
//   std::string SerializePerformerRegistration(const std::string& message)   { return Serialize(Broker_RegisterPerformer, message); }
//   std::string SerializeRequestPerformerQueue(const std::string& message)   { return Serialize(Broker_LookupPerformer, message); }
//   std::string SerializeSynchronize_Ready    (const std::string& responseQ) { return Serialize(Synchronize_Ready,            responseQ); }
//   std::string SerializeSynchronize_Complete (const std::string& responseQ) { return Serialize(Synchronize_Complete,         responseQ); }
//   std::string SerializeBillRouter_File_Data (const std::string& message)   { return Serialize(BillRouter_File_Data,         message); }
};

#endif
