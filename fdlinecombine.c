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

char* separator="\n";
int separator_length = 1;


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
int no_chopped_data;

void closefd(struct fdinfo* f) {
    close(f->fd);
    FD_CLR(f->fd, &rfds);
    if(f->offset && !no_chopped_data) {
        /* Data without trailing end-of-line remaining. Flush them and add end-of-line ourselves. */
        fwrite(f->buffer, 1, f->offset, stdout);
        fwrite(separator, 1, separator_length, stdout);
    }
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

void write_and_move_tail(struct fdinfo* f, int length, int tail_length) {
    fwrite(f->buffer, 1, length, stdout);
    memmove(f->buffer, f->buffer+length, tail_length);
    f->offset = tail_length;
}

/* if SEPARATOR environment variable is set than read that file handle (or name) for line separator value */
void read_separator(const char* sepenv) {
    int fd;
    int ret;
    if(!sscanf(sepenv, "%i", &fd)) {
        open_again:
        fd = open(sepenv, O_RDONLY, 0022);
        if(fd == -1) {
            if ( errno==EINTR || errno==EAGAIN ) goto open_again;
            fprintf(stderr, "%s ", sepenv);
            perror("open");
            fprintf(stderr, "SEPARATOR environment variable should be \n"
                    "    name of file or file descriptor to read separator value from\n");
            exit(2);
        }
    }
    FILE* f = fdopen(fd, "r");
    if(!f) {
        perror("fdopen");
        exit(1);
    }
    separator = (char*) malloc(1024);
    if(!separator) {
        perror("malloc");
        exit(1);
    }
    ret = fread(separator, 1, 1024, f);
    separator_length = ret;
    if(ret==1024) {
        separator = (char*) realloc(separator, 65500);
        if(!separator) {
            perror("realloc");
            exit(1);
        }
        ret = fread(separator+1024, 1, 65500-1024, f);
        if(ret == 65500-1024) {
            fprintf(stderr, "Separator is too long. Maximum separator size is 65499 bytes.");
            exit(3);
        }
        separator_length+=ret;
    }
    fclose(f);
}

int main(int argc, char* argv[]) {

    int i;
    int sret;
    int ret;
    int maxfd;

    const char* sepenv = getenv("SEPARATOR");
    if(sepenv) {
        read_separator(sepenv);
    }

    no_chopped_data = getenv("NO_CHOPPED_DATA") ? 1 : 0;

    if (argc <= 1) {
        fprintf(stderr, 
                "Usage: fdlinecombine fd1 fd2 ... fdN\n"
                "    Read multiple input streams and multiplex them into stdout\n"
                "    Parameters should be file descriptor numbers or file names\n"
                "\n"
                "    Environment variables:\n"
                "    SEPARATOR - fd number of file name to read separator from (default separator is single newline)\n"
                "    NO_CHOPPED_DATA - if set, don't output trailing line without separator at the end\n"
                "    \n"
                "    Advanced example (bash):\n"
                "    SEPARATOR=10   ./fdlinecombine 0 5 6 <(echo HEADER; echo ---) \\\n"
                "           5< <(nc -lp 9979 < /dev/null)   \\\n"
                "           6< <(perl -e '$|=1; sleep 2 and print \"TIMER\\n---\\n\" while true' < /dev/null) \\\n"
                "           10< <(printf -- '\\n---\\n') > out \n"
                "    This will transfer data from stdin, opened TCP port and periodic timer\n"
                "    to file \"out\" using \"---\" line as separator.\n"
                );
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
        if(!sscanf(argv[i], "%i", &f->fd)) {
            open_again:
            f->fd = open(argv[i], O_RDONLY, 0022);
            if(f->fd == -1) {
                if ( errno==EINTR || errno==EAGAIN ) goto open_again;
                fprintf(stderr, "%s ", argv[i]);
                perror("open");
                fprintf(stderr, "All arguments must be file descriptor numbers or file names\n");
                return 2;
            }
        }
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
 *     "debt"                newly_received_data          free_buffer
 * SSSSSSSSSSSSSSSSSSSSSSSSSSNNNNNNNNNNNNNseparatorNNNNNNN--------------
 * ^                         ^            ^        ^      ^             ^
 * 0                         offset       offset+j |      offset+ret   bufsize
 *                                                 |
 *                                                 offset+j+separator_length
 */


            /* Scan new data for newlines. Two implementations: fast (for single-character separator) and safe */
            if (separator_length==1) {
                /* use latest possible separator */
                for(j=ret-1; j!=-1; --j) {
                    if (buffer[offset+j]==*separator) {
                        write_and_move_tail(f, offset+j+separator_length, ret-j-separator_length);
                        offset = f->offset;
                        buffer = f->buffer;
                        fflush(stdout);
                        break;
                    }
                }
                if(j==-1) {
                    /* No newline in newly received data */
                    offset+=ret;
                    f->offset = offset;
                }
            } else {
                /* use earliest possible separator, repeat until not found */
                int found_flag=0;
                for(j=-separator_length+1; j<=(ret-separator_length); ++j) {
                    if(offset+j<0) continue;
                    if (!memcmp(buffer+offset+j, separator, separator_length)) {
                        write_and_move_tail(f, offset+j+separator_length, ret-j-separator_length);
                        buffer =  f->buffer;
                        offset = f->offset;
                        found_flag=1;
                        break;
                    }
                }
                if(found_flag) {
                    for(j=0; j<=offset-separator_length; ++j) {
                        if (!memcmp(buffer+j, separator, separator_length)) {
                            write_and_move_tail(f, j+separator_length, offset-separator_length-j);
                            buffer =  f->buffer;
                            offset = f->offset;
                            j=0;
                        }
                    }
                }
                if(found_flag) {
                    fflush(stdout);
                } else {
                    f->offset = offset+=ret;
                }
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
    if(sepenv) {
        free(separator);
    }
}
