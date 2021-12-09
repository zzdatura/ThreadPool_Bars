#include "ThreadPool.hpp"
using namespace std;

namespace ThreadPool {

  Pool::Pool(size_t nthreads) : done{false}, processing{0} {
    // start up all the nthreads that will be waiting for jobs
    while (nthreads --> 0) // the cutest syntax abuse of all of C++
      // we add a new thread to our list of threads using a lambda function
      threads.emplace_back(thread{[this]() {
        Task task;
        // it will run forever until explicitly asked to stop
        while (true) {
          // protect the queue from racing conditions in this block
          {
            unique_lock lock{queue_lock};

            // wait for tasks to be added to queue or finish
            queued.wait(lock, [this]() -> bool {
              // move on if there are more tasks or if we are finished
              return !tasks.empty() || done;
            });

            // before processing more tasks, check if we want to stop
            if (done)
              return;

            // acquire one task
            task = std::move(tasks.front());
            tasks.pop();
            ++processing;
          }

          // execute task
          task();

          // protect the hook execution and the counter
          {
            unique_lock lock{queue_lock};
            if (hook_dequeue)
              hook_dequeue();
            --processing;
          }

          // notify that a task was removed from queue
          dequeued.notify_one();
        }
      }});
  }

  Pool::~Pool() {
    // signal all threads to finish immediately
    done = true;
    queued.notify_all();

    // wait for joining back all threads
    for (auto &thread : threads)
      if (thread.joinable())
        thread.join();
  }

  void Pool::wait() {
    unique_lock lock{queue_lock};
    dequeued.wait(lock, [this]() -> bool {
      // move on only if tasks queue is empty and no task is being processed
      return tasks.empty() && !processing;
    });
  }

}
