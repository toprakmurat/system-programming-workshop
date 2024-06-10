#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd;
    off_t new_offset;

    // Open a file or device (replace "/path/to/your/file" with the actual path)
    fd = open("/dev/pseudo", O_RDWR);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // Seek to a new offset
    new_offset = lseek(fd, 100, SEEK_SET);
    if (new_offset == (off_t)-1) {
        perror("llseek");
        close(fd);
        return 1;
    }
    printf("New offset after seek: %lld\n", (long long)new_offset);

    // Close the file or device
    close(fd);

    return 0;
}
