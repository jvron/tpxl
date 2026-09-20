# TXPL

TPXL is a C11 terminal media project for displaying images and playing animations, video, and audio directly in the terminal. It consists of `libtpxl`, the terminal media library, and `tpxl`, its command-line interface.

TPXL uses terminal graphics protocols such as Kitty Graphics and SIXEL, with the goal of making media practical to work with directly in the terminal.

## Terminal Support

| Terminal       | Minimum Version | Kitty Backend | SIXEL Backend |
| -------------- | --------------: | :-----------: | :-----------: |
| Kitty          |          0.39.0 |       ✓       |       x       |
| WezTerm        |      2024-02-03 |       ✓       |       ✓       |
| Ghostty        |           1.3.1 |       ✓       |       x       |
| Rio            |          0.5.27 |       ✓       |       ✓       |
| Contour        |      0.5.0.7168 |       ✓       |       ✓       |
| foot           |Testing required |       x       |Testing required|
| Konsole        |         23.08.5 |       ✓       |       ✓       |
| mlterm         |           3.9.3 |       x       |       ✓       |
| Alacritty      |               — |       x       |       x       |
| GNOME Terminal |               — |       x       |       x       |

Terminal support depends on the graphics protocols provided by the terminal. TPXL does not require a terminal to support both backends.

> Later versions of Kitty may provide improved performance with the Kitty backend.

## Building

TPXL uses CMake for its build system.

Debug build:

```bash
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug
```

Release build:

```bash
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

## Contributing

Contributions are welcome. Feel free to add features, share ideas, report bugs, or submit fixes.

## License

TPXL is licensed under the MIT License. See LICENSE for details.
