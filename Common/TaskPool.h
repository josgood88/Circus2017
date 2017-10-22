#ifndef TASK_POOL_H
#define TASK_POOL_H

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <map>
#include <regex>
#include <string>

namespace TaskPoolService {

   struct WorkerArgs {
      int ID;
      const std::string textToProcess; // Can't be & -- 2nd copy is null
      const std::string term;
      const int value;
      WorkerArgs(int i, const std::string& f, const std::string& t, int v) 
         : ID(i), textToProcess(f), term(t), value(v) { }
   };

   class TaskPool {
      public:
         TaskPool(int numberOfThreads, const std::string& textToProcess, std::map<std::string, std::pair<std::regex, int>> rankingTerms);
         ~TaskPool();
         boost::asio::io_service       m_io;                   ///< boost magic

      private:
         std::map<int, boost::thread*> mapThreads;
         /// Task pool worker
         void Worker(const WorkerArgs);

         //boost::asio::io_service::work m_work;                 ///< keep threads engaged
         boost::thread_group           m_threads;              ///< thread collection
   };

   TaskPool* pGetTaskPool();
}
#endif

//
/// \namespace  <TaskPoolService>
/// \remark Copyright (c) 2012 Volcano Corporation
///
/// \remark TaskPoolService manages a task pool
///
/// \section Responsibilities
/// TaskPoolService is responsible for
///    - allowing a calling thread to pass a task to the thread pool, so it
///      can resume doing whatever it is supposed to do.
///
/// \section Usage
/// TaskPoolService has the following entry points
///    - (TaskPool)     constructor
///    - (pGetTaskPool) get access to the task pool singleton.   this allows direct
///         use of boost post.
//
