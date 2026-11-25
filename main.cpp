#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iomanip>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <conio.h>
#endif

struct Pixel {
    uint8_t r, g, b, a;
};

struct Image {
    int width = 0;
    int height = 0;
    int channels = 0;
    std::vector<Pixel> data;
    std::string sourceName;

    bool isValid() const {
        return width > 0 && height > 0 && !data.empty();
    }

    Pixel getPixel(int x, int y) const {
        x = std::clamp(x, 0, width - 1);
        y = std::clamp(y, 0, height - 1);
        return data[y * width + x];
    }
};

struct Palette {
    std::string name;
    std::vector<std::string> chars; // Supports single ASCII or UTF-8 multi-byte chars
};

static const std::vector<Palette> PALETTES = {
    { "Standard", {" ", ".", ":", "-", "=", "+", "*", "#", "%", "@"} },
    { "Detailed", {" ", ".", "'", "`", "^", "\"", ",", ":", ";", "I", "l", "!", "i", ">", "<", "~", "+", "_", "-", "?", "]", "[", "}", "{", "1", ")", "(", "|", "\\", "/", "t", "f", "j", "r", "x", "n", "u", "v", "c", "z", "X", "Y", "U", "J", "C", "L", "Q", "0", "O", "Z", "m", "w", "q", "p", "d", "b", "k", "h", "a", "o", "*", "#", "M", "W", "&", "8", "%", "B", "@", "$"} },
    { "Blocks (UTF-8)", {" ", "░", "▒", "▓", "█"} },
    { "Minimal", {" ", ".", "o", "O", "@"} },
    { "Retro / Matrix", {" ", "·", ":", "+", "x", "X", "$", "#"} },
    { "Binary", {" ", "0", "1"} }
};

struct RenderSettings {
    int targetWidth = 90;
    bool fitToTerminal = true;
    int paletteIndex = 0;
    bool colorMode = false;      // ANSI 24-bit TrueColor
    bool invert = false;         // Invert brightness
    bool edgeEnhance = false;    // Sobel edge filter mix
    float brightness = 0.0f;     // -1.0 .. +1.0
    float contrast = 1.0f;       // 0.2 .. 3.0
    float charAspect = 0.5f;     // Height-to-width ratio of terminal font (~0.5)
    std::string outputPath = "";
};

// Setup Windows console for UTF-8 and ANSI escape codes
void setupConsole() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif
}

void getTerminalSize(int& outCols, int& outRows) {
    outCols = 100;
    outRows = 35;
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        outCols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        outRows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
#endif
}

// Load image from disk via stb_image
bool loadImage(const std::string& path, Image& outImg) {
    int w, h, c;
    unsigned char* raw = stbi_load(path.c_str(), &w, &h, &c, 4);
    if (!raw) {
        return false;
    }
    outImg.width = w;
    outImg.height = h;
    outImg.channels = c;
    outImg.sourceName = path;
    outImg.data.resize(w * h);

    for (int i = 0; i < w * h; ++i) {
        uint8_t r = raw[i * 4 + 0];
        uint8_t g = raw[i * 4 + 1];
        uint8_t b = raw[i * 4 + 2];
        uint8_t a = raw[i * 4 + 3];
        // Alpha blend over black background
        float alpha = a / 255.0f;
        outImg.data[i] = {
            static_cast<uint8_t>(r * alpha),
            static_cast<uint8_t>(g * alpha),
            static_cast<uint8_t>(b * alpha),
            a
        };
    }
    stbi_image_free(raw);
    return true;
}

// Generate a procedural demo image (3D lit sphere + shapes + gradient) if no image provided
Image generateDemoImage(int w = 320, int h = 240) {
    Image img;
    img.width = w;
    img.height = h;
    img.channels = 4;
    img.sourceName = "Built-in Demo Scene";
    img.data.resize(w * h);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float nx = (x - w * 0.5f) / (h * 0.42f);
            float ny = (y - h * 0.5f) / (h * 0.42f);
            float r2 = nx * nx + ny * ny;

            uint8_t r = 0, g = 0, b = 0;

            if (r2 <= 1.0f) {
                // 3D sphere normal
                float nz = std::sqrt(1.0f - r2);
                // Light direction
                float lx = -0.55f, ly = -0.55f, lz = 0.63f;
                float diff = std::max(0.0f, nx * lx + ny * ly + nz * lz);
                float spec = std::pow(std::max(0.0f, nz * lz + nx * lx + ny * ly), 24.0f);
                float ambient = 0.12f;

                float baseR = 0.25f + 0.75f * (0.5f + 0.5f * nx);
                float baseG = 0.35f + 0.65f * (0.5f - 0.5f * ny);
                float baseB = 0.85f;

                float valR = std::clamp(ambient + diff * baseR + spec * 0.8f, 0.0f, 1.0f);
                float valG = std::clamp(ambient + diff * baseG + spec * 0.8f, 0.0f, 1.0f);
                float valB = std::clamp(ambient + diff * baseB + spec * 0.9f, 0.0f, 1.0f);

                r = static_cast<uint8_t>(valR * 255);
                g = static_cast<uint8_t>(valG * 255);
                b = static_cast<uint8_t>(valB * 255);
            } else {
                // Starry / ring background pattern
                float dist = std::sqrt(r2);
                float ring = std::sin(dist * 18.0f) * 0.5f + 0.5f;
                float vig = std::clamp(1.5f - dist * 0.6f, 0.0f, 1.0f);
                float bg = ring * 0.22f * vig;
                r = static_cast<uint8_t>(bg * 180);
                g = static_cast<uint8_t>(bg * 140);
                b = static_cast<uint8_t>(bg * 255);
            }

            img.data[y * w + x] = { r, g, b, 255 };
        }
    }
    return img;
}

float pixelLuma(const Pixel& p) {
    return (0.299f * p.r + 0.587f * p.g + 0.114f * p.b) / 255.0f;
}

// Compute Sobel edge magnitude at (x, y)
float computeEdge(const Image& img, int x, int y) {
    auto L = [&](int px, int py) {
        return pixelLuma(img.getPixel(px, py));
    };
    float gx = -L(x - 1, y - 1) + L(x + 1, y - 1)
               - 2.0f * L(x - 1, y) + 2.0f * L(x + 1, y)
               - L(x - 1, y + 1) + L(x + 1, y + 1);

    float gy = -L(x - 1, y - 1) - 2.0f * L(x, y - 1) - L(x + 1, y - 1)
               + L(x - 1, y + 1) + 2.0f * L(x, y + 1) + L(x + 1, y + 1);

    return std::clamp(std::sqrt(gx * gx + gy * gy), 0.0f, 1.0f);
}

struct RenderedAscii {
    int cols = 0;
    int rows = 0;
    std::string text;      // With ANSI colors if enabled
    std::string plainText; // Pure text without ANSI codes (for .txt saving)
};

RenderedAscii renderAscii(const Image& img, const RenderSettings& settings, int maxRowsLimit = 0) {
    RenderedAscii result;
    if (!img.isValid()) return result;

    int outCols = std::max(10, settings.targetWidth);
    float imgAspect = static_cast<float>(img.height) / static_cast<float>(img.width);
    int outRows = std::max(5, static_cast<int>(std::round(outCols * imgAspect * settings.charAspect)));

    // If auto-fitting to terminal and height exceeds available terminal rows, scale down proportionally
    if (maxRowsLimit > 4 && outRows > maxRowsLimit) {
        float scale = static_cast<float>(maxRowsLimit) / static_cast<float>(outRows);
        outRows = maxRowsLimit;
        outCols = std::max(10, static_cast<int>(std::round(outCols * scale)));
    }

    result.cols = outCols;
    result.rows = outRows;

    const auto& palette = PALETTES[settings.paletteIndex % PALETTES.size()].chars;
    int numChars = static_cast<int>(palette.size());

    std::ostringstream ossColor;
    std::ostringstream ossPlain;

    float scaleX = static_cast<float>(img.width) / outCols;
    float scaleY = static_cast<float>(img.height) / outRows;

    for (int row = 0; row < outRows; ++row) {
        int y0 = static_cast<int>(row * scaleY);
        int y1 = std::max(y0 + 1, static_cast<int>((row + 1) * scaleY));
        y1 = std::min(y1, img.height);

        for (int col = 0; col < outCols; ++col) {
            int x0 = static_cast<int>(col * scaleX);
            int x1 = std::max(x0 + 1, static_cast<int>((col + 1) * scaleX));
            x1 = std::min(x1, img.width);

            // Area averaging over the cell
            float sumR = 0.0f, sumG = 0.0f, sumB = 0.0f;
            int count = 0;

            // Step size so super high-res images remain instant
            int stepY = std::max(1, (y1 - y0) / 4);
            int stepX = std::max(1, (x1 - x0) / 4);

            for (int y = y0; y < y1; y += stepY) {
                for (int x = x0; x < x1; x += stepX) {
                    Pixel p = img.getPixel(x, y);
                    sumR += p.r;
                    sumG += p.g;
                    sumB += p.b;
                    count++;
                }
            }

            if (count == 0) count = 1;
            uint8_t avgR = static_cast<uint8_t>(sumR / count);
            uint8_t avgG = static_cast<uint8_t>(sumG / count);
            uint8_t avgB = static_cast<uint8_t>(sumB / count);

            float luma = (0.299f * avgR + 0.587f * avgG + 0.114f * avgB) / 255.0f;

            if (settings.edgeEnhance) {
                int cx = (x0 + x1) / 2;
                int cy = (y0 + y1) / 2;
                float edge = computeEdge(img, cx, cy);
                luma = std::clamp(luma * 0.7f + edge * 0.65f, 0.0f, 1.0f);
            }

            // Apply brightness and contrast
            luma += settings.brightness;
            luma = (luma - 0.5f) * settings.contrast + 0.5f;
            luma = std::clamp(luma, 0.0f, 1.0f);

            if (settings.invert) {
                luma = 1.0f - luma;
            }

            int charIdx = static_cast<int>(std::round(luma * (numChars - 1)));
            charIdx = std::clamp(charIdx, 0, numChars - 1);
            const std::string& ch = palette[charIdx];

            ossPlain << ch;

            if (settings.colorMode) {
                // Boost color slightly based on contrast/brightness for vivid terminal display
                auto adjustChan = [&](uint8_t c) -> int {
                    float v = c / 255.0f;
                    v += settings.brightness;
                    v = (v - 0.5f) * settings.contrast + 0.5f;
                    return std::clamp(static_cast<int>(v * 255.0f), 0, 255);
                };
                int cr = adjustChan(avgR);
                int cg = adjustChan(avgG);
                int cb = adjustChan(avgB);
                ossColor << "\x1b[38;2;" << cr << ";" << cg << ";" << cb << "m" << ch;
            } else {
                ossColor << ch;
            }
        }

        if (settings.colorMode) {
            ossColor << "\x1b[0m";
        }
        if (row < outRows - 1) {
            ossColor << "\n";
            ossPlain << "\n";
        }
    }

    result.text = ossColor.str();
    result.plainText = ossPlain.str();
    return result;
}

bool saveToFile(const std::string& path, const std::string& content) {
    std::ofstream ofs(path, std::ios::out | std::ios::binary);
    if (!ofs) return false;
    ofs << content << "\n";
    return ofs.good();
}

void printHelp(const char* progName) {
    std::cout << "=========================================================\n"
              << "  ASCII Image Generator & Interactive Console Previewer\n"
              << "=========================================================\n\n"
              << "Usage:\n"
              << "  " << progName << " <image_path> [options]\n"
              << "  " << progName << " --demo [options]\n\n"
              << "Options:\n"
              << "  -i, --interactive      Launch interactive real-time preview (default if no -o)\n"
              << "  -n, --no-interactive   Print directly to stdout and exit\n"
              << "  -w, --width <cols>     Output width in characters (default: 90)\n"
              << "  -p, --palette <0-5>    Character palette (0:Standard, 1:Detailed, 2:Blocks, 3:Minimal, 4:Retro, 5:Binary)\n"
              << "  -c, --color            Enable 24-bit ANSI TrueColor output\n"
              << "  -I, --invert           Invert brightness\n"
              << "  -e, --edges            Enable Sobel edge enhancement\n"
              << "  -b, --brightness <val> Brightness offset (-1.0 to 1.0, default: 0.0)\n"
              << "  -C, --contrast <val>   Contrast multiplier (0.2 to 3.0, default: 1.0)\n"
              << "  -a, --aspect <val>     Font aspect ratio (default: 0.5)\n"
              << "  -o, --output <file>    Save ASCII art to file\n"
              << "  --demo                 Use built-in 3D procedural sphere image\n"
              << "  -h, --help             Show this help message\n";
}

// Run interactive live console preview
void runInteractivePreview(const Image& img, RenderSettings& settings) {
    // Enter alternate screen buffer and hide cursor
    std::cout << "\x1b[?1049h\x1b[?25l" << std::flush;

    std::string statusMsg = "Interactive Preview Ready. Use arrow keys or hotkeys below.";
    bool running = true;
    bool keepOnExit = true;
    RenderedAscii lastRendered;

    while (running) {
        int termCols = 100, termRows = 35;
        getTerminalSize(termCols, termRows);

        int availRows = std::max(6, termRows - 4); // Reserve 4 rows at bottom for HUD

        if (settings.fitToTerminal) {
            settings.targetWidth = std::max(10, termCols - 2);
        }

        lastRendered = renderAscii(img, settings, settings.fitToTerminal ? availRows : 0);

        std::ostringstream frame;
        // Move cursor home
        frame << "\x1b[H";

        // Split lines and pad/clear to end of line to prevent ghosting without flicker
        std::istringstream iss(lastRendered.text);
        std::string line;
        int linesPrinted = 0;
        while (std::getline(iss, line) && linesPrinted < availRows) {
            frame << line << "\x1b[0m\x1b[K\n";
            linesPrinted++;
        }
        while (linesPrinted < availRows) {
            frame << "\x1b[K\n";
            linesPrinted++;
        }

        // HUD Status bar
        const std::string& palName = PALETTES[settings.paletteIndex % PALETTES.size()].name;
        frame << "\x1b[48;2;30;35;45m\x1b[38;2;220;230;255m"
              << " [File: " << img.sourceName << " (" << img.width << "x" << img.height << ")]"
              << "  Size: " << lastRendered.cols << "x" << lastRendered.rows
              << (settings.fitToTerminal ? " (AutoFit)" : " (Manual)")
              << " | Palette: " << palName
              << " | Contrast: " << std::fixed << std::setprecision(2) << settings.contrast
              << " | Bright: " << std::showpos << settings.brightness << std::noshowpos
              << " | Color: " << (settings.colorMode ? "ON" : "OFF")
              << " | Invert: " << (settings.invert ? "ON" : "OFF")
              << " | Edges: " << (settings.edgeEnhance ? "ON" : "OFF")
              << "\x1b[K\x1b[0m\n";

        frame << "\x1b[48;2;20;22;30m\x1b[38;2;160;210;170m"
              << " Controls: [Left/Right] Width  [F] AutoFit  [Up/Down] Contrast  [ [/] ] Brightness  [P] Palette  [C] Color  [I] Invert  [E] Edges  [A/Z] Aspect  [S] Save .txt  [R] Reset  [Q] Quit"
              << "\x1b[K\x1b[0m\n";

        frame << "\x1b[38;2;255;215;100m > " << statusMsg << "\x1b[K\x1b[0m";

        std::cout << frame.str() << std::flush;

#ifdef _WIN32
        int ch = _getch();
        if (ch == 0 || ch == 224) {
            // Arrow keys
            int ext = _getch();
            if (ext == 75) { // Left
                settings.fitToTerminal = false;
                settings.targetWidth = std::max(12, lastRendered.cols - 4);
                statusMsg = "Width decreased to " + std::to_string(settings.targetWidth);
            } else if (ext == 77) { // Right
                settings.fitToTerminal = false;
                settings.targetWidth = std::min(400, lastRendered.cols + 4);
                statusMsg = "Width increased to " + std::to_string(settings.targetWidth);
            } else if (ext == 72) { // Up
                settings.contrast = std::min(3.5f, settings.contrast + 0.1f);
                statusMsg = "Contrast increased";
            } else if (ext == 80) { // Down
                settings.contrast = std::max(0.2f, settings.contrast - 0.1f);
                statusMsg = "Contrast decreased";
            }
        } else {
            if (ch == 'q' || ch == 'Q' || ch == 27) {
                running = false;
            } else if (ch == '-' || ch == '_') {
                settings.fitToTerminal = false;
                settings.targetWidth = std::max(12, lastRendered.cols - 4);
                statusMsg = "Width: " + std::to_string(settings.targetWidth);
            } else if (ch == '+' || ch == '=') {
                settings.fitToTerminal = false;
                settings.targetWidth = std::min(400, lastRendered.cols + 4);
                statusMsg = "Width: " + std::to_string(settings.targetWidth);
            } else if (ch == 'f' || ch == 'F') {
                settings.fitToTerminal = !settings.fitToTerminal;
                if (!settings.fitToTerminal) settings.targetWidth = lastRendered.cols;
                statusMsg = settings.fitToTerminal ? "Auto-Fit to terminal ON" : "Manual width mode";
            } else if (ch == 'p' || ch == 'P') {
                settings.paletteIndex = (settings.paletteIndex + 1) % static_cast<int>(PALETTES.size());
                statusMsg = "Palette switched to: " + PALETTES[settings.paletteIndex].name;
            } else if (ch == 'c' || ch == 'C') {
                settings.colorMode = !settings.colorMode;
                statusMsg = std::string("24-bit ANSI TrueColor: ") + (settings.colorMode ? "ENABLED" : "DISABLED");
            } else if (ch == 'i' || ch == 'I') {
                settings.invert = !settings.invert;
                statusMsg = std::string("Invert brightness: ") + (settings.invert ? "ON" : "OFF");
            } else if (ch == 'e' || ch == 'E') {
                settings.edgeEnhance = !settings.edgeEnhance;
                statusMsg = std::string("Sobel Edge Filter: ") + (settings.edgeEnhance ? "ON" : "OFF");
            } else if (ch == '[') {
                settings.brightness = std::max(-0.9f, settings.brightness - 0.05f);
                statusMsg = "Brightness decreased";
            } else if (ch == ']') {
                settings.brightness = std::min(0.9f, settings.brightness + 0.05f);
                statusMsg = "Brightness increased";
            } else if (ch == 'a' || ch == 'A') {
                settings.charAspect = std::min(1.0f, settings.charAspect + 0.05f);
                statusMsg = "Char aspect ratio: " + std::to_string(settings.charAspect);
            } else if (ch == 'z' || ch == 'Z') {
                settings.charAspect = std::max(0.25f, settings.charAspect - 0.05f);
                statusMsg = "Char aspect ratio: " + std::to_string(settings.charAspect);
            } else if (ch == 'r' || ch == 'R') {
                settings.brightness = 0.0f;
                settings.contrast = 1.0f;
                settings.invert = false;
                settings.edgeEnhance = false;
                settings.charAspect = 0.5f;
                settings.fitToTerminal = true;
                statusMsg = "Settings reset to defaults";
            } else if (ch == 's' || ch == 'S') {
                std::string outTxt = settings.outputPath.empty() ? "ascii_output.txt" : settings.outputPath;
                if (saveToFile(outTxt, lastRendered.plainText)) {
                    statusMsg = "Saved plain ASCII to '" + outTxt + "'";
                    if (settings.colorMode) {
                        std::string outAns = outTxt + ".ans";
                        saveToFile(outAns, lastRendered.text);
                        statusMsg += " (and ANSI color to '" + outAns + "')";
                    }
                } else {
                    statusMsg = "ERROR: Failed to save to '" + outTxt + "'";
                }
            }
        }
#else
        running = false;
#endif
    }

    // Restore normal screen buffer and show cursor
    std::cout << "\x1b[?25h\x1b[?1049l" << std::flush;

    if (keepOnExit) {
        std::cout << lastRendered.text << "\x1b[0m\n";
    }
}

int main(int argc, char* argv[]) {
    setupConsole();

    RenderSettings settings;
    std::string imagePath = "";
    bool useDemo = false;
    bool interactiveSpecified = false;
    bool interactiveMode = true;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp(argv[0]);
            return 0;
        } else if (arg == "--demo") {
            useDemo = true;
        } else if (arg == "-i" || arg == "--interactive") {
            interactiveMode = true;
            interactiveSpecified = true;
        } else if (arg == "-n" || arg == "--no-interactive") {
            interactiveMode = false;
            interactiveSpecified = true;
            settings.fitToTerminal = false;
        } else if ((arg == "-w" || arg == "--width") && i + 1 < argc) {
            settings.targetWidth = std::max(10, std::atoi(argv[++i]));
            settings.fitToTerminal = false;
        } else if ((arg == "-p" || arg == "--palette") && i + 1 < argc) {
            settings.paletteIndex = std::clamp(std::atoi(argv[++i]), 0, static_cast<int>(PALETTES.size()) - 1);
        } else if (arg == "-c" || arg == "--color") {
            settings.colorMode = true;
        } else if (arg == "-I" || arg == "--invert") {
            settings.invert = true;
        } else if (arg == "-e" || arg == "--edges") {
            settings.edgeEnhance = true;
        } else if ((arg == "-b" || arg == "--brightness") && i + 1 < argc) {
            settings.brightness = std::clamp(static_cast<float>(std::atof(argv[++i])), -1.0f, 1.0f);
        } else if ((arg == "-C" || arg == "--contrast") && i + 1 < argc) {
            settings.contrast = std::clamp(static_cast<float>(std::atof(argv[++i])), 0.1f, 4.0f);
        } else if ((arg == "-a" || arg == "--aspect") && i + 1 < argc) {
            settings.charAspect = std::clamp(static_cast<float>(std::atof(argv[++i])), 0.2f, 1.5f);
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            settings.outputPath = argv[++i];
            if (!interactiveSpecified) {
                interactiveMode = false;
                settings.fitToTerminal = false;
            }
        } else if (!arg.empty() && arg[0] != '-') {
            imagePath = arg;
        }
    }

    Image img;
    if (!imagePath.empty()) {
        if (!loadImage(imagePath, img)) {
            std::cerr << "Error: Failed to load image '" << imagePath << "' (" << stbi_failure_reason() << ")\n";
            return 1;
        }
    } else if (useDemo || argc == 1) {
        img = generateDemoImage();
    } else {
        printHelp(argv[0]);
        return 1;
    }

    if (interactiveMode) {
        runInteractivePreview(img, settings);
    } else {
        RenderedAscii rendered = renderAscii(img, settings, 0);
        std::cout << rendered.text << "\x1b[0m\n";
        if (!settings.outputPath.empty()) {
            if (saveToFile(settings.outputPath, rendered.plainText)) {
                std::cerr << "\n[Saved ASCII art (" << rendered.cols << "x" << rendered.rows << ") to " << settings.outputPath << "]\n";
            } else {
                std::cerr << "\n[Error: Failed to write to " << settings.outputPath << "]\n";
                return 1;
            }
        }
    }

    return 0;
}
