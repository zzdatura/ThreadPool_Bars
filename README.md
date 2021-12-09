# Simple thread pool with progress bars in C++

This sample code showcases a simple canonical thread pool which can be
tracked by progress bars shown in the terminal.  Each thread can be
associated with one progress bar, and an additional one accounts for all the
tasks in the pool. A simple use case is when each task in the pool is long
enough so it could benefit from user feedback while it's executing.

The 'jobs' in this sample are fake and don't do anything other than wait for
a certain amount of time.  Everything should finish running in about 40s.

It is assumed compilation with `gcc`, using `c++20` (alias template
deduction), and output should be fine as long as the terminal can draw the
following nine characters in addition to ASCII:

    │ ▏ ▎ ▍ ▌ ▋ ▊ ▉ █
    (vertical line and 8 block chars gradually filling from left to right)

Finally, the terminal should also support the ANSI escape sequences:

    CSI n F        Cursor previous line
    CSI ? 25 l     Hide cursor (DECTCEM), VT220
    CSI ? 25 h     Show cursor (DECTCEM), VT220
    CSI ? 12 l     Stop blinking cursor (AT&T 610)

While the last three sequences are not strictly necessary, they make the
output much cleaner `;)`, though they could be easily disabled, if there is a
problem, in `ThreadPool::Bars::start()`.

Compiling the code with the provided `makefile` and `makedepend.py` will
generate the binary `bin/sample`.
