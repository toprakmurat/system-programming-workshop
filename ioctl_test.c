#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define PSEUDO_INC _IOW('p', 1, signed char)
#define DEVICE_FILE "/dev/pseudo"

int main() {
    int fd;
    int8_t increment = 5; // Example increment value

    // Open the device file
    fd = open(DEVICE_FILE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device file");
        return 1;
    }

    // Invoke the PSEUDO_INC ioctl
    if (ioctl(fd, PSEUDO_INC, &increment) < 0) {
        perror("PSEUDO_INC ioctl failed");
        close(fd);
        return 1;
    }

    // Close the device file
    close(fd);

    printf("PSEUDO_INC ioctl completed successfully\n");

    return 0;
}
