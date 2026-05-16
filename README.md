# ASCII Video Player

A command-line tool that plays videos and displays images as colorized ASCII art in the terminal, complete with synchronized audio support.
Inspired from: https://github.com/gouwsxander/ascii-view

## Features
- **Video Playback**: High-speed frame processing for smooth video streaming.
- **Audio Synchronization**: Automatic background audio playback.
- **TrueColor Support**: Uses 24-bit ANSI color codes for high-fidelity reproduction.
- **Retro Mode**: Optional 8-color mode for a classic terminal aesthetic.
- **Edge Enhancement**: Sobel filters to keep outlines sharp at low resolutions.

---

## 🛠 Prerequisites

Before building, ensure you have the following installed and added to your system **PATH**:

### 1. FFmpeg & FFprobe (Required for Video)
Used for video decoding, scaling, and audio extraction.
- **Windows**: Download from [gyan.dev](https://www.gyan.dev/ffmpeg/builds/) or install via `winget install ffmpeg`.
- **Linux**: `sudo apt install ffmpeg` (Ubuntu/Debian) or `sudo pacman -S ffmpeg` (Arch).
- **macOS**: `brew install ffmpeg`.

### 2. Build Tools
- A C99-compatible compiler (**GCC** or **Clang**).
- **Make** build system.

---

## 🚀 Getting Started

### 1. Build the Project
For the best performance (especially for video), use the `release` target which enables optimizations:
```bash
make release
```
This generates the `ascii-view` executable.

### 2. Play Your First Video
```bash
./ascii-view path/to/your_video.mp4
```

---

## 📖 Full Usage Guide

```bash
./ascii-view <input_file> [OPTIONS]
```

### Options Explained

| Option | Name | Description | Default |
| :--- | :--- | :--- | :--- |
| `-mw` | Max Width | Maximum horizontal characters. | Terminal width |
| `-mh` | Max Height | Maximum vertical characters. | Terminal height |
| `-cr` | Char Ratio | Height-to-width ratio of terminal characters. | `2.0` |
| `-et` | Edge Threshold | Sobel filter sensitivity (0.0 - 4.0). Lower is more sensitive. | `4.0` (Off) |
| `--retro-colors` | Retro Mode | Limits output to 8 classic colors. | False |

### Advanced Usage Examples

**Optimize for Large Terminals:**
If you reduce your terminal font size, you can achieve much higher "resolution":
```bash
./ascii-view movie.mp4 -mw 200 -mh 100
```

**Fine-tune Aspect Ratio:**
If the video looks "stretched" vertically, increase the `-cr` value (e.g., `2.2`). If it looks squashed, decrease it (e.g., `1.8`).
```bash
./ascii-view video.mp4 -cr 2.3
```

**Highlight Outlines:**
Use a low edge threshold to draw character outlines over the colors:
```bash
./ascii-view animation.mp4 -et 1.5
```

---

## 💡 Tips for Best Results

1. **Smaller Fonts**: The smaller your terminal font, the more "pixels" you have. Try zooming out (Ctrl + Minus) before running.
2. **Black Background**: Best results are achieved on a dark terminal background.
3. **TrueColor Terminal**: Ensure your terminal supports 24-bit color (Windows Terminal, iTerm2, Alacritty, and most modern Linux terminals do).
4. **Squint your eyes**: Seriously! If you squint slightly, the ASCII characters blend together to form a remarkably clear image.

---

## ❓ Troubleshooting

- **No Audio (Windows)**: The player uses PowerShell's `MediaPlayer` by default. Ensure your system volume is up and PowerShell is allowed to run scripts.
- **No Audio (Linux)**: Ensure `ffplay` is installed (part of the ffmpeg package).
- **Video is Laggy**: Build with `make release` instead of just `make`. If it's still slow, reduce the width and height with `-mw` and `-mh`.
- **Colors Look Wrong**: Ensure your terminal is set to `xterm-256color` or `xterm-truecolor`.

---

## ⚙️ Technical Details
1. **Piped Decoding**: Video is decoded by `ffmpeg` and scaled to the target resolution before being piped into the C program as raw RGB24 data.
2. **HSV Conversion**: Colors are converted to the HSV space to accurately map them to the best-matching ANSI color while using the "Value" (brightness) to select the ASCII character.
3. **Double Buffering**: (Internal) Uses ANSI escape codes to reposition the cursor instead of clearing the screen, preventing flicker.
