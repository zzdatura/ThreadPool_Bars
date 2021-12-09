#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <future>
#include <queue>
#include <vector>

namespace ThreadPool {

  // Pool
  //
  // Simple canonical implementation of a thread pool.  Threads are setup at
  // construction and tasks are later dynamically created and added to a
  // queue. Idle threads get the first available task and execute it before
  // waiting for a new one.
  class Pool {
   private:
    using thread = std::thread;

    // used later to select types that are functions of set arguments
    template<typename ...T>
    using IsInvocable = std::enable_if_t<std::is_invocable_v<T&&...>>;

    // generic type for any task
    template<typename T=void>
    using Task = std::packaged_task<T()>;

   public:
    // construct a pool with given number of threads (defaults to hardware)
    Pool(size_t nthreads=std::max(1u, thread::hardware_concurrency()));

    // destructor needs to wait/cleanup threads
    ~Pool();

    // wait until all threads are idle and there are no more tasks in queue
    void wait ();

    // execute (function(), ...args) -> future
    //
    // Create a new task to the pool that will execute function(args)
    //   function   :reference to the function to be called
    //   args       :list of arguments to be passed to the function
    // It returns a future of the same return type as function's
    template<typename R, typename ...A, typename=IsInvocable<R, A...>>
    auto execute(R &&function, A &&...args);

    // split (function(), n, ...args) -> future
    //
    // Create new tasks to the pool that will execute function(n, args)
    //   function   :reference to the function to be called
    //   n          :number of times to execute the function
    //   args       :list of arguments to be passed to the function
    // It returns a list of futures of the same return type as function's
    // This differs from execute() as it will split the load 'n' into equal
    // parts according to the number of threads and add as many tasks as
    // there are number of threads.
    template<typename R, typename ...A, typename=IsInvocable<R, long, A...>>
    auto split(R &&function, long n, A &&...args);

    // set_hook_(en|de)queue (function())
    //
    // Set function() to be executed when enqueueing/dequeueing new tasks.
    inline void set_enqueue(std::function<void()> f=0) {hook_enqueue = f;}
    inline void set_dequeue(std::function<void()> f=0) {
      // dequeue hook is executed by every thread, so protect it
      std::unique_lock _{queue_lock};
      hook_dequeue = f;
    }

   private:
    // internal method that adds task to the pool
    template<typename T>
    auto enqueue(T &&task);

    bool                      done;             // whether finished tasks
    size_t                    processing;       // count working threads
    std::condition_variable   queued, dequeued; // signals when add/rm task
    std::mutex                queue_lock;       // guard sensitive access
    std::vector<std::thread>  threads;          // list of running threads
    std::queue<Task<>>        tasks;            // queue of tasks to be run
    std::function<void()>     hook_enqueue;     // executes after enqueueing
    std::function<void()>     hook_dequeue;     // executes after finish task
  };

  template<typename R, typename ...A, typename>
  auto Pool::execute(R &&function, A &&...args) {
    using namespace std;
    // The lambda is used to construct the task by coupling the function with
    // its arguments.  We need to be careful about how we pass on the
    // arguments, we want to keep the value category of the arguments, that
    // is, whether they are lvalues or rvalues, otherwise we might
    // inadvertently miss the reference when they are accessed later.
    // forward() takes care of keeping the value category
    // move() guarantees we now have ownership of the data we will use later
    // Now we pass this task to the enqueue() method that returns a future.
    return enqueue(Task<invoke_result_t<R, A...>>{[
      function = move(function),
      args     = make_tuple(forward(args)...)
    ] () mutable {
      return apply(move(function), move(args));
    }});
  }

  template <typename R, typename ...A, typename>
  auto Pool::split(R &&function, long n, A &&...args) {
    using namespace std;
    // we want to split a work in equal parts among the threads so we need as
    // many future values as the number of threads.
    using result_t = invoke_result_t<R, long, A...>;
    vector<future<result_t>> futures{threads.size()};

    // now for each part we create a task with execute using our splitted n
    for (auto &future : futures)
      future = execute(move(function), n/threads.size(), forward(args)...);

    return futures;
  }

  template<typename T>
  auto Pool::enqueue(T &&task) {
    using namespace std;
    // get the future result of the task
    auto future = task.get_future();

    {
      // careful to lock the queue while we emplace a new task
      unique_lock _(queue_lock);
      tasks.emplace(move(task));
    }

    // execute the enqueue hook if there is one
    if (hook_enqueue)
      hook_enqueue();

    // notify the next available thread that a task was added
    queued.notify_one();

    // return the future associated with this task
    return future;
  }

}

#endif
