#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <string.h>

#include "utils.h"
#include "config.h"

struct Message {
  int length;
  char type;
  char data[4096];
};

ssize_t write_s(int fd, const void *buf, size_t count) {
  ssize_t ret = write(fd, buf, count);

  if (ret == -1) {
    LOGE("Failed when write to fd %d count %zu ret %zd", fd, count, ret);
    exit(0);
  }

  return ret;
}

void send_server_message(int sockfd, char type, void *payload, int payload_len) {
  static struct Message msg;

  if (payload_len > 0) {
    if (!payload) {
      LOGE("Payload is null but paylaod_len is %d", payload_len);
      return;
    }
    if (payload_len > sizeof(msg.data)) {
      LOGE("Payload len too large: %d\n", payload_len);
      return;
    }
  }

  msg.length = sizeof(int) + sizeof(char) + payload_len;
  msg.type = type;
  memcpy(msg.data, payload, payload_len);

  write_s(sockfd, &msg, msg.length);
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
