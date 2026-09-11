/* SPDX-License-Identifier: Apache-2.0 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define AERA_MAGIC 0x41325049U
#define AERA_API 2U
#define AERA_PRIMARY 1U

enum kind {
  HELLO = 1, BEGIN_PAGE, ADD_BUTTON, COMMIT_PAGE, SET_STATUS,
  REQUEST_OPERATION, CLOSE_WORKER,
  HELLO_ACK = 64, ACTION, LIFECYCLE, OPERATION_RESULT
};
enum operation { BACKUP_SETTINGS = 1, RESTORE_SETTINGS = 2 };

struct message {
  uint32_t magic, version, kind, request_id, value, flags;
  char title[96];
  char text[1024];
};

static int send_message(int fd, uint32_t kind, uint32_t request,
                        uint32_t value, uint32_t flags,
                        const char *title, const char *text) {
  struct message message = {AERA_MAGIC, AERA_API, kind, request, value, flags,
                            {0}, {0}};
  if (title) snprintf(message.title, sizeof(message.title), "%s", title);
  if (text) snprintf(message.text, sizeof(message.text), "%s", text);
  ssize_t sent;
  do {
    sent = send(fd, &message, sizeof(message), MSG_NOSIGNAL);
  } while (sent < 0 && errno == EINTR);
  return sent == (ssize_t)sizeof(message) ? 0 : -1;
}

static int valid(const struct message *message) {
  return message->magic == AERA_MAGIC && message->version == AERA_API &&
         memchr(message->title, 0, sizeof(message->title)) &&
         memchr(message->text, 0, sizeof(message->text));
}

static int publish_page(int fd) {
  return send_message(fd, BEGIN_PAGE, 0, 0, 0, "Settings Backup",
      "Back up or restore AERA Recovery preferences. The isolated plugin never "
      "receives the settings file; AERA performs each approved operation.") ||
    send_message(fd, ADD_BUTTON, 1, 0, AERA_PRIMARY,
                 "Back up settings", "Save the current recovery preferences") ||
    send_message(fd, ADD_BUTTON, 2, 0, 0,
                 "Restore settings", "Use the most recent settings backup") ||
    send_message(fd, COMMIT_PAGE, 0, 0, 0, 0, 0);
}

int main(void) {
  const int fd = 4;
  if (send_message(fd, HELLO, 0, AERA_API, AERA_API, 0,
                   "Settings Backup API 2 example")) return 78;
  uint32_t operation_request = 100;
  int page_published = 0;
  for (;;) {
    struct message message;
    ssize_t count;
    do {
      count = recv(fd, &message, sizeof(message), MSG_TRUNC);
    } while (count < 0 && errno == EINTR);
    if (count != (ssize_t)sizeof(message) || !valid(&message)) return 78;
    if (message.kind == HELLO_ACK) continue;
    if (message.kind == LIFECYCLE) {
      if (message.value == 3) return 0;
      if (message.value == 1 && !page_published) {
        if (publish_page(fd)) return 78;
        page_published = 1;
      }
      continue;
    }
    if (message.kind == ACTION && (message.request_id == 1 ||
                                   message.request_id == 2)) {
      const uint32_t operation = message.request_id == 1
          ? BACKUP_SETTINGS : RESTORE_SETTINGS;
      if (send_message(fd, REQUEST_OPERATION, ++operation_request, operation,
                       0, 0, 0)) return 78;
      continue;
    }
    if (message.kind == OPERATION_RESULT) {
      if (send_message(fd, SET_STATUS, 0, 0, 0, 0, message.text)) return 78;
      continue;
    }
    if (message.kind == CLOSE_WORKER) return 0;
  }
}
