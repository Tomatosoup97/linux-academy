#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define DUMMY_CHAR_ERASE _IO('d', 0x01)
#define DUMMY_CHAR_SIZE _IOR('d', 0x02, size_t)

#define DEVICE_FILE "/dev/my-dummy"

void ioctl_device_erase(int dummy_fd) {
  int rc;
  rc = ioctl(dummy_fd,DUMMY_CHAR_ERASE);

  if (rc < 0) {
    perror("ioctl");
    exit(EXIT_FAILURE);
  }

  printf("[ioctl] dummy char erased\n");
}

size_t ioctl_device_read_size(int dummy_fd) {
  int rc;
  size_t dummy_size;
  rc = ioctl(dummy_fd,DUMMY_CHAR_SIZE,&dummy_size);

  if (rc < 0) {
    perror("ioctl");
    exit(EXIT_FAILURE);
  }

  printf("[ioctl] dummy char size: %zu\n", dummy_size);
  return dummy_size;
}

void device_read(FILE *file) {
  int bytes_read;
  const int BYTES_TO_READ = 1024;
  char buffer[BYTES_TO_READ];

  bytes_read = fread(buffer, sizeof(char), BYTES_TO_READ - 1, file);
  if (bytes_read != BYTES_TO_READ-1) {
    perror("device fread failed");
    exit(EXIT_FAILURE);
  }
  buffer[bytes_read] = '\0';

  printf("[dummy char] read:\n%s", buffer);
}

void device_write(FILE *file) {
    int bytes_written;
    char buffer[] = "test write\n";

    bytes_written = fwrite(buffer, sizeof(char), strlen(buffer), file);

    if (bytes_written != strlen(buffer)) {
        perror("device fwrite failed");
    }

    printf("[dummy char] written:\n%s", buffer);
}

int main() {
    int fd;
    FILE *file;

    file = fopen(DEVICE_FILE, "r+");
    fd = fileno(file);

    if (!file) {
        perror("Failed to open device file");
        return 1;
    }
    if (fd < 0) {
        perror("Failed to get file descriptor");
        return 1;
    }

    ioctl_device_erase(fd);
    device_write(file);
    ioctl_device_read_size(fd);

    fclose(file);

    file = fopen(DEVICE_FILE, "r+");

    if (!file) {
        perror("Failed to open device file");
        return 1;
    }

    device_read(file);

    fclose(file);

    return 0;
}
