#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

// Build Switch: Set to 1 for Assignment 8, 0 for Assignment 5
#ifndef USE_AESD_CHAR_DEVICE
#define USE_AESD_CHAR_DEVICE 1
#endif

#if USE_AESD_CHAR_DEVICE
    #define FILENAME "/dev/aesdchar"
#else
    #define FILENAME "/var/tmp/aesdsocketdata"
#endif

#define PORT 9000
#define BACKLOG 10

typedef struct pthread_arg_t {
    int new_socket_fd;
    struct sockaddr_in client_address;
} pthread_arg_t;

int socket_fd = -1;
pthread_mutex_t file_mutex;
atomic_bool time_stamp_trigger = 0;

void *pthread_routine(void *arg);
void signal_handler(int sig, siginfo_t *si, void *uc);
static void timer_setup();

int main(int argc, char *argv[]) {
    int new_socket_fd;
    struct sockaddr_in address;
    pthread_attr_t pthread_attr;
    socklen_t client_address_len;
    int yes = 1;

    openlog("aesdsocket", LOG_PID, LOG_USER);

    struct sigaction sa = {0};
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = signal_handler;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGRTMIN, &sa, NULL);

    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        syslog(LOG_ERR, "Socket creation failed");
        return -1;
    }

    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        syslog(LOG_ERR, "Bind failed: %m");
        return -1;
    }

    if (listen(socket_fd, BACKLOG) == -1) {
        syslog(LOG_ERR, "Listen failed");
        return -1;
    }

    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        daemon(0, 0);
    }

    pthread_mutex_init(&file_mutex, NULL);
    pthread_attr_init(&pthread_attr);
    pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED);

#if (USE_AESD_CHAR_DEVICE == 0)
    timer_setup();
#endif

    while (1) {
#if (USE_AESD_CHAR_DEVICE == 0)
        if (time_stamp_trigger) {
            time_t rawtime;
            struct tm *info;
            char buffer[64];
            time(&rawtime);
            info = localtime(&rawtime);
            strftime(buffer, sizeof(buffer), "timestamp:%F %H:%M:%S\n", info);
            
            int fd = open(FILENAME, O_APPEND | O_WRONLY | O_CREAT, 0644);
            pthread_mutex_lock(&file_mutex);
            write(fd, buffer, strlen(buffer));
            pthread_mutex_unlock(&file_mutex);
            close(fd);
            time_stamp_trigger = 0;
        }
#endif

        pthread_arg_t *pthread_arg = malloc(sizeof(pthread_arg_t));
        client_address_len = sizeof(pthread_arg->client_address);
        new_socket_fd = accept(socket_fd, (struct sockaddr *)&pthread_arg->client_address, &client_address_len);
        
        if (new_socket_fd != -1) {
            pthread_arg->new_socket_fd = new_socket_fd;
            pthread_t thread;
            pthread_create(&thread, &pthread_attr, pthread_routine, (void *)pthread_arg);
        } else {
            free(pthread_arg);
        }
    }
    return 0;
}

void *pthread_routine(void *arg) {
    pthread_arg_t *parg = (pthread_arg_t *)arg;
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &parg->client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
    syslog(LOG_DEBUG, "Accepted connection from %s", client_ip);

    char *recv_buf = malloc(1024);
    size_t buffer_size = 1024;
    size_t total_bytes = 0;
    ssize_t bytes_recv;

    while ((bytes_recv = recv(parg->new_socket_fd, recv_buf + total_bytes, 1024, 0)) > 0) {
        total_bytes += bytes_recv;
        
        // Check if we reached the end of a packet
        if (recv_buf[total_bytes - 1] == '\n') {
            pthread_mutex_lock(&file_mutex);
            
            // OPEN - WRITE - READ - CLOSE pattern for Driver compatibility
            int fd = open(FILENAME, O_RDWR | O_APPEND | O_CREAT, 0644);
            write(fd, recv_buf, total_bytes);

            // Read back the entire content
            lseek(fd, 0, SEEK_SET);
            char read_buf[1024];
            ssize_t bytes_read;
            while ((bytes_read = read(fd, read_buf, sizeof(read_buf))) > 0) {
                send(parg->new_socket_fd, read_buf, bytes_read, 0);
            }
            close(fd); 
            
            pthread_mutex_unlock(&file_mutex);
            total_bytes = 0; // Reset for next packet
            break; // Assignment specifies closing connection after newline exchange
        }
        
        // Expand buffer if needed
        buffer_size += 1024;
        recv_buf = realloc(recv_buf, buffer_size);
    }

    close(parg->new_socket_fd);
    free(recv_buf);
    free(parg);
    syslog(LOG_DEBUG, "Closed connection from %s", client_ip);
    return NULL;
}

void signal_handler(int sig, siginfo_t *si, void *uc) {
    if (si->si_code == SI_TIMER) {
        time_stamp_trigger = 1;
    } else {
        syslog(LOG_DEBUG, "Caught signal, exiting");
        if (socket_fd >= 0) close(socket_fd);
#if (USE_AESD_CHAR_DEVICE == 0)
        unlink(FILENAME);
#endif
        exit(0);
    }
}

static void timer_setup() {
    timer_t timerid;
    struct sigevent sev = {0};
    struct itimerspec its;

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;

    timer_create(CLOCK_REALTIME, &sev, &timerid);
    its.it_value.tv_sec = 10;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 10;
    its.it_interval.tv_nsec = 0;
    timer_settime(timerid, 0, &its, NULL);
}
