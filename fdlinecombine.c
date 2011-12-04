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
#include <stdlib.h>

//#define dbgprintf fprintf
#define dbgprintf(...)

#define MAXFD 1024

#define DEFAULT_READ_SIZE 4096
#define SEPARATOR '\n'


struct fdinfo {
    char* buffer;
    int bufsize;
    int offset;
    int fd;

    /* Increases when too little data received. Decreases when full buffer of data received. */
    int little_data_hyster; 
};

struct fdinfo* fds;
int num_active_fds;
int numfds;
fd_set rfds;

void closefd(struct fdinfo* f) {
    close(f->fd);
    FD_CLR(f->fd, &rfds);
    free(f->buffer);
    --num_active_fds;
    f->fd = -1;
}

char* realloc_buffer(struct fdinfo* f, int newsize, int obligatory) {
    char* buffer;
    
    if(newsize<DEFAULT_READ_SIZE) {
        /* Integer overflow? */
        buffer=NULL;
    } else {
        buffer = (char*) realloc(f->buffer, newsize);
    }

    if(!buffer) {
        if(obligatory) {
            fprintf(stderr, "Out of memory on fd %d\n", f->fd);
            fflush(stderr);
        
            /* refusing to reallocate, returning the old buffer */
        
            f->bufsize=-1;
        } else {
            /* Can't reallocate, but don't really need to. Just leave the buffer as it was. */
        }
    } else {
        f->buffer = buffer;
        f->bufsize=newsize;
    }

    f->little_data_hyster=0;
    return f->buffer;
}

int main(int argc, char* argv[]) {

    int i;
    int sret;
    ssize_t ret;
    int maxfd;

    if (argc <= 1) {
        fprintf(stderr, "Usage: fdlinecombine fd1 fd2 ... fdN\n");
        return 0; /* No fds (therefore no output) is correct degenerate case */
    }

    numfds = argc-1;
    num_active_fds = numfds;
    fds = (struct fdinfo*) malloc(numfds*sizeof(*fds));
    if (fds==NULL) {
        perror("malloc");
        return 1;
    }

    for(i=1; i<argc; ++i) {
        struct fdinfo* f = fds+(i-1);
        f->fd = atoi(argv[i]);
        f->buffer = (char*) malloc(DEFAULT_READ_SIZE);
        if (!f->buffer) {
            perror("malloc");
            return 1;
        }
        f->bufsize = DEFAULT_READ_SIZE;
        f->offset = 0;
    }

    for(;;) { /* select loop */
        FD_ZERO(&rfds);
        maxfd=0;
        for(i=0; i<numfds; ++i) {
            struct fdinfo* f = fds+i;
            if (f->fd >= 0) {
                FD_SET(f->fd, &rfds);
                if(maxfd < f->fd+1) maxfd = f->fd+1;
                fcntl(f->fd, F_SETFL, O_NONBLOCK);
            }
        }

        sret = select(maxfd, &rfds, NULL, NULL, NULL);

        if (sret==-1) {
            perror("select");
            return 1;
        }

        for(i=0; i<numfds; ++i) {
            struct fdinfo* f = fds+i;

            if(f->fd==-1) continue;
            if(!FD_ISSET(f->fd, &rfds)) continue;

            int j;
            int offset = f->offset;
            char* buffer = f->buffer;

            ret = read(f->fd, buffer+offset, f->bufsize-offset);
            if (ret==-1) {
                if (errno == EAGAIN || errno == EINTR) {
                    continue;
                }
                dbgprintf(stderr, "fd %d failed\n", f->fd);
                closefd(f);
                continue;
            } else
            if (ret == 0) {
                closefd(f);
                continue;
            }

            /* Manage buffer size to keep output fast (can dynamically shrink and enlarge buffers) */
            if (ret == f->bufsize - f->offset) {
                /* Maximum possible bytes received. Better to enlarge buffer. */
                --f->little_data_hyster;

                /* Don't enlarge buffer just to read more than megabyte chunks */
                /* Also don't enlarge buffer here upon the first request - enlarge if it is trend */
                if(ret < 1024*1024 && f->little_data_hyster<-3) {
                    dbgprintf(stderr, "Much data: enlarging buffer from %d to %d\n", f->bufsize, f->bufsize*2);
                    buffer = realloc_buffer(f, f->bufsize*2, 0); 
                }
            } else {
                if (f->little_data_hyster<0) f->little_data_hyster=0;
            }
            
            /* Buffer is too large - can shrink */
            if ( (ret < (f->bufsize - offset)/4) && (f->bufsize>DEFAULT_READ_SIZE*2) ) {
                if (offset < DEFAULT_READ_SIZE) { // Don't shrink if it is just a looong line
                    ++f->little_data_hyster;
                    if(f->little_data_hyster>5) { // Don't shink-grow-shink-grow the buffer rapidly
                        dbgprintf(stderr, "Little data: shrinking buffer from %d to %d\n", 
                                f->bufsize, offset+ret+DEFAULT_READ_SIZE);
                        buffer = realloc_buffer(f, offset+ret+DEFAULT_READ_SIZE, 0); 
                    }
                }
            } else {
                if(f->little_data_hyster>0) f->little_data_hyster=0;
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
                    f->offset = offset = ret-j-1;
                    break;
                }
            }
            if(j==-1) {
                /* No newline in newly received data */
                offset+=ret;
                f->offset = offset;
            }

            /* enlarge buffer if we don't have space to read more data
             *     (used if too long lines)
             */
            if(f->bufsize - offset < DEFAULT_READ_SIZE && f->bufsize>0) {
                dbgprintf(stderr, "Long line: enlarging buffer from %d to %d\n", f->bufsize, f->bufsize * 2);
                buffer = realloc_buffer(f, f->bufsize * 2, 1); 
            }

            if (f->bufsize==-1) {
                closefd(f);
            }

        } /* fds loops */

        if (num_active_fds==0) return 0; /* All inputs finished */
    } /* main loop */


    free(fds);
}
