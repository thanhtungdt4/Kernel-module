#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>

#define DEVICE_PATH "/dev/bt_device"
#define MAX_LEN 256

int running = 1;

void *reader_thread(void *arg) {
    int fd = *(int *)arg;
    struct pollfd fds;
    char buffer[MAX_LEN];

    fds.fd = fd;
    fds.events = POLLIN;

    while (running) {
        int ret = poll(&fds, 1, 500); // timeout = 500ms
        if (ret > 0 && (fds.revents & POLLIN)) {
            int len = read(fd, buffer, sizeof(buffer) - 1);
            if (len > 0) {
                buffer[len] = '\0';
                printf("\n[BT] %s\n# ", buffer); // echo incoming BT data
                fflush(stdout);
            }
        }
    }

    return NULL;
}

int main() {
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/bt_device");
        return 1;
    }

    pthread_t tid;
    if (pthread_create(&tid, NULL, reader_thread, &fd) != 0) {
        perror("Failed to create reader thread");
        close(fd);
        return 1;
    }

    char input[MAX_LEN];
    printf("Bluetooth Console (type 'exit' to quit)\n");

    while (1) {
        printf("# ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "exit") == 0)
            break;

        ssize_t written = write(fd, input, strlen(input));
        if (written < 0) {
            perror("Write failed");
        }
    }

    running = 0;
    pthread_join(tid, NULL);
    close(fd);
    return 0;
}