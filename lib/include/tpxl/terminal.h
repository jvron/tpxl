#ifndef TPXL_TERMINAL_H
#define TPXL_TERMINAL_H

#include <stdint.h>
#include <stdbool.h>

#include "type.h"

typedef struct {
    bool initialized;

    uint32_t rows;
    uint32_t columns;

    uint32_t cell_width;
    uint32_t cell_height;

    uint32_t pixel_width;
    uint32_t pixel_height;

    uint32_t cursor_row;
    uint32_t cursor_column;
} TpxlTerminal;

/**
 * Initializes terminal handling.
 *
 * This saves the terminal's current settings and configures it for
 * libtpxl. The original settings are restored by
 * tpxl_shutdown_terminal().
 *
 * @param terminal Terminal to initialize.
 *
 * @return TPXL_OK on success.
 * @return TPXL_IO_ERROR if the terminal cannot be configured.
 */
TpxlResult tpxl_init_terminal(TpxlTerminal* terminal);

/**
 * Gets the current terminal cursor position.
 *
 * The returned row and column are one-based.
 *
 * @param row Output pointer receiving the cursor row.
 * @param column Output pointer receiving the cursor column.
 *
 * @return TPXL_OK on success.
 * @return TPXL_IO_ERROR if the cursor position cannot be queried.
 */
TpxlResult tpxl_get_cursor_position(uint32_t* row, uint32_t* column);

/**
 * Queries the current terminal dimensions and cursor position.
 *
 * On success, the TpxlTerminal structure is populated with the terminal's
 * character-cell dimensions, pixel dimensions, cell size, and cursor
 * position.
 *
 * @param terminal Terminal to populate.
 *
 * @return TPXL_OK on success.
 * @return TPXL_IO_ERROR if the terminal dimensions or cursor position
 *         cannot be queried.
 */
TpxlResult tpxl_query_terminal(TpxlTerminal* terminal);

/**
 * Moves the terminal cursor to the specified position.
 *
 * Row and column positions are one-based.
 *
 * @param row Destination row.
 * @param column Destination column.
 *
 * @return TPXL_OK on success.
 * @return TPXL_IO_ERROR if the cursor movement cannot be written.
 */
TpxlResult tpxl_move_cursor(uint32_t row, uint32_t column);

/**
 * Ensures that the terminal has enough rows available from the current
 * cursor position.
 *
 * If fewer than required_rows are available, the terminal is scrolled
 * upward and the cursor is restored to its original position.
 *
 * On success, the terminal's cursor position is updated.
 *
 * @param terminal Terminal whose available rows are checked.
 * @param required_rows Number of rows required from the current cursor
 *                      position.
 *
 * @return TPXL_OK on success.
 * @return TPXL_IO_ERROR if the terminal cannot be scrolled or the cursor
 *         position cannot be queried.
 */
TpxlResult tpxl_terminal_ensure_rows(TpxlTerminal* terminal, uint32_t required_rows);

/**
 * Restores the terminal settings saved by tpxl_init_terminal().
 *
 * Calling this function on an uninitialized terminal has
 * no effect.
 *
 * @param terminal Terminal to shut down.
 *
 * @return TPXL_OK on success.
 * @return TPXL_IO_ERROR if the original terminal settings cannot be restored.
 */
TpxlResult tpxl_shutdown_terminal(TpxlTerminal* terminal);

#endif
