#define _XOPEN_SOURCE 600
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>


static const int MBYTE = 1024 * 1024;

typedef struct b_buff {
        char buff [1024 * 1024];
        int free_space;
        int count;
        char *wp;
} b_buff;

static inline void init_buff (b_buff *const buf)
{
        memset (buf, 0, sizeof (b_buff));
        buf->free_space = MBYTE;
        buf->wp = buf->buff;
}

static inline size_t uncached_write (int fd, const void *buf, size_t count)
{
        const size_t n = write (fd, buf, count);
        fdatasync (fd);
        posix_fadvise (fd, 0, 0, POSIX_FADV_DONTNEED);

        return n;
}


int main (int argc, char *argv[])
{
        if ( argc < 2 ) {
                return 1;
        }
        const char *const fn = argv [1];
        int fd = open (fn, O_WRONLY | O_CREAT, 0600);
        if ( -1 == fd ) {
                int errsv = errno;
                printf ("ERROR! Failed to open %s!\n", fn);

                return errsv;
        }

        b_buff disk_buff;
        init_buff (&disk_buff);

        int n = 0;
        do {
                disk_buff.free_space -= n;
                disk_buff.count += n;

                if ( 0 == disk_buff.free_space ) {
                        const size_t ret = uncached_write (fd, disk_buff.buff, disk_buff.count);
                        if ( -1 == ret ) {
                                int errsv = errno;
                                printf ("ERROR! Write to file failed! Data maybe lost!\n");
                                return errsv;
                        }

                        init_buff (&disk_buff);
                        n = 0;
                }

                disk_buff.wp = disk_buff.buff + disk_buff.count;
                n = read (STDIN_FILENO, disk_buff.wp, disk_buff.free_space);
                if ( n < 0 ) {
                        int errsv = errno;
                        printf ("ERROR! Read from file failed! Data maybe lost!\n");
                        return errsv;
                        break;
                }
        } while ( n > 0 );

        if ( disk_buff.count != 0 ) {
                const size_t ret = uncached_write (fd, disk_buff.buff, disk_buff.count);
                if ( -1 == ret ) {
                        int errsv = errno;
                        printf ("ERROR! Write to file failed! Data maybe lost!\n");
                        return errsv;
                }

        }
        init_buff (&disk_buff);

        close (fd);
        return 0;
}

