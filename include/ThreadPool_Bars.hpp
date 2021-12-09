#ifndef THREADPOOL_BARS_HPP
#define THREADPOOL_BARS_HPP

#include <map>
#include <thread>
#include "Progress.hpp"
#include "ThreadPool.hpp"

namespace ThreadPool {

  // Bars
  //
  // This class implements a list of progress bars to track the progress of
  // ThreadPool::Pool.  Each progress bar corresponds to one worker thread,
  // and an additional bar (the first) corresponds to the total progress of
  // all workers combined.
  class Bars {
   public:
    // constructor gets a reference to the pool that we are going to track
    Bars(Pool &pool);

    // destructor will clean up the hooks and finish the tracker thread
    ~Bars();

    // new_counter(n, message) -> counter
    //
    // Create a new counter (and associated bar) for current thread.
    //   n        :total number of steps to count towards
    //   message  :message to show beside this thread's bar
    Progress::Counter& new_counter (int n, std::string_view message="");

    // set the message to be displayed beside the total progress bar
    void               set_message (std::string_view message);

    // wait until tracker thread has been joined, output and bars are ready
    void               wait        ();

   private:
    // a couple of internal naming shortcuts
    using Thread_ID  = std::thread::id;
    using Thread_Bar = std::pair<Progress::Counter, std::string>;
    using List       = std::map<Thread_ID, Thread_Bar>;

    // return the bar (counter + message) associated with given thread id
    Thread_Bar& get_bar (Thread_ID id);

    // print all bars to the stdout
    void        print   ();

    // spawn a new thread responsible for tracking the progress of the pool
    void        start   ();

    Pool        &pool;     // pool that we are tracking
    List        bars;      // list of bars currently in use
    std::mutex  mutex;     // guard the bars list when add/rm bars
    std::thread tracker;   // thread running the tracking
    bool        tracking;  // whether we are still tracking the pool
  };

}

#endif
