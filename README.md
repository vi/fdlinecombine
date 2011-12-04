Multiplex multiple input streams into stdout using \n as separators.

It is attempt to make a program that Does One Thing, But Does It Well.

For example,

    ./fdlinecombine  6 5 7  5< <( sleep 0; echo qqq )  6< <( sleep 0; echo www) 7< <( sleep 0; echo eee);

should produce one of:

    qqq\nwww\neee\n
    qqq\neee\nwww\n
    eee\nqqq\nwww\n
    eee\nwww\nqqq\n
    www\neee\nqqq\n
    www\nqqq\nqqq\n

, but never "qqwww\nqee\ne\n" or like that.

The program should handle very long lines (storing them in memory), short reads, memory outage. In case of memory outage it closes the problematic file descriptor.

The program dynamically shrinks and enlarges buffers as line gets longer (or many bytes available at once)

In case of only short lines the program should use little memory:

    yes | ( ulimit -v 500; ./fdlinecombine_static 0)  > /dev/null

Example of listening two sockets and combining the data to file:

    ./fdlinecombine 3 4 3< <( nc -lp 9988)  4< <( nc -lp 9989) > out

Compiled i386 static executable is on http://vi-server.org/pub/fdlinecombine_static .
