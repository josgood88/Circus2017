#include "Logger.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>

class thread_pool {
private:
   boost::asio::io_service io_service;
   boost::asio::io_service::work work;
   boost::thread_group threads;
   std::size_t available;
   boost::mutex mutex;
public:

   thread_pool(std::size_t pool_size): work(io_service), available(pool_size) {
      for (std::size_t i = 0; i < pool_size; ++i) {
         threads.create_thread(boost::bind(&boost::asio::io_service::run,&io_service));
      }
   }

   ~thread_pool() {
      io_service.stop();		// Force all threads to return from io_service::run().
      try {
         threads.join_all();
      } catch (const std::exception&) { }
   }

   /// @brief Adds a task to the thread pool if a thread is currently available.
   template <typename Task>
   bool run_task(Task task) {
      boost::unique_lock< boost::mutex > lock(mutex);
      if (0 == available) return false;	// If no threads are available, then return.
      --available;						// Decrement count, indicating thread is no longer available.
      // Post a wrapped task into the queue.
      io_service.post(boost::bind(&thread_pool::wrap_task,this, boost::function<void()>(task)));
	  return true;
   }

   /// @brief Adds a task to the thread pool, waiting until a thread is available.
   template <typename Task>
   bool run_task_wait_until_thread_available(Task task) {
	   while (!run_task(task)) {
		   LoggerNS::Logger::Log("Waiting");
		   Sleep(1000);
	   }
	  return true;
   }

private:
   /// @brief Wrap a task so that the available count can be increased once
   ///        the user provided task has completed.
   void wrap_task(boost::function<void()> task) {
      // Run the user supplied task.
      try {
         task();
      }
      // Suppress all exceptions.
      catch (const std::exception&) { }

      // Task has finished, so increment count of available threads.
      boost::unique_lock< boost::mutex > lock(mutex);
      ++available;
   }
};