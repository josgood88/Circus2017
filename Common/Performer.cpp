//
/// \page Performer Performer
/// \remark Performer provides the basic functionality needed by all Circus performers.
//
///   <h1>Responsibilities</h1>
///   Performer's responsibilities are: \n
///   1) Provide some standard initialization. \n
///   2) Provide a message dispatch loop and associated data structures. \n
///   3) Wrap boost::interprocess::message_queue, managing its lifetime
///   4) Handle the details of converting a text string to and from a CircusMessage
///   5) Handle the details of sending and receiving messages
//
#pragma warning (disable:4996)      // 'std::_Copy_backward': Function call with parameters that may be unsafe
#include "Performer.h"
#include <CircusMessage.h>
#include <MessageTypes.h>
#include <QueueMap.h>
#include <QueueNames.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>
#include <boost/interprocess/sync/interprocess_recursive_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

namespace {
   CircusMessageNS circusMessageNS;
}
//
//*****************************************************************************
// Static members
//*****************************************************************************
//
const boost::filesystem::path Performer::baseLocalFolder("D:/CCHR");
std::string Performer::tableName("Bills2015Thru2016");
ExecutionType Performer::executionType(Normal);

//
//*****************************************************************************
/// \brief Construction creates the performer's queue and locates other queues.
/// \param[in] name_      - this performer's name
/// \param[in] queueName_ - this performer reads messages from this queue
//*****************************************************************************
//
Performer::Performer(const std::string& name_, const std::string& queueName_) 
         : name(name_), queueName(queueName_)
{ 
   const std::string queueNames[] = { 
      Name_BillRankerQueue, Name_BillRouterQueue,  Name_DatabaseQueue, Name_LoggerQueue,
      Name_RingMasterQueue, Name_SynchronizeQueue, Name_TextConverterQueue/*, Name_DifferenceMakerQueue*/
   };

   // RingMaster has already populated queueMap, because it creates these queues
   if (queueName != Name_RingMasterQueue) { 
      BOOST_FOREACH(const std::string qname, queueNames) { 
         // In the normal case, RingMaster creates the message queues.
         if (Normal == executionType) {
            boost::shared_ptr <boost::interprocess::message_queue> queue(
               new boost::interprocess::message_queue(boost::interprocess::open_only, qname.c_str()));
            queueMap.Add(qname,queue);
         // For other cases the unit under test has no RingMaster and must create the queues.
         } else if (name != "RingMaster") {
            boost::shared_ptr <boost::interprocess::message_queue> queue(
               new boost::interprocess::message_queue(boost::interprocess::open_or_create, qname.c_str(), stdQueueSize, stdMsgMaximum));
            queueMap.Add(qname,queue);
         }
      }
   }
}
//
//*****************************************************************************
/// \brief Destructor is responsible for erasing performer's message queue from the system.
//*****************************************************************************
//
Performer::~Performer() { 
   LogThis("~Performer: Destroying " + name);
   if (queueName != "Logger") { boost::interprocess::message_queue::remove(queueName.c_str()); }
}
//
//*****************************************************************************
/// \brief Some initialization is common to all performers.
//*****************************************************************************
//
void Performer::CommonInitialization() { 
   // Tell the RingMaster that we're started
   std::stringstream ss;
   ss << name << "->RingMaster: Performer Started";
   Send(MsgTypePerformerStarted,ss.str(),*(queueMap[Name_RingMasterQueue]));
}
//
//*****************************************************************************
/// \brief Some shutdown code is common to all performers.
//*****************************************************************************
//
void Performer::CommonShutdown() { 
   std::stringstream ss;
   ss << name << " is exiting";
   LogThis(ss.str());
   ss.str(std::string());                          // Clear the stringstream
   ss << name << "->RingMaster: Performer Finished";
   Send(MsgTypePerformerFinished,ss.str(),*(queueMap[Name_RingMasterQueue]));
   LogThis(ss.str());
}
//
//*****************************************************************************
/// \brief The message dispatcher waits for messages to arrive on the input queue
///        and dispatches each to the appropriate handler.
//*****************************************************************************
//
void Performer::MessageDispatcher() { 
   // Message dispatching is wrapped in a try/catch with the intention that all exceptions that bubble 
   // up this far result in
   //    1) A message saying what was caught (as far as possible)
   //    2) Orderly shutdown of this commponent
   //    3) std::terminate
   try {

      bool timeToExit(false);
      do {
         std::string message;
         boost::optional<MessageType> type = Receive(message);                   // Receive incoming message

         if (type) {                                                             // If receive successful
                                                                                 // Special logging for shutdown messaging (tracking down inconsistent shutdown)
            // Find the handler for the received MessageType
            if (handlerMap.find(type.get()) != handlerMap.end()) {               // If received message is in the map
               MessageHandler msgHandler = handlerMap[type.get()];               // Get the handler for the message
               (*msgHandler)(type.get(),message,timeToExit);                     // Handle the message

               // Some handler maps (typically for unit tests) contain a wildcard that matches all other MessageTypes
            } else if (handlerMap.find(MsgTypeWildcard) != handlerMap.end()) {   // If the map supports a wildcard match
               MessageHandler msgHandler = handlerMap[MsgTypeWildcard];          // Get the handler for the message
               (*msgHandler)(type.get(),message,timeToExit);                     // Handle the message
            } else {
               std::stringstream ss;
               ss << name << " cannot dispatch " << message;
               LogThis(ss.str());
            }
         }
      } while (!timeToExit);
   } catch (...) {
      CatchAndTerminate();
   }
}
//*****************************************************************************
/// \brief NonLoggingSend implements the Send(const std::string& message, boost::interprocess::message_queue& onThisQueue) functionality.
///        It does not log.  Other Send methods are free to log.
/// \brief message_queue sends can hang.  Reboot fixes the problem.  A possible reference is
///        http://boost.2283326.n4.nabble.com/interprocess-message-queue-hangs-when-another-process-dies-td2648488.html
/// \param[in] message - transmit this string
/// \param[in] queue   - place the string on this queue
/// \return bool saying whether the message was placed on the queue (true) or not (false)
//*****************************************************************************
//
bool Performer::NonLoggingSend(const std::string& message, boost::interprocess::message_queue& onThisQueue, unsigned int priority) {
   try {
      #define NOW boost::posix_time::microsec_clock::universal_time()
      return onThisQueue.timed_send(message.data(), message.size(), priority, NOW);
      #undef NOW
   } catch (const boost::interprocess::interprocess_exception& e) {
      /// \todo How should Performer::NonLoggingSend report a failure to send?
      std::cout << name << " -- Performer::Send(" << message << "): Exception: (" << e.get_error_code() << ") " << e.what() << std::endl;
      return false;
   }
}
//
//*****************************************************************************
/// \brief Send transmits a Message type and a std::string.  It combines these two to create a complete string and places them on a queue.
/// \param[in] type    - message type
/// \param[in] message - transmit this string
/// \param[in] queue   - place the string on this queue
/// \return bool saying whether the message was placed on the queue (true) or not (false)
//*****************************************************************************
//
namespace {
   bool IsNotGuiMsg(MessageType type) {
      switch (type) {
         case MsgStatusBarTextGUI:
         case MsgLegSiteProgGUI:
         case MsgFileProgGUI:
         case MsgFileConversionGUI:
         case MsgFileRankingGUI:
            return false;
         default:
            return false;
      }
   }
}

bool Performer::TryAgain(const std::stringstream& ss, boost::interprocess::message_queue& onThisQueue, unsigned int delay) {
   std::stringstream msg;
   msg << "Send failed.  Retry send " << ss.str() << " after " << delay << " second delay.";
   LogThis(msg.str());
   boost::this_thread::sleep(boost::posix_time::seconds(delay));
   return NonLoggingSend(ss.str(),onThisQueue);
}

bool Performer::Send(MessageType type, const std::string& message, boost::interprocess::message_queue& onThisQueue, unsigned int priority) {
   std::stringstream ss;
   ss << type << "," << message;
   if (!NonLoggingSend(ss.str(),onThisQueue,priority)) {
      if (!TryAgain(ss,onThisQueue, 1)  &&   // Several tries before giving up
          !TryAgain(ss,onThisQueue, 2)  &&
          !TryAgain(ss,onThisQueue, 5)  &&
          !TryAgain(ss,onThisQueue,10)) {
            std::stringstream msg2;
            msg2 << "Send failed after several retries " << ss.str() << ". Returning false.";
            LogThis(msg2.str());
            return false;
      }
   }
   if (IsNotGuiMsg(type)) {                        // Don't log GUI-related messages sent by Synchronize.  They just clog the log file.
      std::stringstream msg;
      msg << name << " sent " << TranslateMsgType(type) << " " << message;
      LogThis(msg.str());
   }
   return true;
}
//
//*****************************************************************************
/// \brief Send transmits a Message type and a std::string.  It combines these two to create a complete string and places them on a queue of known name.
/// \param[in] type    - message type
/// \param[in] message - transmit this string
/// \param[in] queue   - place the string on this queue
/// \return bool saying whether the message was placed on the queue (true) or not (false)
//*****************************************************************************
//
bool Performer::Send(MessageType type, const std::string& message_, const std::string& onThisQueue, unsigned int priority) {
   const std::string message(ConvertBackslash(message_));   // Not critical, just looks nicer in the message log
   std::stringstream ss;
   ss << name << " sending: " << TranslateMsgType(type) << ", " << message << " --> " << onThisQueue;
   LogThis(ss.str());

   if (queueMap.Exists(onThisQueue)) {
      bool success = Send(type,message,*queueMap[onThisQueue],priority);
      if (!success) {
         std::stringstream ss2;
         ss2 << name << " failed to send: " << type << "," << message << " --> " << onThisQueue;
         LogThis(ss2.str());
      }
      return success;
   } else {
      std::stringstream ss1, ss2, ss3;
      ss1 << name << "::Send: Queue " << onThisQueue << " doesn't exist in the Queue Map";
      LogThis(ss1.str());

      std::vector<std::string> contents = queueMap.Contents();
      ss2 << name << " QueueMap contains " << contents.size() << " members";
      LogThis(ss2.str());

      for (auto itr = contents.begin(); itr != contents.end(); itr++) { ss3 << *itr << ", "; }
      LogThis(ss3.str());
      return false;
   }
}
//
//*****************************************************************************
/// \brief Receive a message on the performer's input queue.
/// \param[in] message - receive the message into this location
/// \return MessageType indicating the message type
//*****************************************************************************
//
boost::optional<MessageType> Performer::Receive(std::string& message) {
   // Avoid having the logger iteratively log that it is waiting for messages for the logger
   /// \todo Performer::Receive needs to not depend on the Logger name
   if (name != "Logger") {
      std::stringstream ss;
      ss << name << " Receive, wait for message, " << queueMap[queueName]->get_num_msg() << " now on queue";
      LogThis(ss.str());
   }

   std::size_t msg_size(0);
   unsigned int priority(0);
   try {
      queueMap[queueName]->receive(buffer,sizeof(buffer),msg_size,priority);
   } catch (const boost::interprocess::interprocess_exception& e) {
      std::cout << name<< " Receive: Exception: (" << e.get_error_code() << ") " << e.what() << std::endl;
      return boost::optional<MessageType>();
   }
   return MsgTypeAndContents(msg_size, message);
}
//
//*****************************************************************************
/// \brief Receive a message, waiting no longer than a timeout period.
/// \param[in] message - receive the message into this location
/// \param[in] waitThisManySeconds - how long to wait for a message
/// \return boost::optional<MessageType> indicating the message type
//*****************************************************************************
//
boost::optional<MessageType> Performer::TimedReceive(std::string& message, unsigned int waitThisManySeconds) {
   std::size_t msg_size(0);
   unsigned int priority(0);
   try {
      boost::posix_time::ptime uNow = boost::posix_time::microsec_clock::universal_time();
      boost::posix_time::ptime waitUntil = uNow + boost::posix_time::seconds(waitThisManySeconds);
      if (!queueMap[queueName]->timed_receive(buffer,sizeof(buffer),msg_size,priority, waitUntil)) {
         //std::stringstream ss;
         //ss << name << " TimeReceived failed to receive message within "  << waitThisManySeconds << " seconds." << std::endl;
         //LogThis(ss.str());
         return boost::optional<MessageType>();
      }
   } catch (const boost::interprocess::interprocess_exception& e) {
      std::cout << name<< " Receive: Exception: (" << e.get_error_code() << ") " << e.what() << std::endl;
      return boost::optional<MessageType>();
   }
   return MsgTypeAndContents(msg_size, message);
}
//
//*****************************************************************************
/// \brief Parse a message.
/// \param[in] message - the message to be parsed
/// \return std::vector<std::string> containing the component parts of the message
/// \remark Message parts are separated by commas.
//*****************************************************************************
//
std::vector<std::string> Performer::ParseMessage(const std::string& message) {
   std::vector<std::string> result;
   boost::regex re(",");                                 // Tokens delimited by comma
   // -1 specifies that the regex tells how to split the tokens -- that they are delimited by the regex
   boost::sregex_token_iterator itr(message.begin(), message.end(), re, -1), end;
   while (itr != end) result.push_back(*itr++);
   return result;
}
//
//*****************************************************************************
/// \brief Parse a message.
/// \param[in] msg_size - size of the message to be parsed
/// \param[in] message - the message to be parsed
/// \return std::vector<std::string> containing the component parts of the message
/// \remark Message parts are separated by commas.
/// \remark The caller is expected to protect this method from re-entrance..
//*****************************************************************************
//
MessageType Performer::MsgTypeAndContents(std::size_t msg_size, std::string& message) {
   buffer[msg_size] = 0;
   std::string receivedMessage(buffer);
   std::vector<std::string> tokens = ParseMessage(receivedMessage);
   message.clear();
   BOOST_FOREACH(std::string s, tokens) { if (message.length() > 0) message += ","; message += s; }
   return Serialize(tokens[0]);
}
//
//*****************************************************************************
/// \brief MessageType serialization.
/// \param[in] type - MessageType to be serialized
/// \return std::string serialization
//*****************************************************************************
//
std::string Performer::Serialize(MessageType type) { 
   return circusMessageNS.Serialize(type);
}
//
//*****************************************************************************
/// \brief MessageType serialization.
/// \param[in] serialized - std::string to be serialized
/// \return MessageType serialization
//*****************************************************************************
//
MessageType Performer::Serialize(std::string serialized) { 
   return circusMessageNS.Serialize(serialized);
}
//
//*****************************************************************************
/// \brief Remove type identifer that preceds each message.
//*****************************************************************************
//
std::string Performer::RemoveTypeIdentifierFromMessage(const std::string& message) {
   // Remove leading message type identifier
   const std::regex rx(".*?,(.*)");
   const std::string fmt("$1");
   const std::string result = std::regex_replace(message,rx,fmt);
   return result;
}
//
//*****************************************************************************
/// \brief Convert backslashes in a string to forward slashes
//*****************************************************************************
//
std::string Performer::ConvertBackslash(const std::string input) {
   std::string output(input);
   for (auto itr = output.begin(); itr < output.end(); itr++) {
      if (*itr == '\\') *itr = '/';
   }
   return output;
}
//
//*****************************************************************************
/// \brief Translate MessageType to std::string
//*****************************************************************************
//
std::string Performer::TranslateMsgType(const MessageType msgType) {
   switch (msgType) {
      case MsgTypeWildcard:                     return "MsgTypeWildcard";                             
      case MsgTypeNotCircusMessage:             return "MsgTypeNotCircusMessage";
      case MsgTypeQueueShutdown:                return "MsgTypeQueueShutdown";
      case MsgTypeLogMessage:                   return "MsgTypeLogMessage";
      case MsgTypeError:                        return "MsgTypeError";
      case MsgTypePerformerStarted:             return "MsgTypePerformerStarted";
      case MsgTypePerformerFinished:            return "MsgTypePerformerFinished";
      case MsgHTMLFileName:                     return "MsgHTMLFileName";
      case MsgTextFileName:                     return "MsgTextFileName";
      case MsgFileName:                         return "MsgFileName";
      case MsgLastInSequence:                   return "MsgLastInSequence";
      case MsgQueueSizeQuery:                   return "MsgQueueSizeQuery";
      case MsgQueueSizeResponse:                return "MsgQueueSizeResponse";
      case MsgStatusBarTextGUI:                 return "MsgStatusBarTextGUI";
      case MsgLegSiteProgGUI:                   return "MsgLegSiteProgGUI";
      case MsgFileProgGUI:                      return "MsgFileProgGUI";
      case MsgFileConversionGUI:                return "MsgFileConversionGUI";
      case MsgFileRankingGUI:                   return "MsgFileRankingGUI";
      case MsgCountCompletedProcess:            return "MsgCountCompletedProcess";
      case MsgSpecialLoggerShutdown:            return "MsgSpecialLoggerShutdown";
   }
   return "Unknown message type";
}
//
//*****************************************************************************
/// \brief Catch and Terminate
/// \remark This is the final catch handler for any performer.
/// \remark No parameter is passed. Intended to be called by a catch handler.
//*****************************************************************************
//
void Performer::CatchAndTerminate() {
   LogThis(name + " called CatchAndTerminate");

   try {
   } catch (const std::exception& ex) {
      LogThis(std::string("CatchAndTerminate caught std::exception: ") + ex.what());
   } catch (...) {
      LogThis("CatchAndTerminate caught an ellipsis exception");
   }
   CommonShutdown();
   std::terminate();
}

