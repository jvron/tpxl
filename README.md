# TXPL

TPXL is a C11 terminal media project for displaying images and playing animations, video, and audio directly in the terminal. It consists of `libtpxl`, the terminal media library, and `tpxl`, its command-line interface.

TPXL uses terminal graphics protocols such as Kitty Graphics, SIXEL and iTerm2 Images, with the goal of making media practical to work with directly in the terminal.

**Platform support:** TPXL currently supports Linux only. Windows builds are not yet supported, and macOS support has not been tested. Cross-platform support for Windows and macOS is planned for future releases.

## Terminal Support

| Terminal       | Kitty Graphics                  | SIXEL            | iTerm2 Images    |
| -------------- | ------------------------------- | ---------------- | ---------------- |
| Kitty          | 0.39.0+                         | —                | —                |
| WezTerm        | 20220101-133340+                | 20220408-101518+ | 20220101-133340+ |
| Ghostty        | 1.3.1+                          | —                | —                |
| Rio            | 0.5.27+                         | —                | —                |
| foot           | —                               | Testing Required | —                |
| iTerm2         | —                               | —                | Testing Required |
| Contour        | 0.6.2.8008+                     | 0.6.2.8008+      | —                |
| Konsole        | 23.08.5+                        | 23.08.5+         | —                |
| mlterm         | —                               | 3.9.3+           | 3.9.3+           |
| Warp           | 0.2026.10.14.08.27.stable_01+   | —                | —                |
| Alacritty      | —                               | —                | —                |
| GNOME Terminal | —                               | —                | —                |

Terminal support depends on the graphics protocols provided by the terminal. TPXL does not require a terminal to support both backends.

> Later versions of Kitty may provide improved performance with the Kitty backend.

## Building

TPXL uses CMake for its build system.

```bash
# Debug
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug

# Release
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

**CMake options:**
- `TPXL_ASAN=ON` — Enable AddressSanitizer.
- `TPXL_TSAN=ON` — Enable ThreadSanitizer.
- `TPXL_BUILD_SHARED=ON` —  Build `libtpxl` as a shared library; disables the CLI build. (`OFF` by default)

AddressSanitizer and ThreadSanitizer cannot be enabled simultaneously.

## Planned Features

- Windows support.
- macOS support.
- Expanded media playback controls.
- Performance improvements.

## Contributing

Contributions are welcome. Feel free to add features, share ideas, report bugs, or submit fixes.

## License

TPXL is licensed under the MIT License. See [LICENSE](LICENSE) for details.
