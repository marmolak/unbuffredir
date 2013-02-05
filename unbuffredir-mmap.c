#define _XOPEN_SOURCE 600
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdlib.h>


static const int MBYTE = 1024 * 1024;

typedef struct b_buff {
        char *buff;
        char *wp;
	off_t flen;
} b_buff;

static inline void init_buff (b_buff *const buf)
{
        memset (buf, 0, sizeof (b_buff));
}

static inline void commit (const int fd)
{
        fdatasync (fd);
        posix_fadvise (fd, 0, 0, POSIX_FADV_DONTNEED);
}

static inline int fallocate (const int fd)
{
	const off_t r = lseek (fd, MBYTE-1, SEEK_END);
	if ( r < 0 ) {
		return -1;
	}

	const ssize_t n = write (fd, "0", 1);
	if ( n < n ) {
		return -1;
	}

	return 1;
}

static inline int fmap (const int fd, b_buff *const buf, const off_t offset)
{
	buf->buff = mmap (NULL, MBYTE, PROT_WRITE | PROT_READ, MAP_SHARED, fd, offset);
	if ( buf->buff == MAP_FAILED ) {
                int errsv = errno;
                printf ("ERROR! Failed to map file!\n");

                return errsv;
	}
        buf->wp = buf->buff;
	return 1;
}

int main (int argc, char *argv[])
{
        if ( argc < 2 ) {
                return 1;
        }
        const char *const fn = argv [1];
	const int flags = O_RDWR | O_CREAT | O_TRUNC;
        int fd = open (fn, flags, 0600);
        if ( -1 == fd ) {
                printf ("ERROR! Failed to open %s!\n", fn);
                return EXIT_FAILURE;
        }
	if ( fallocate (fd) < 0 ) {
		printf ("Lseek failed! Data maybe lost!\n");
		return EXIT_FAILURE;
	}

        b_buff disk_buff;
        init_buff (&disk_buff);
	if ( fmap (fd, &disk_buff, disk_buff.flen) < 0 ) {
		printf ("Map of file failed! Data maybe lost!\n");
		return EXIT_FAILURE;
	}

	ssize_t n = 0;
	ssize_t fsp = MBYTE - (disk_buff.wp - disk_buff.buff);
	do {
		n = read (STDIN_FILENO, disk_buff.wp, fsp);
		if ( n < 0 ) {
			printf ("Read failed. Data maybe lost!\n");
			if ( munmap (disk_buff.buff, MBYTE) == -1 ) {
				printf ("Unmap failed! Data maybe lost!\n");
			}
			close (fd);
			return EXIT_FAILURE;
		}
		commit (fd);

		disk_buff.flen += n;
		disk_buff.wp += n;

		fsp = MBYTE - (disk_buff.wp - disk_buff.buff);
		if ( 0 == fsp ) {
			if ( munmap (disk_buff.buff, MBYTE) == -1 ) {
				printf ("Unmap failed! Data maybe lost!\n");
				return EXIT_FAILURE;
			}
			if ( fallocate (fd) < 0 ) {
				printf ("Lseek failed! Data maybe lost!\n");
				return EXIT_FAILURE;
			}
			if ( fmap (fd, &disk_buff, disk_buff.flen) < 0 ) {
				printf ("Map of file failed! Data maybe lost!\n");
				return EXIT_FAILURE;
			}
			fsp = MBYTE;
		}

	} while ( n > 0 );

	ftruncate (fd, disk_buff.flen);

        return EXIT_SUCCESS;
}

