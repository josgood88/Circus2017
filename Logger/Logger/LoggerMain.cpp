//
/// \page Logger Logger
/// \remark Logger receives messages and writes them to the log file.
//
#pragma warning (disable:4482)      //  nonstandard extension used: enum 'MessageType' used in qualified name

#include <MessageTypes.h>
#include "Performer.h"
#include <QueueNames.h>
#include <boost/date_time.hpp>
#include <iostream>

namespace {
   boost::scoped_ptr<Performer> logger;      // Performer encapsulates the queue and message handling
   std::ofstream logFile;
   const std::string logFileLocation("D:/CCHR/Projects/Circus/Logs/CircusLogFile.txt");
   std::string Timestamped(const std::string& input);
   bool running_stand_alone = false;                                 // Stand-alone testing has no interprocess queues

   //
   //*****************************************************************************
   /// \brief LogThis logs a single line to the log file.
   /// \param[in] input  log this input string  
   //*****************************************************************************
   //
   void LogThis(const std::string& input) {
      logFile << Timestamped(input) << std::endl;
   }
   void LogEither(const std::string& s) {
      if (!running_stand_alone) LogThis(s);
      else std::cout << s << std::endl;
   }
   void LogEither(const std::stringstream& ss) { LogEither(ss.str()); }
   void LogFlush(const std::string& s) {
      LogEither(s);
      logFile.close();
      logFile.open(logFileLocation, std::ofstream::out + std::ofstream::app);
   }
   //
   //*****************************************************************************
   /// \brief Handle Special Shutdown command from RingMaster.
   /// \brief Parameters follow the (*MessageHandler) pattern defined in Performer.
   //*****************************************************************************
   //
   void Handler_MsgSpecialLoggerShutdown(MessageType, const std::string&, bool& setToExitProcess) {
      LogThis("Logger received special shutdown message");
      setToExitProcess = true;
   }
   //
   //*****************************************************************************
   /// \brief Handle message receipt.
   /// \brief Parameters follow the (*MessageHandler) pattern defined in Performer.
   //*****************************************************************************
   //
   void Handler_Wildcard(MessageType, const std::string& message, bool&) {
      LogThis(message);
   }
   //
   //*****************************************************************************
   /// \brief Timestamped prefaces a string with the current time.
   /// \param[in] input  preface this input string  
   /// \return std::string containing the prefaced input
   //*****************************************************************************
   //
   std::string Timestamped(const std::string& input) {
      boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
      std::stringstream ss;
      ss << now << " " << input;
      return ss.str();
   }
}
//
//*****************************************************************************
/// \brief This the main Logger procedure.  It is driven by messages on its input queue.
/// \remark Logger is a very simple process.  It simply writes log messages to a file until told to shut down.
//*****************************************************************************
//
void main(int argc, char* argv[]) {
   logFile = std::ofstream(logFileLocation);
   LogFlush("Logger start");

   // Logger can be run 
   //    1) Manually to debug a problem starting Circus -- ensure logging is starting
   //    2) Automatically as part of the normal weekly CCHR processing
   Performer::executionType = Normal;
   if (argc > 1) {
      Performer::executionType = std::string(argv[1]) == "StandAlone" ? StandAlone : UnknownExecutionType;
   }
   if (Performer::executionType == Normal) {
      running_stand_alone = false; 
      try {
         logger.reset(new Performer("Logger",Name_LoggerQueue));
      } catch (boost::interprocess::interprocess_exception ex) {
         std::string msg(std::string("Logger start: ") + ex.what());
         LogFlush(msg);
      } catch (std::exception ex) {
         std::string msg(std::string("Logger start: ") + ex.what());
         LogFlush(msg);
      } catch (...) {
         std::string msg("Logger start: Ellipsis exception");
         LogFlush(msg);
      }
      logger->CommonInitialization();

      // Prepare message dispatching
      logger->AddHandler(MsgTypeWildcard,         Handler_Wildcard);
      logger->AddHandler(MsgSpecialLoggerShutdown,Handler_MsgSpecialLoggerShutdown); // Logger ignores the common shutdown message
   
      // Handle incoming messages until told to shut down.
      if (Performer::executionType == Normal) logger->MessageDispatcher();
   }

   // Shutdown and exit
   LogFlush("Logger exit");
   if (Performer::executionType == Normal)logger->CommonShutdown();
}
