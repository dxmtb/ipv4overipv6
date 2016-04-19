#include <time.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "utils.h"

struct FileDescriptors {
    int server_fd, tun_fd, ip_info_fd, traffic_info_fd, timer_fd;
};

static clock_t last_heartbeat;

void handle_heartbeat() {
    last_heartbeat = clock();
}

void handle_network_response(const struct FileDescriptors *fds, char *payload, int payload_length) {
    write_s(fds->tun_fd, payload, payload_length);
    write_s(fds->traffic_info_fd, &payload_length, sizeof(payload_length));
}

void handle_server_msg(const struct FileDescriptors *fds) {
    int type, payload_length;
    char payload[4096];
    read_msg(fds->server_fd, &type, payload, &payload_length);
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
    uint64_t exp;
    read_auto(fds->timer_fd, &exp);

    int sec = (clock() - last_heartbeat) / CLOCKS_PER_SEC;
    if (sec > MAX_HEARTBEAT_SEC) {
        close(fds->server_fd);
        return false;
    }

    return true;
}

void handle_tun_msg(const struct FileDescriptors *fds) {
    char payload[MAX_BUF_LEN];

    int payload_length = read(fds->tun_fd, payload, sizeof(payload));

    send_server_message(fds->server_fd, TYPE_NETWORK_REQUEST, payload, payload_length);
}

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
        LOGE("event_loop_add_fd");
}

int create_timer_fd() {
    int fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (fd == -1)
        LOGE("timerfd_create");

    struct itimerspec value;
    memset(&value, 0, sizeof(value));
    value.it_interval.tv_sec = 1;
    value.it_value.tv_sec = 1;

    if (timerfd_settime(fd, TFD_TIMER_ABSTIME, &value, NULL) == -1)
        LOGE("timerfd_settime");
}

int read_tun_fd(int fd) {
    int ret;
    read_auto(fd, &ret);
    return ret;
}

void start(struct FileDescriptors *fds) {
    fds->tun_fd = read_tun_fd(fds->ip_info_fd);
    close(fds->ip_info_fd);
    fds->ip_info_fd = -1;

    int event_loop = create_event_loop();

    fds->timer_fd = create_timer_fd();

    event_loop_add_fd(event_loop, fds->server_fd);
    event_loop_add_fd(event_loop, fds->tun_fd);
    event_loop_add_fd(event_loop, fds->timer_fd);

    run_event_loop(event_loop, fds);
}

int connect_to_server(const char *ip_addr, int port) {
    int sockfd = socket(PF_INET6, SOCK_STREAM, 0);

    if (sockfd == -1) {
        LOGE("Failed to create socket\n");
    }

    struct sockaddr_in6 addr;
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(PORT);

    if (inet_pton(AF_INET6, ip_addr, &addr.sin6_addr) < 0) {
        LOGE("Failed in inet_pton");
    }

    if (connect(sockfd, &addr, sizeof(addr)) < 0) {
        LOGE("Failed to connect to [%s]:%d\n", SERVER_IP, PORT);
        exit(0);
    }

    return sockfd;
}


void handle_ip_response(int ip_info_fd, char *payload, int payload_length) {
    write_s(ip_info_fd, payload, payload_length);
}

struct FileDescriptors *init() {
    // Create pipes
    mknod(IP_INFO_PIPE, S_IFIFO | 0666, 0);
    mknod(TRAFFIC_INFO_PIPE, S_IFIFO | 0666, 0);

    static struct FileDescriptors fds;

    fds.server_fd = connect_to_server(SERVER_IP, PORT);

    // Request ipv4 addr
    send_server_message(fds.server_fd, 100, NULL, 0);

    int type, payload_length;
    char payload[4096];

    read_msg(fds.server_fd, &type, payload, &payload_length);

    if (type != 101) {
        LOGE("unexpected response type: %d\n", type);
        return NULL;
    }

    fds.ip_info_fd = open(IP_INFO_PIPE, O_RDWR|O_CREAT|O_TRUNC);
    if (fds.ip_info_fd == -1)
        LOGE("Failed to open %s", IP_INFO_PIPE);
    handle_ip_response(fds.ip_info_fd, payload, payload_length);
    handle_heartbeat();

    fds.ip_info_fd = open(TRAFFIC_INFO_PIPE, O_RDWR|O_CREAT|O_TRUNC);
    if (fds.ip_info_fd == -1)
        LOGE("Failed to open %s", TRAFFIC_INFO_PIPE);

    return &fds;
}
