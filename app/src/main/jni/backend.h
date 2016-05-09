//
// Created by Bo Tian on 4/19/16.
//

#ifndef IPV4OVERIPV6_BACKEND_H
#define IPV4OVERIPV6_BACKEND_H

struct FileDescriptors {
    int server_fd, tun_fd, traffic_info_fd, timer_fd;
};

char* init(struct FileDescriptors *fds);
void start(struct FileDescriptors *fds);

#endif //IPV4OVERIPV6_BACKEND_H
