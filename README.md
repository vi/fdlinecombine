Multiplex multiple input streams into stdout using \n as separators.

It is attempt to make a program that Does One Thing, But Does It Well.

For example,

    ./fdlinecombine  6 5 7  5< <( sleep 0; echo qqq )  6< <( sleep 0; echo www) 7< <( sleep 0; echo eee);

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

    yes | ( ulimit -v 500; ./fdlinecombine_static 0)  > /dev/null

Example of listening two sockets and combining the data to file:

    ./fdlinecombine 3 4   3< <( nc -lp 9988 < /dev/null)  4< <( nc -lp 9989 < /dev/null) > out

You can override line separator (including to multiple characters) with SEPARATOR environment variable set to additional file descriptor.
Trailing lines that does not end in separator are also outputted (followed by separator) unless NO_CHOPPED_DATA is set.

More advanced example:

    SEPARATOR=10   ./fdlinecombine 0 5 6  \
           5< <(nc -lp 9979 < /dev/null)   \
           6< <(perl -e '$|=1; print "TIMER\n---\n" and sleep 2 while true' < /dev/null) \
           10< <(printf -- '\n---\n') > out 

This will transfer data from stdin, opened TCP port and periodic timer to file "out" using "---" line as separator.


Pre-built executables are on "Download" GitHub page or here:

* http://vi-server.org/pub/fdlinecombine_static  - musl-based
* http://vi-server.org/pub/fdlinecombine
* http://vi-server.org/pub/fdlinecombine_arm 
* http://vi-server.org/pub/fdlinecombine_arm_static 
