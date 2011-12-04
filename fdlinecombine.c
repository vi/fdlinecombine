/*
 * Read multiple streams and combine them to single output (linewise)
 *
 * Created by Vitaly Shukela ("_Vi") in 2011. License=MIT.
 */

#include <stdio.h>
#include <malloc.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

//#define dbgprintf fprintf
#define dbgprintf(...)

#define MAXFD 1024

#define DEFAULT_READ_SIZE 4096
#define SEPARATOR '\n'

int numfds;
char* buffers[MAXFD];
int bufsizes[MAXFD];
int bufpointers[MAXFD];
int fds[MAXFD];
int active_fds;
fd_set rfds;
int little_data_hyster[MAXFD];

void closefd(int index) {
    close(fds[index]);
    FD_CLR(fds[index], &rfds);
    free(buffers[index]);
    --active_fds;
    fds[index]=-1;
}

char* realloc_buffer(int i, int newsize) {
    char* buffer;
    bufsizes[i]=newsize;
    
    if(newsize<DEFAULT_READ_SIZE) {
        /* Integer overflow? */
        buffer=NULL;
    } else {
        buffer = (char*) realloc(buffers[i], bufsizes[i]);
    }

    if(!buffer) {
        fprintf(stderr, "Out of memory on fd %d\n", fds[i]);
        fflush(stderr);
        
        /* refusing to reallocate, returning the old buffer */
        
        bufsizes[i]=-1;
    } else {
        buffers[i] = buffer;
    }
    little_data_hyster[i]=0;
    return buffers[i];
}

int main(int argc, char* argv[]) {

    int i;
    int sret;
    ssize_t ret;
    int maxfd;

    if (argc <= 1) {
        fprintf(stderr, "Usage: fdlinecombine fd1 fd2 ... fdN\n");
        return 1;
    }

    numfds = argc-1;
    active_fds = numfds;
    for(i=1; i<argc; ++i) {
        fds[i-1] = atoi(argv[i]);
    }


    for(i=0; i<numfds; ++i) {
        buffers[i] = (char*) malloc(DEFAULT_READ_SIZE);
        bufsizes[i] = DEFAULT_READ_SIZE;
        bufpointers[i] = 0;
    }

    for(;;) { /* select loop */
        FD_ZERO(&rfds);
        maxfd=0;
        for(i=0; i<numfds; ++i) {
            if (fds[i] >= 0) {
                FD_SET(fds[i], &rfds);
                if(maxfd < fds[i]+1) maxfd = fds[i]+1;
                fcntl(fds[i], F_SETFL, O_NONBLOCK);
            }
        }

        sret = select(maxfd, &rfds, NULL, NULL, NULL);

        if (sret==-1) {
            perror("select");
            return 1;
        }

        for(i=0; i<numfds; ++i) {
            if(FD_ISSET(fds[i], &rfds)) {
                int j;
                int offset = bufpointers[i];
                char* buffer = buffers[i];

                ret = read(fds[i], buffer+offset, bufsizes[i]-offset);
                if (ret==-1) {
                    if (errno == EAGAIN || errno == EINTR) {
                        continue;
                    }
                    dbgprintf(stderr, "fd %d failed\n", fds[i]);
                    closefd(i);
                    continue;
                } else
                if (ret == 0) {
                    closefd(i);
                    continue;
                }

                /* Manage buffer size to keep output fast (can dynamically shrink and enlarge buffers) */
                if (ret == bufsizes[i] - bufpointers[i]) {
                    /* Maximum possible bytes received. Better to enlarge buffer. */
                    --little_data_hyster[i];

                    /* Don't enlarge buffer just to read more than megabyte chunks */
                    /* Also don't enlarge buffer here upon the first request - enlarge if it is trend */
                    if(ret < 1024*1024 && little_data_hyster[i]<-3) {
                        dbgprintf(stderr, "Much data: enlarging buffer from %d to %d\n", bufsizes[i], bufsizes[i]*2);
                        buffer = realloc_buffer(i, bufsizes[i]*2); 
                    }
                } else {
                    if (little_data_hyster[i]<0) little_data_hyster[i]=0;
                }
                
                /* Buffer is too large - can shrink */
                if ( (ret < (bufsizes[i] - offset)/4) && (bufsizes[i]>DEFAULT_READ_SIZE*2) ) {
                    if (offset < DEFAULT_READ_SIZE) { // Don't shrink if it is just a looong line
                        ++little_data_hyster[i];
                        if(little_data_hyster[i]>5) { // Don't shink-grow-shink-grow the buffer rapidly
                            dbgprintf(stderr, "Little data: shrinking buffer from %d to %d\n", 
                                    bufsizes[i], offset+ret+DEFAULT_READ_SIZE);
                            buffer = realloc_buffer(i, offset+ret+DEFAULT_READ_SIZE); 
                        }
                    }
                } else {
                    if(little_data_hyster[i]>0) little_data_hyster[i]=0;
                }

                /*
                 *        "debt"                newly_received_data   free_buffer
                 *    SSSSSSSSSSSSSSSSSSSSSSSSSSNNNNNNNNNNNNN\nNNNNNNN--------------
                 *    ^                         ^            ^        ^             ^
                 *    0                         offset       offset+j offset+ret   bufsize
                 */

                /* Scan new data for newlines */
                for(j=ret-1; j!=-1; --j) {
                    if (buffer[offset+j] == SEPARATOR) {
                        fwrite(buffer, 1, offset+j+1, stdout);
                        fflush(stdout);
                        memmove(buffer, buffer+offset+j+1, ret-j-1);
                        bufpointers[i] = offset = ret-j-1;
                        break;
                    }
                }
                if(j==-1) {
                    /* No newline in newly received data */
                    offset+=ret;
                    bufpointers[i] = offset;
                }

                /* enlarge buffer if we don't have space to read more data
                 *     (used if too long lines)
                 */
                if(bufsizes[i] - offset < DEFAULT_READ_SIZE && bufsizes[i]>0) {
                    dbgprintf(stderr, "Long line: enlarging buffer from %d to %d\n", bufsizes[i], bufsizes[i]*2);
                    buffer = realloc_buffer(i, bufsizes[i]*2); 
                }

                if (bufsizes[i]==-1) {
                    closefd(i);
                }

            }
        } /* fds loops */

        if (active_fds==0) return 0; /* All inputs finished */
    } /* main loop */
}
