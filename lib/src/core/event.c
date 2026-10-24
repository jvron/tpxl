#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <poll.h>
    #include <unistd.h>
#endif

#include "tpxl/event.h"
#include "tpxl/type.h"

TpxlResult tpxl_poll_event(TpxlEvent* event) {
    if (!event) {
        return TPXL_INVALID_ARGUMENT;
    }

    event->type = TPXL_EVENT_NONE;

#ifdef _WIN32

    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD available = 0;

    if (input == INVALID_HANDLE_VALUE || input == NULL) {
        return TPXL_IO_ERROR;
    }

    if (!GetNumberOfConsoleInputEvents(input, &available)) {
        return TPXL_IO_ERROR;
    }

    while (available > 0) {
        INPUT_RECORD record;
        DWORD count = 0;

        if (!ReadConsoleInputW(input, &record, 1, &count)) {
            return TPXL_IO_ERROR;
        }

        if (count != 1) {
            return TPXL_IO_ERROR;
        }

        if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown) {
            WCHAR c = record.Event.KeyEvent.uChar.UnicodeChar;

            if (c == 0) {
                if (!GetNumberOfConsoleInputEvents(input, &available)) {
                    return TPXL_IO_ERROR;
                }
                continue;
            }

            event->type = TPXL_EVENT_KEY;

            switch (c) {
                case L'q': event->key = TPXL_KEY_Q; break;
                case L'w': event->key = TPXL_KEY_W; break;
                case L'a': event->key = TPXL_KEY_A; break;
                case L's': event->key = TPXL_KEY_S; break;
                case L'd': event->key = TPXL_KEY_D; break;
                case L'h': event->key = TPXL_KEY_H; break;
                case L'm': event->key = TPXL_KEY_M; break;
                case L'p': event->key = TPXL_KEY_P; break;
                case L' ': event->key = TPXL_KEY_SPACE; break;

                default:
                    event->key = TPXL_KEY_UNKNOWN;
                    break;
            }

            return TPXL_OK;
        }

        if (!GetNumberOfConsoleInputEvents(input, &available)) {
            return TPXL_IO_ERROR;
        }
    }

    return TPXL_OK;

#else

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

        if (read_result != 1) {
            return TPXL_OK;
        }

        event->type = TPXL_EVENT_KEY;

        switch (c) {
            case 'q': event->key = TPXL_KEY_Q; break;
            case 'w': event->key = TPXL_KEY_W; break;
            case 'a': event->key = TPXL_KEY_A; break;
            case 's': event->key = TPXL_KEY_S; break;
            case 'd': event->key = TPXL_KEY_D; break;
            case 'h': event->key = TPXL_KEY_H; break;
            case 'm': event->key = TPXL_KEY_M; break;
            case 'p': event->key = TPXL_KEY_P; break;
            case ' ': event->key = TPXL_KEY_SPACE; break;

            default: event->key = TPXL_KEY_UNKNOWN; break;
        }
    }

    return TPXL_OK;

#endif
}
