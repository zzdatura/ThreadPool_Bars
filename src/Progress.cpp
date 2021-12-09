#include "Progress.hpp"

#include <string>
#include <iomanip>
#include <iostream>

using namespace std;

namespace Progress {

  std::ostream& Bar(double fraction, int width) {
    // the characters we need to build a pretty progress bar
    static string const bar[]  {"▏", "▎", "▍", "▌", "▋", "▊", "▉", "█"};
    // clear the line after cursor position
    static string const clearline   {"\33[K"};
    // sequences for color manipulation
    static string const setcolor    {""};     //{"\33[2;7;30;40m"};
    static string const unsetcolor  {""};     //{"\33[m"};

    fraction = min(max(0.0, fraction), 1.0);  // limit range
    int perc = fraction * 100 + 0.5;          // percentage to be shown
    int fill = fraction * (8*width-1) + 0.5;  // how many filled characters
    int part = fill % 8;                      // partially filled character
    fill /= 8;

    // print a prefix with percentage number
    cout << '\r' << setw(5) << perc << "% │" << setcolor;

    // print the completely filled characters
    for (int i{0}; i < fill; ++i)
      cout << bar[7];

    // now there's one partially filled character
    cout << bar[part];

    // all the rest are empty characters
    for (int i{0}; i < width - fill - 1; ++i)
      cout << ' ';

    // print the suffix of the bar
    cout << unsetcolor << "│ " << clearline;

    return cout;
  }

}
