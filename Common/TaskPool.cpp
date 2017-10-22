#include "TaskPool.h"

//
//*****************************************************************************
/// \brief create a thread pool to handle commands.  the intent is to allow the
///   hardware thread to post a task and immediately return to blocking on 
///   an IOCTL
/// \remake alternate syntax: boost::bind(&boost::asio::io_service::run, &m_io)
//*****************************************************************************
//
TaskPoolService::TaskPool::TaskPool(int numberOfThreads, const std::string& textToProcess, std::map<std::string, std::pair<std::regex, int>> rankingTerms) 
   //: m_work(m_io) 
   {
      std::map<std::string, std::pair<std::regex, int>>::const_iterator m_itr(rankingTerms.cbegin());
      for (int i = 0; i < numberOfThreads; ++i) {
         WorkerArgs w(i,textToProcess, m_itr->first, m_itr->second.second);
         m_itr++;
         mapThreads[i] = m_threads.create_thread(boost::bind(&TaskPool::Worker, this, w));
      }
}
//
//*****************************************************************************
/// \brief kill off the thread pool
//*****************************************************************************
//
TaskPoolService::TaskPool::~TaskPool() {
   m_io.stop();
   m_threads.join_all();
}
//
//*****************************************************************************
/// \brief worker thread in the boost::asio thread pool.
/// \remark wrapping the run() with a try catch block prevents a posted task
///   from killing the app (VDCOM queue sizing example) if it doesn't handle
///   its exceptions. the intent isn't to handle exceptions (that is the 
///   responsibility of the task writer. instead it is to protect the 
///   infrastructure.)
/// \param[in] id diagnostic nicety, passing in an id means that the worker
///   threads are named
//*****************************************************************************
//
void TaskPoolService::TaskPool::Worker(const WorkerArgs w) {
   std::stringstream ss;
   //ss << "Worker " << w.ID << " processing " << w.textToProcess.length() << "(" << w.term << ", " << w.value << ")\n";
   //std::cout << ss.str();

   std::ptrdiff_t const match_count(std::distance(
      std::sregex_iterator(w.textToProcess.begin(), w.textToProcess.end(), std::regex(w.term)), 
      std::sregex_iterator()));
   if (match_count > 0) {
      ss.clear();
      ss << "Worker " << w.ID << " Count=" << match_count << ", value=" << w.value << " : " << w.term << "\n";
      std::cout << ss.str();
   }

   ss.clear();
   ss << "Worker " << w.ID << " exit.\n";
   std::cout << ss.str();
} 
