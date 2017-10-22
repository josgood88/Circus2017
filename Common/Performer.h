#ifndef performer_h
#define performer_h

#include <MessageTypes.h>
#include <QueueMap.h>
#include <QueueNames.h>

#include <boost/filesystem/path.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/optional.hpp>
#include <boost/regex.hpp>
#include <boost/scoped_ptr.hpp>
#include <string>

typedef void (*MessageHandler)(MessageType, const std::string& message, bool& setToExitProcess);
const unsigned int stdQueueSize(5000);
const unsigned int stdMsgMaximum(9999);

const unsigned int Priority_Shutdown(1);
const unsigned int Priority_Ordinary(100);

// Execution Types
//    SingleFile:     Process a single file (a test case)
//    StandAlone:     Iterate over all files (a test case)
//    Normal:         Production processing
enum ExecutionType { UnknownExecutionType, SingleFile, StandAlone, CTA, Custom, UnitTest, Normal };

class Performer {
public:
   Performer(const std::string& name_, const std::string& queueName_);
   ~Performer();
   void CatchAndTerminate();
   void CommonInitialization();
   void CommonShutdown();
   void AddHandler(MessageType type, MessageHandler handler) { handlerMap[type] = handler; }
   void MessageDispatcher();
   bool LogThis(const std::string& message) { return NonLoggingSend(message,*(queueMap[Name_LoggerQueue])); }
   bool Send(MessageType type, const std::string& message, boost::interprocess::message_queue& onThisQueue, unsigned int priority = Priority_Ordinary);
   bool Send(MessageType type, const std::string& message, const std::string& onThisQueue, unsigned int priority = Priority_Ordinary);
   boost::optional<MessageType> Receive(std::string& message);
   boost::optional<MessageType> TimedReceive(std::string& message, unsigned int waitThisManySeconds);
   bool IsMessageAvailable() { return queueMap[queueName]->get_num_msg() > 0; }
   std::vector<std::string> ParseMessage(const std::string& message);
   MessageType MsgTypeAndContents(std::size_t msg_size, std::string& message);
   static std::string ConvertBackslash(const std::string input);

   std::string TranslateMsgType(const MessageType);
   bool SendStatusBar  (const std::string& message) { return Send(MsgStatusBarTextGUI, message,*(queueMap[Name_RingMasterQueue])); }
   bool SendLegSiteProg(const std::string& message) { return Send(MsgLegSiteProgGUI,   message,*(queueMap[Name_RingMasterQueue])); }
   bool SendFileProg   (const std::string& message) { return Send(MsgFileProgGUI,      message,*(queueMap[Name_RingMasterQueue])); }
   bool SendFileConv   (const std::string& message) { return Send(MsgFileConversionGUI,message,*(queueMap[Name_RingMasterQueue])); }
   bool SendFileRank   (const std::string& message) { return Send(MsgFileRankingGUI,   message,*(queueMap[Name_RingMasterQueue])); }

   static std::string RemoveTypeIdentifierFromMessage(const std::string& message);
   static std::string Serialize(MessageType type);
   static MessageType Serialize(std::string serialized);
   static std::string TableName() { return tableName; }
   static void TableName(std::string s) { tableName = s; }
   static boost::filesystem::path BaseLocalFolder() { return baseLocalFolder; }

   static ExecutionType executionType;
   static std::string tableName;
   QueueMap queueMap;

protected:
   std::string name;
   std::string queueName;
   static const boost::filesystem::path baseLocalFolder;
private:
   bool NonLoggingSend(const std::string& message, boost::interprocess::message_queue& onThisQueue, unsigned int priority = Priority_Ordinary);
   bool TryAgain(const std::stringstream& ss, boost::interprocess::message_queue& onThisQueue, unsigned int delay);
   std::map<MessageType, MessageHandler> handlerMap;
   char buffer[stdMsgMaximum];
};

#endif
