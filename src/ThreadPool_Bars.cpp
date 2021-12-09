#include "ThreadPool_Bars.hpp"
#include "ThreadPool.hpp"

#include <chrono>
#include <csignal>
#include <functional>
#include <iostream>

namespace ThreadPool {

  using namespace std;
  using namespace Progress;
  thread::id const id0{};     // default unjoinable value used for total bar

  Bars::Bars(Pool &pool)
      : pool{pool}
      , tracking{false} {

    // add hook to thread pool when enqueuing a new task
    pool.set_enqueue([this](){
      auto &bar = get_bar(id0);
      unique_lock lock{mutex};

      // already tracking, just increment our task counter total number
      if (tracking) {
        lock.unlock();
        get<0>(bar).add_step();
      }
      // first task: counter is default initialized to 1 and we start tracker
      else {
        tracking = true;
        lock.unlock();
        start();
      }
    });

    // add hook to the thread pool when dequeuing (after executing) a task
    pool.set_dequeue([this](){
      // we just need to mark a task as done by increment task counter
      unique_lock _{mutex};
      tracking = ++get<0>(get_bar(id0));
    });
  }

  Bars::~Bars() {
    // destructor will immediately shut down everything
    tracking = false;
    if (tracker.joinable())
      tracker.join();
    // and don't forget to remove the hooks from the thread pool
    pool.set_enqueue();
    pool.set_dequeue();
  }

  Counter& Bars::new_counter(int n, string_view message) {
    // get a (maybe new) bar associated with this thread
    auto &bar = get_bar(this_thread::get_id());
    // reset the counter and message to the requested values
    bar = {Counter{n}, string{message}};
    // return the counter
    return get<0>(bar);
  }

  void Bars::set_message(std::string_view message) {
    // set message of the total bar (thread id0)
    get<1>(get_bar(id0)) = message;
  }

  void Bars::wait() {
    // wait for pool to reach idle state (i.e. all tasks finished)
    pool.wait();
    // then wait for last update of the bars to the screen
    if (tracker.joinable())
      tracker.join();
    // output state should be clean now and we can use Bars object again
  }

  Bars::Thread_Bar& Bars::get_bar(Thread_ID id) {
    try {
      // get bar associated with this thread, throw if nonexistent
      return bars.at(id);
    }
    catch (out_of_range const &e) {
      // to create a new bar we need to protect our object
      unique_lock _{mutex};
      return bars[id];
    }
  }

  void Bars::print() {
    // first bar (total) needs to account for partial values of others
    auto const &[counter, message] = bars[id0];
    double total = counter;
    double step  = counter.get_step();
    for (auto const &[id, bar] : bars) {
      if (id == id0) continue;
      double partial = get<0>(bar);
      // if partial >= 1 it's not partial, therefore already accounted for
      if (partial < 1)
        total += step * partial;
    }
    Bar(total) << message << endl;

    // the other bars are straightforward
    for (auto const &[id, bar] : bars) {
      if (id == id0) continue;
      auto const &[counter, message] = bar;
      Bar(counter) << message << endl;
    }

    // now here's the trick to move the cursor up to the first bar's line
    // so next iteration when we print, we will overwrite the bars in place
    cout << "\33[" << bars.size() << 'F';
  }

  void Bars::start() {
    // set up the tracker thread with this lambda
    tracker = thread{[this](){
      using namespace chrono_literals;

      // first, hide cursor for a cleaner output
      cout << "\033[?25l";

      // enter loop that updates the screen every iteration until finished
      do {
        this_thread::sleep_for(16ms /* equiv. approx 60Hz */);
        print();
      } while (tracking);

      // after finishing, move the cursor to the line after all bars
      for (int i = bars.size(); i > 0; --i)
        cout << endl;

      // and clear all the counters
      bars.clear();

      // finally, restore cursor visibility
      cout << "\033[?12l\033[?25h" << flush;
    }};
  }

}
