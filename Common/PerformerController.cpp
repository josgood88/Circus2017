//
/// \page PerformerController PerformerController
/// \remark PerformerController provides the basic functionality needed by all testers of performers.
//
///   <h1>Responsibilities</h1>
///   Performer's responsibilities are: \n
///   1) Provide message dispatch for a single message.  Testers typically are looking for arrival of a specific message. \n
//
#pragma warning (disable:4996)      // 'std::_Copy_backward': Function call with parameters that may be unsafe
#include "PerformerController.h"
#include <MessageTypes.h>
#include <QueueNames.h>

#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/named_recursive_mutex.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

//
//*****************************************************************************
/// \brief PerformerController extends Performer and takes the same parameters.
/// \param[in] name_      - this performer's name
/// \param[in] queueName_ - this performer reads messages from this queue
//*****************************************************************************
//
PerformerController::PerformerController(const std::string& name_, const std::string& queueName_) 
                    : Performer(name_, queueName_)
{ }
//
//*****************************************************************************
/// \brief Destructor is responsible for erasing the message queue from the system.
///        Leave the RingMaster's queue and the Logger's queue alone.
//*****************************************************************************
//
PerformerController::~PerformerController() {  }
//
//*****************************************************************************
/// \brief StartProcess starts a specified executable.
/// \param[in] path  full path to the executable
/// \return boost::optional<PROCESS_INFORMATION> defining the newly created process.
/// \remark This method doesn't complain if the path doesn't exist -- it simply doesn't set the returned PROCESS_INFORMATION's process handle.
//*****************************************************************************
//
boost::optional<Poco::ProcessHandle> PerformerController::StartProcess(const std::string& path) {
   boost::optional<Poco::ProcessHandle> ph;
   const Poco::Process::Args args;
   if (boost::filesystem::exists(path)) {
      try {
         ph = Poco::Process::launch(path,args);
      } catch (...) {
         // No need to do anything special, since the return value is boost::optional
      }
   }
   return ph;
}
//
//*****************************************************************************
/// \brief EndProcess ends the process identified by the passed structure.
//*****************************************************************************
//
bool PerformerController::EndProcess(Poco::ProcessHandle process) {
   return true;
}
//
//*****************************************************************************
/// \brief The message dispatcher waits for a message to arrive on the input queue.
///        If the defined message arrives, it dispatches the message to the passed handler.
/// \return bool saying whether the message was received on the queue (true) or not (false).
//*****************************************************************************
//
bool PerformerController::SingleMessageDispatcher(MessageType expectedType, MessageHandler msgHandler, unsigned int waitThisManySeconds) { 
   std::stringstream ss;
   ss << name << " SingleMessageDispatcher entry: " << waitThisManySeconds;
   LogThis(ss.str());

   bool successfulReceive(false);
   std::string message;
   boost::optional<MessageType> type = TimedReceive(message,waitThisManySeconds);   // Receive incoming message
   if (type) {                                                                      // If receive successful
      bool timeToExit(false);                                                       // (simply following MessageHandler signature)
      (*msgHandler)(type.get(),message,timeToExit);                                 // Handle the message
      successfulReceive = true;
   }
   return successfulReceive;
}
//
//*****************************************************************************
/// \brief Check message type for correctness
/// \param[in] expected - Expected message type
/// \param[in] actual   - Actual message type
/// \return boost::optional<std::string>() if no complaint, else string explaining the problem
//*****************************************************************************
//
boost::optional<std::string> PerformerController::MessageCheck(MessageType expected, boost::optional<MessageType> actual) {
   boost::optional<std::string> result;
   std::stringstream ss;
   if (!actual) {
      ss << "Message had no type, expected " << Serialize(expected);
      result = ss.str();
   } else if (expected != actual.get()) {
      ss << std::string("Message type mismatch: Expected ") << Serialize(expected) << ", actual " << Serialize(actual.get());
      result = ss.str();
   }
   return result;
}
//
//*****************************************************************************
/// \brief Check message payload for correctness
/// \param[in] expected - Expected message type
/// \param[in] actual   - Actual message type
//*****************************************************************************
//
boost::optional<std::string> PerformerController::MessageCheck(std::string expected, std::string actual) {
   boost::optional<std::string> result;
   std::stringstream ss;
   if (expected != actual) {
      ss << "Message content mismatch: Expected " << expected << ", actual " << actual;
      result = ss.str();
   }
   return result;
}

