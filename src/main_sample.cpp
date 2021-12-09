#include <iostream>
#include "ThreadPool_Bars.hpp"

// pretend to run some heavy calculation by just waiting, we use the
// parameter 'i' to make the waiting time slightly different for
// demonstration purposes
void important_calculator(size_t i=0) {
  using namespace std;
  this_thread::sleep_for(chrono::microseconds(1000+10*i));
}

// simple sequence of tests showcasing a progress bar
void test_progress_bars() {
  using namespace std;
  using namespace Progress;

  cout << "Let's first try out a couple of different bars:" << endl;
  Bar(0.3)     << string( 7, ' ') << "30\% bar with default width" << endl;
  Bar(0.6, 20) << string(27, ' ') << "60\% bar with half width"    << endl;
  Bar(0.8, 45) << string( 2, ' ') << "80\% bar with larger width"  << endl;
  cout << endl;

  cout << "Now let's put this in a little loop:" << endl;
  for (int i = 0; i < 1000; ++i) {
    Bar(i/1000.0) << "counting... " << to_string(i+1) << flush;
    important_calculator();
  }
  cout << endl << endl;

  cout << "We can make the counter less redundant using a class:" << endl;
  for (Counter c{1000}; c; ++c) {
    Bar(c) << flush;
    important_calculator();
  }
  cout << endl << endl;
}

// this sequence of tests showcase multiple bars tracking a thread pool
void test_threadpool() {
  using namespace std;
  using namespace ThreadPool;

  cout << "Now here's the deal, we want to use these progress bars" << endl
       << "to track the progress of a thread pool running through"  << endl
       << "its tasks..." << endl;

  {
    Pool pool{};
    Bars bars{pool};
    bars.set_message("I am doing something here...");

    for (int ntasks{0}; ntasks < 30; ++ntasks) {
      pool.execute([&bars, ntasks](){
        string msg{"I am task number " + to_string(ntasks)};
        for (auto &c{bars.new_counter(1000, msg)}; c; ++c)
          important_calculator(ntasks);
      });
    }

    bars.wait();
  }
  cout << endl;

  cout << "That's it!  The interface should be pretty simple, as to"  << endl
       << "avoid interfering with the important calculation that we"  << endl
       << "are doing.  Compare it with the same calculations without" << endl
       << "the pretty bars!" << endl
       << "Calculating..." << endl;

  {
    Pool pool{};
    for (int ntasks{0}; ntasks < 30; ++ntasks) {
      pool.execute([ntasks](){
        for (int i{0}; i < 1000; ++i)
          important_calculator(ntasks);
      });
    }
    pool.wait();
  }
  cout << endl;

  cout << "So basically we just need to initialize the Bars class,"  << endl
       << "set an optional message and use the defined counter"      << endl
       << "inside each task.  Note that if we don't want per-thread" << endl
       << "progress we can just not use the counters, but the total" << endl
       << "bar will update discretely for each complete task only:"  << endl;

  Pool pool{};
  Bars bars{pool};
  bars.set_message("I am doing something here...");
  for (int ntasks{0}; ntasks < 30; ++ntasks) {
    pool.execute([ntasks](){
      for (int i{0}; i < 1000; ++i)
        important_calculator(ntasks);
    });
  }
  bars.wait();
  cout << endl;

  cout << "We left the last one out of scope so we can test if we"  << endl
       << "can reutilize the objects... note that bars.wait() is"   << endl
       << "necessary to guarantee our bars are ready to start over" << endl;

  bars.set_message("This is something new!!!");
  for (int ntasks{0}; ntasks < 30; ++ntasks) {
    pool.execute([&bars, ntasks](){
      string msg{"I am job number " + to_string(ntasks)};
      for (auto &c{bars.new_counter(1000, msg)}; c; ++c)
        important_calculator(ntasks);
    });
  }
  bars.wait();
  cout << endl;

  cout << "That's all for now!" << endl;
}

int main() {
  test_progress_bars();
  test_threadpool();
  return 0;
}
