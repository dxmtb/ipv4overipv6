#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "utils.h"
#include "config.h"

ssize_t write_s(int fd, const void *buf, size_t count) {
  ssize_t ret = write(fd, buf, count);

  if (ret == -1) {
    LOGE("Failed when write to fd %d count %zu ret %zd", fd, count, ret);
    exit(0);
  }

  return ret;
}

void send_int(int sockfd, int payload) {
  payload = htons(payload);
  write_s(sockfd, &payload, sizeof(payload));
}

void send_char(int sockfd, char payload) {
  write_s(sockfd, &payload, sizeof(payload));
}

void send_server_message(int sockfd, char type, void *payload, int payload_len) {
  int total_length = sizeof(int) + sizeof(char) + payload_len;

  send_int(sockfd, total_length);
  send_char(sockfd, type);

  if (payload_len > 0) {
    if (!payload) {
      LOGE("Payload is null but paylaod_len is %d", payload_len);
      return;
    }
    write_s(sockfd, payload, payload_len);
  }
}

void read_len(int fd, void *buf, int len) {
    while (len > 0) {
        int ret = read(fd, buf, len);
        if (ret == -1)
            LOGE("failed to read len");
        len -= ret;
        buf += ret;
    }
}

void read_msg(int fd, char *type, void *payload, int *payload_length) {
    read_auto(fd, payload_length);
    read_auto(fd, type);

    *payload_length -= sizeof(int) + sizeof(char);
    read_len(fd, payload, *payload_length);
}
