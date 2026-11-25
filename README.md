# ASCII Image Generator (`ascii-gen`)

Fast C++17 CLI utility for converting images (`PNG`, `JPG`, `BMP`, `TGA`, `GIF`) into ASCII art with a **real-time interactive console preview**, 24-bit TrueColor support, Sobel edge detection, and multiple character palettes.

## Features

- **Interactive Live Console Preview** — tweak width, contrast, brightness, palettes, and filters in real time with zero flicker.
- **Area-Averaging Resampler** — smooth downscaling that accounts for terminal font aspect ratio.
- **24-bit ANSI TrueColor** — full RGB colored ASCII output in modern terminals.
- **Sobel Edge Enhancement** — optional edge filter to sharpen outlines and shapes.
- **6 Character Palettes** — `Standard`, `Detailed (70 levels)`, `Blocks (UTF-8 ░▒▓█)`, `Minimal`, `Retro / Matrix`, and `Binary`.
- **Single-file compilation** — powered by `stb_image.h` with no external runtime dependencies.

## Build

Using MinGW / GCC (`g++`):

```bash
g++ -O3 -std=c++17 main.cpp -o ascii-gen.exe -static-libgcc -static-libstdc++
```

## Usage

### 1. Interactive Preview Mode (Default)

```bash
./ascii-gen sample.png
# Or run with built-in 3D procedural demo scene:
./ascii-gen --demo
```

#### Interactive Hotkeys

| Key | Action |
| --- | --- |
| `←` / `→` or `-` / `+` | Decrease / Increase ASCII width |
| `F` | Toggle Auto-Fit to terminal window size |
| `↑` / `↓` | Increase / Decrease contrast |
| `[` / `]` | Decrease / Increase brightness |
| `P` | Cycle through character palettes |
| `C` | Toggle 24-bit ANSI TrueColor mode |
| `E` | Toggle Sobel edge enhancement filter |
| `I` | Invert brightness |
| `A` / `Z` | Adjust character aspect ratio |
| `S` | Save current ASCII art to `ascii_output.txt` (and `.ans` if colored) |
| `R` | Reset settings to default |
| `Q` / `ESC` | Exit preview and print final ASCII art |

### 2. Direct CLI / Batch Mode

```bash
# Print directly to terminal (width 80, TrueColor, edge filter)
./ascii-gen sample.png -n -w 80 -c -e

# Convert and save directly to a file
./ascii-gen sample.png -w 100 -p 1 -o art.txt
```
