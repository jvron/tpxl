#include <poll.h>
#include <unistd.h>

#include "tpxl/event.h"
#include "tpxl/type.h"

TpxlResult tpxl_poll_event(TpxlEvent* event) {

    if (!event) {
        return TPXL_INVALID_ARGUMENT;
    }

    event->type = TPXL_EVENT_NONE;

    struct pollfd pfd = {
        .fd = STDIN_FILENO,
        .events = POLLIN,
        .revents = 0,
    };

    int poll_result = poll(&pfd, 1, 0);

    if (poll_result < 0) {
        return TPXL_IO_ERROR;
    }

    if (poll_result == 0) {
        return TPXL_OK;
    }

    if (pfd.revents & POLLIN) {

        char c;

        ssize_t read_result = read(STDIN_FILENO, &c, 1);

        if (read_result < 0) {
            return TPXL_IO_ERROR;
        }

        if (read_result == 1) {

            event->type = TPXL_EVENT_KEY;

            switch (c) {
                case 'q': event->key = TPXL_KEY_Q; break;
                case 'w': event->key = TPXL_KEY_W; break;
                case 'a': event->key = TPXL_KEY_A; break;
                case 's': event->key = TPXL_KEY_S; break;
                case 'd': event->key = TPXL_KEY_D; break;

                default:
                    event->key = TPXL_KEY_UNKNOWN;
                    break;
            }
        }
    }

    return TPXL_OK;
}

