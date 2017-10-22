#ifndef message_types_h
#define message_types_h

enum MessageType {
   MsgTypeWildcard = -1,

   // General usage
   MsgTypeNotCircusMessage = 0,
/* 1*/  MsgTypeQueueShutdown, 
/* 2*/  MsgTypeLogMessage, 
/* 3*/  MsgTypeError,
/* 4*/  MsgTypePerformerStarted, 
/* 5*/  MsgTypePerformerFinished,
        // Performer-specific messages
/* 6*/  MsgHTMLFileName,                     // Path to local copy of HTML file (contains bill text)
/* 7*/  MsgTextFileName,                     // Path to local copy of text file (remainder after HTML removed)
/* 8*/  MsgTextAndDiffFiles,                 // Paths to text file and difference file
/* 9*/  MsgFileName,
/*10*/  MsgLastInSequence,
/*11*/  MsgQueueSizeQuery,
/*12*/  MsgQueueSizeResponse,
/*13*/  MsgStatusBarTextGUI,
/*14*/  MsgLegSiteProgGUI,
/*15*/  MsgFileProgGUI,
/*16*/  MsgFileConversionGUI,
/*17*/  MsgFileRankingGUI,
/*18*/  MsgCountCompletedProcess,
/*19*/  MsgSpecialLoggerShutdown,
};

#endif
