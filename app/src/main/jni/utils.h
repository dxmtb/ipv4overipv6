//
// Created by Bo Tian on 4/19/16.
//

#ifndef IPV4OVERIPV6_UTILS_H
#define IPV4OVERIPV6_UTILS_H

#include <android/log.h>
#include <stdlib.h>

#define LOG_TAG "4Over6Backend"

#define LOGE(...) do { __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__); exit(-1); } while (1)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

ssize_t write_s(int fd, const void *buf, size_t count);
void send_int(int sockfd, int payload);
void send_char(int sockfd, char payload);
void send_server_message(int sockfd, char type, void *payload, int payload_len);

void read_msg(int fd, char *type, void *payload, int *payload_length);
void read_len(int fd, void *buf, int len);
#define read_auto(fd, buf) read_len(fd, buf, sizeof(*buf));

int connect_to_server(const char *addr, int port);

#endif //IPV4OVERIPV6_UTILS_H
