Multiplex multiple input streams into stdout using \n as separators.

It is attempt to make a program that Does One Thing, But Does It Well.

For example,

    fdlinecombine <( sleep 0; echo qqq ) <( sleep 0; echo www) <( sleep 0; echo eee)

should produce one of:

* qqq\nwww\neee\n
* qqq\neee\nwww\n
* eee\nqqq\nwww\n
* eee\nwww\nqqq\n
* www\neee\nqqq\n
* www\nqqq\nqqq\n

, but never "qqwww\nqee\ne\n" or like that.

The program should handle very long lines (storing them in memory), short reads, memory outage. In case of memory outage it closes the problematic file descriptor.

The program dynamically shrinks and enlarges buffers as line gets longer (or many bytes available at once)

In case of only short lines the program should use little memory:

    yes | ( ulimit -v 500; fdlinecombine 0)  > /dev/null

Example of listening two sockets and combining the data to file:

    fdlinecombine <( nc -lp 9988 < /dev/null) <( nc -lp 9989 < /dev/null) > out

You can override line separator (it can be longer than one character) with SEPARATOR environment variable set to additional file descriptor (or file name).
Trailing lines that does not end in separator are also outputted (followed by separator) unless NO_CHOPPED_DATA is set.

More advanced example:

    SEPARATOR=<(printf %s '\n---\n')  fdlinecombine 0 5 6  \
           5< <(nc -lp 9979 < /dev/null)   \
           6< <(perl -e '$|=1; print "TIMER\n---\n" and sleep 2 while true' < /dev/null) \
           > out 

This will transfer data from stdin, opened TCP port and periodic timer to file "out" using "---" line as separator.


Pre-built executables are on "Download" GitHub page or here:

* http://vi-server.org/pub/fdlinecombine_static  - musl-based
* http://vi-server.org/pub/fdlinecombine
* http://vi-server.org/pub/fdlinecombine_arm 
* http://vi-server.org/pub/fdlinecombine_arm_static 
