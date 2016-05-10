#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __ANDROID__
#include <sys/epoll.h>
#include <sys/timerfd.h>
#endif

#include "config.h"
#include "utils.h"

#include "backend.h"

static time_t last_heartbeat;

void handle_heartbeat() {
    last_heartbeat = time(0);
}

void handle_network_response(const struct FileDescriptors *fds, char *payload, int payload_length) {
    write_s(fds->tun_fd, payload, payload_length);
    payload_length = -payload_length;
    write_s(fds->traffic_info_fd, &payload_length, sizeof(payload_length));
    LOGI("pack from server len %d", payload_length);
}

void handle_server_msg(const struct FileDescriptors *fds) {
    static char type, payload[MAX_BUF_LEN];
    int payload_length;
    read_msg(fds->server_fd, &type, payload, &payload_length);
    LOGI("handle_server_msg type %d", type);
    switch (type) {
        case TYPE_NETWORK_RESPONSE:
            handle_network_response(fds, payload, payload_length);
            break;
        case TYPE_HEARTBEAT:
            handle_heartbeat();
            break;
        default:
            LOGW("Unknown server message type: %d\n", type);
            break;
    }
}

bool handle_timer_msg(const struct FileDescriptors *fds) {
    LOGI("handle_timer_msg");
    uint64_t exp;
    read_auto(fds->timer_fd, &exp);

    double sec = difftime(time(0), last_heartbeat);
    if (sec > MAX_HEARTBEAT_SEC) {
        LOGW("heartbeat timeout");
        close(fds->server_fd);
        return false;
    }

    send_server_message(fds->server_fd, TYPE_HEARTBEAT, NULL, 0);

    return true;
}

void handle_tun_msg(const struct FileDescriptors *fds) {
    LOGI("handle_tun_msg");
    static char payload[MAX_BUF_LEN];

    int payload_length = read(fds->tun_fd, payload, sizeof(payload));

    send_server_message(fds->server_fd, TYPE_NETWORK_REQUEST, payload, payload_length);
    write_s(fds->traffic_info_fd, &payload_length, sizeof(payload_length));

    LOGI("pack from local len %d", payload_length);
}

int connect_to_server(const char *ip_addr, int port) {
    int sockfd = socket(PF_INET6, SOCK_STREAM, 0);

    if (sockfd == -1) {
        LOGE("Failed to create socket\n");
    }

    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);

    if (inet_pton(AF_INET6, ip_addr, &addr.sin6_addr) < 0) {
        LOGE("Failed in inet_pton");
    }

    if (connect(sockfd, (const struct sockaddr *) &addr, sizeof(addr)) < 0) {
        LOGE("Failed to connect to [%s]:%d\n", ip_addr, port);
        exit(0);
    }

    return sockfd;
}


#ifdef __ANDROID__
void run_event_loop(int loop, const struct FileDescriptors *fds) {
#define MAX_EVENTS 10
    struct epoll_event events[MAX_EVENTS];
    int i;

    for (;;) {
        int nfds = epoll_wait(loop, events, MAX_EVENTS, -1);
        if (nfds == -1)
            LOGE("epoll_wait");

        for (i = 0; i < nfds; i++) {
            int from = events[i].data.fd;
            if (from == fds->server_fd) {
                handle_server_msg(fds);
            } else if (from == fds->tun_fd) {
                handle_tun_msg(fds);
            } else if (from == fds->timer_fd) {
                if (!handle_timer_msg(fds)) {
                    return;
                }
            } else {
                LOGW("Message from unknown fd: ", from);
            }
        }
    }
}

int create_event_loop() {
    int epollfd = epoll_create1(0);
    if (epollfd == -1)
        LOGE("epoll_create1");
    return epollfd;
}

void event_loop_add_fd(int loop, int fd) {
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(loop, EPOLL_CTL_ADD, fd, &ev) == -1)
        LOGE("event_loop_add_fd, errno %d", errno);
}

int create_timer_fd() {
    int fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (fd == -1)
        LOGE("timerfd_create");

    struct itimerspec value;
    memset(&value, 0, sizeof(value));
    value.it_interval.tv_sec = 20;
    value.it_value.tv_sec = 20;

    if (timerfd_settime(fd, TFD_TIMER_ABSTIME, &value, NULL) == -1)
        LOGE("timerfd_settime");
    LOGI("timerfd %d", fd);

    return fd;
}

void start(struct FileDescriptors *fds) {
    int event_loop = create_event_loop();

    fds->timer_fd = create_timer_fd();

    event_loop_add_fd(event_loop, fds->server_fd);
    event_loop_add_fd(event_loop, fds->tun_fd);
    event_loop_add_fd(event_loop, fds->timer_fd);

    run_event_loop(event_loop, fds);
}

char* init(struct FileDescriptors *fds) {
    fds->server_fd = connect_to_server(SERVER_IP, PORT);

    // Request ipv4 addr
    send_server_message(fds->server_fd, 100, NULL, 0);

    int payload_length;
    static char type, payload[MAX_BUF_LEN];

    read_msg(fds->server_fd, &type, payload, &payload_length);

    if (type != 101) {
        LOGE("unexpected response type: %d\n", type);
        return NULL;
    }

    payload[payload_length] = 0;

    handle_heartbeat(); // init heartbeat time.

    const char *fifo = "/data/user/0/com.troublemaker.ipv4overipv6/files/"
            TRAFFIC_INFO_PIPE;
    mkfifo(fifo, 0666);
    fds->traffic_info_fd = open(fifo, O_RDWR|O_CREAT|O_TRUNC);
    if (fds->traffic_info_fd == -1)
        LOGE("Failed to open %s", TRAFFIC_INFO_PIPE);

    return payload;
}
#endif
