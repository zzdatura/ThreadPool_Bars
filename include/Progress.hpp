#ifndef PROGRESS_HPP
#define PROGRESS_HPP

#include <ostream>

namespace Progress {

  // Bar (fraction, width) -> stdout
  //
  // Print a progress bar to the standard output.
  //   fraction :[0.0, 1.0] filled fraction of the bar
  //   width    :number of characters occupied by the bar
  // Returns the output stream so a message can be appended to it
  std::ostream& Bar (double fraction, int width=40);

  // Counter
  //
  // Implements a simple integer counter towards a determined number of total
  // steps.  This class is mainly used for implementing implicit conversion
  // to a real number between 0 and 1, suitable for calling 'Bar()' from
  // within a loop.
  class Counter {
   public:
    // constructs an object given the expected number of total steps
    Counter(long steps = 1) : count{0}, step{1./steps} {}

    // implicit conversion to real number of 'count / total'
    inline operator double      () const {return count*step;}

    // returns true if the counter hasn't reached the total
    inline operator bool        () const {return operator double() < 1.0;}

    // increments the counter towards the total
    inline Counter& operator++  ()       {++count; return *this;}

    // increments the total number of steps
    inline void     add_step    ()       {step /= 1.0 + step;}

    // get the current weight of a single step: '1.0 / total'
    inline double   get_step    () const {return step;}

   private:
    long   count;  // current count
    double step;   // weight of a single step: '1.0 / total'
  };

}

#endif
