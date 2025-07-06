#include <windows.h>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

std::ofstream fout;

// Declare DPI awareness related APIs
#include <ShellScalingAPI.h>
#pragma comment(lib, "Shcore.lib")
#pragma comment(lib, "User32.lib")

// Set process DPI awareness level
void setProcessDpiAwareness() {
    // Try to set Per Monitor v2 DPI awareness (Windows 10 1703+)
    auto setDpiAwarenessContext = [](DPI_AWARENESS_CONTEXT context) {
        using SetDpiAwarenessContextFn = DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT);
        static auto fn = reinterpret_cast<SetDpiAwarenessContextFn>(
            GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext"));
        return fn && fn(context);
    };

    // Try to set DPI awareness level (Windows 8.1+)
    auto setDpiAwareness = [](PROCESS_DPI_AWARENESS value) {
        using SetDpiAwarenessFn = HRESULT(WINAPI*)(PROCESS_DPI_AWARENESS);
        static auto fn = reinterpret_cast<SetDpiAwarenessFn>(
            GetProcAddress(GetModuleHandleW(L"shcore.dll"), "SetProcessDpiAwareness"));
        return fn && SUCCEEDED(fn(value));
    };

    // Try to set DPI awareness (Vista+)
    auto setDpiAware = []() {
        using SetDpiAwareFn = BOOL(WINAPI*)();
        static auto fn = reinterpret_cast<SetDpiAwareFn>(
            GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDPIAware"));
        return fn && fn();
    };

    // Priority: Per Monitor v2 > Per Monitor > System > Legacy
    if (!setDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        if (!setDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE)) {
            if (!setDpiAwareness(PROCESS_SYSTEM_DPI_AWARE)) {
                setDpiAware();
            }
        }
    }
}

// Get the DPI scaling factor of the monitor where the current mouse position is located
double getCurrentDpiScalingFactor() {
    // Get current mouse position
    POINT pt;
    GetCursorPos(&pt);

    // Get monitor handle
    HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

    // Try to use GetDpiForMonitor (Windows 8.1+)
    using GetDpiForMonitorFn = HRESULT(WINAPI*)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);
    static auto getDpiForMonitor = reinterpret_cast<GetDpiForMonitorFn>(
        GetProcAddress(GetModuleHandleW(L"shcore.dll"), "GetDpiForMonitor"));

    if (getDpiForMonitor) {
        UINT dpiX = 0, dpiY = 0;
        if (SUCCEEDED(getDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
            return static_cast<double>(dpiX) / 96.0;
        }
    }

    // Fallback: get primary monitor DPI
    HDC hdc = GetDC(NULL);
    const int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(NULL, hdc);

    return static_cast<double>(dpiX) / 96.0;
}

double dpiScaling = 1;

// Structure to hold command line arguments
struct CommandLineArgs {
    std::string type = "none";          // Mouse button type (none, left, right)
    std::string action = "";            // Action to perform
    int x = -1;                         // X coordinate
    int y = -1;                         // Y coordinate
    std::string mode = "none";          // Mode (none or back)
    int smooth = 0;                     // Smooth movement (0: disabled, 1: enabled)
    int smoothTime = 200;               // Smooth movement duration in milliseconds
    std::string smoothMode = "linear";  // Smoothing algorithm (linear or ease)
    bool help = false;                  // Help flag
    bool validArgs = false;             // Flag to indicate if required args are provided
};

// Function to display help information
void displayHelp() {
    fout << "Mouse Click Simulator - Simulates mouse events at specified screen coordinates\n\n";
    fout << "Usage: MouseClickSimulator [OPTIONS]\n\n";
    fout << "Options:\n";
    fout << "  -t, --type        Mouse button (none, left, right) [default: none]\n";
    fout << "  -a, --action      Action to perform (click, doubleclick, keydown, keyup)\n";
    fout << "                    [default: none for type=none, click for type=left/right]\n";
    fout << "  -x                X coordinate (-1: keep current position) [required]\n";
    fout << "  -y                Y coordinate (-1: keep current position) [required]\n";
    fout << "  -m, --mode        Mode of operation (none, back) [default: none]\n";
    fout << "  -sm, --smooth     Enable smooth movement (0: disabled, 1: enabled) [default: 0]\n";
    fout << "  -smt, --smooth_time Duration of smooth movement in milliseconds [default: 200]\n";
    fout << "  -smm, --smooth_mode Smoothing algorithm (linear, ease) [default: linear]\n";
    fout << "  -h, --help        Display this help message and exit\n\n";
    fout << "Examples:\n";
    fout << "  MouseClickSimulator -x 500 -y 500                        (just move mouse)\n";
    fout << "  MouseClickSimulator -t left -x 500 -y 500                (left click)\n";
    fout << "  MouseClickSimulator -t right -a doubleclick -x 800 -y 600 -m back -sm 1 -smm ease\n";
}

// Function to parse command line arguments
CommandLineArgs parseCommandLine(int argc, char* argv[]) {
    CommandLineArgs args;
    bool xProvided = false;
    bool yProvided = false;

    // If no arguments provided, show help
    if (argc <= 1) {
        args.help = true;
        return args;
    }

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-t" || arg == "--type") {
            if (i + 1 < argc) {
                args.type = argv[++i];
                if (args.type != "none" && args.type != "left" && args.type != "right") {
                    fout << "Error: Invalid type. Must be 'none', 'left' or 'right'.\n";
                    args.help = true;
                }
            }
        }
        else if (arg == "-a" || arg == "--action") {
            if (i + 1 < argc) {
                args.action = argv[++i];
                if (args.action != "click" && args.action != "doubleclick" &&
                    args.action != "keydown" && args.action != "keyup") {
                    fout << "Error: Invalid action. Must be 'click', 'doubleclick', 'keydown', or 'keyup'.\n";
                    args.help = true;
                }
            }
        }
        else if (arg == "-x") {
            if (i + 1 < argc) {
                try {
                    args.x = std::stoi(argv[++i]);
                    xProvided = true;
                } catch (...) {
                    fout << "Error: Invalid X coordinate.\n";
                    args.help = true;
                }
            }
        }
        else if (arg == "-y") {
            if (i + 1 < argc) {
                try {
                    args.y = std::stoi(argv[++i]);
                    yProvided = true;
                } catch (...) {
                    fout << "Error: Invalid Y coordinate.\n";
                    args.help = true;
                }
            }
        }
        else if (arg == "-m" || arg == "--mode") {
            if (i + 1 < argc) {
                args.mode = argv[++i];
                if (args.mode != "none" && args.mode != "back") {
                    fout << "Error: Invalid mode. Must be 'none' or 'back'.\n";
                    args.help = true;
                }
            }
        }
        else if (arg == "-sm" || arg == "--smooth") {
            if (i + 1 < argc) {
                try {
                    args.smooth = std::stoi(argv[++i]);
                    if (args.smooth != 0 && args.smooth != 1) {
                        fout << "Error: Invalid smooth value. Must be 0 or 1.\n";
                        args.help = true;
                    }
                } catch (...) {
                    fout << "Error: Invalid smooth value.\n";
                    args.help = true;
                }
            }
        }
        else if (arg == "-smt" || arg == "--smooth_time") {
            if (i + 1 < argc) {
                try {
                    args.smoothTime = std::stoi(argv[++i]);
                    if (args.smoothTime < 0) {
                        fout << "Error: Smooth time must be non-negative.\n";
                        args.help = true;
                    }
                } catch (...) {
                    fout << "Error: Invalid smooth time value.\n";
                    args.help = true;
                }
            }
        }
        else if (arg == "-smm" || arg == "--smooth_mode") {
            if (i + 1 < argc) {
                args.smoothMode = argv[++i];
                if (args.smoothMode != "linear" && args.smoothMode != "ease") {
                    fout << "Error: Invalid smooth mode. Must be 'linear' or 'ease'.\n";
                    args.help = true;
                }
            }
        }
        else if (arg == "-h" || arg == "--help") {
            args.help = true;
        }
        else {
            fout << "Unknown option: " << arg << "\n";
            args.help = true;
        }
    }

    // Check required parameters
    if (!args.help) {
        if (!xProvided && !yProvided) {
            fout << "Error: At least one of X or Y coordinates must be specified.\n";
            args.help = true;
        }
    }

    // Set default action based on type if not provided
    if (args.action.empty()) {
        if (args.type == "none") {
            args.action = "none";  // Just move the cursor
        }
        else {
            args.action = "click";  // Default to click for left/right
        }
    }

    // Mark arguments as valid if all required parameters are present
    args.validArgs = (xProvided || yProvided);

    return args;
}

// Function to calculate easing for smooth movement
double calculateEasing(double t, const std::string& mode) {
    if (mode == "linear") {
        return t;
    }
    else if (mode == "ease") {
        // CSS ease is approximately cubic-bezier(0.25, 0.1, 0.25, 1.0)
        // We'll implement a simpler approximation of ease using cubic easing
        if (t < 0.5) {
            return 2 * t * t;
        }
        else {
            return -1 + (4 - 2 * t) * t;
        }
    }
    // Fallback to linear if mode is not recognized
    return t;
}

// Function to move the mouse cursor smoothly
void smoothMoveMouse(int targetX, int targetY, int duration, const std::string& mode, int fps = 120) {
    fout << "Smoothly moving mouse to (" << targetX << ", " << targetY << ") over "
         << duration << " ms using " << mode << " easing.\n";

    POINT currentPos;
    GetCursorPos(&currentPos);

    int startX = currentPos.x;
    int startY = currentPos.y;

    // If -1 was specified, keep the current coordinate
    targetX = (targetX != -1) ? targetX : startX;
    targetY = (targetY != -1) ? targetY : startY;

    // Calculate the number of steps based on duration and fps
    int steps = duration * fps / 1000;
    if (steps < 2) steps = 2;  // At least 2 steps for a smooth movement

    double stepTime = static_cast<double>(duration) / steps;

    for (int i = 1; i <= steps; i++) {
        double t = static_cast<double>(i) / steps;

        // Apply easing function
        double easedT = calculateEasing(t, mode);

        int x = startX + static_cast<int>((targetX - startX) * easedT);
        int y = startY + static_cast<int>((targetY - startY) * easedT);

        SetCursorPos(x, y);
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(stepTime)));
    }

    // Ensure we end up exactly at the target position
    SetCursorPos(targetX, targetY);
}

// Function to simulate a mouse event
void simulateMouseEvent(const CommandLineArgs& args) {
    fout << "Parsed Arguments:\n";
    fout << "  Type: " << args.type << "\n";
    fout << "  Action: " << args.action << "\n";
    fout << "  X: " << args.x << "\n";
    fout << "  Y: " << args.y << "\n";
    fout << "  Mode: " << args.mode << "\n";
    fout << "  Smooth: " << args.smooth << "\n";
    fout << "  Smooth Time: " << args.smoothTime << "\n";
    fout << "  Smooth Mode: " << args.smoothMode << "\n";
    fout << "  Help: " << (args.help ? "true" : "false") << "\n";
    fout << "  Valid Args: " << (args.validArgs ? "true" : "false") << "\n\n";

    POINT originalPos;
    GetCursorPos(&originalPos);

    fout << "Original Mouse Position: (" << originalPos.x << ", " << originalPos.y << ")\n";

    int targetX = args.x;
    int targetY = args.y;

    // If -1 was specified, keep the current coordinate
    targetX = (targetX != -1) ? targetX : originalPos.x;
    targetY = (targetY != -1) ? targetY : originalPos.y;

    fout << "Target Mouse Position: (" << targetX << ", " << targetY << ")\n";

    // Move mouse to target position, either smoothly or instantly
    if (args.smooth == 1) {
        smoothMoveMouse(targetX, targetY, args.smoothTime, args.smoothMode);
    }
    else {
        fout << "Moving mouse to target position: (" << targetX << ", " << targetY << ")\n";
        SetCursorPos(targetX, targetY);
    }

    // Skip mouse button events if type is "none"
    if (args.type != "none") {
        // Determine the appropriate mouse event constants
        DWORD mouseDownFlag = 0;
        DWORD mouseUpFlag = 0;

        if (args.type == "left") {
            mouseDownFlag = MOUSEEVENTF_LEFTDOWN;
            mouseUpFlag = MOUSEEVENTF_LEFTUP;
        }
        else if (args.type == "right") {
            mouseDownFlag = MOUSEEVENTF_RIGHTDOWN;
            mouseUpFlag = MOUSEEVENTF_RIGHTUP;
        }

        // Perform the requested action
        if (args.action == "click") {
            mouse_event(mouseDownFlag, 0, 0, 0, 0);
            mouse_event(mouseUpFlag, 0, 0, 0, 0);
        }
        else if (args.action == "doubleclick") {
            mouse_event(mouseDownFlag, 0, 0, 0, 0);
            mouse_event(mouseUpFlag, 0, 0, 0, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Short delay between clicks
            mouse_event(mouseDownFlag, 0, 0, 0, 0);
            mouse_event(mouseUpFlag, 0, 0, 0, 0);
        }
        else if (args.action == "keydown") {
            mouse_event(mouseDownFlag, 0, 0, 0, 0);
        }
        else if (args.action == "keyup") {
            mouse_event(mouseUpFlag, 0, 0, 0, 0);
        }
    }

    // Return to original position if requested
    if (args.mode == "back") {
        fout << "Returning to original position: (" << originalPos.x << ", " << originalPos.y << ")\n";
        if (args.smooth == 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Small delay before moving back
            smoothMoveMouse(originalPos.x, originalPos.y, args.smoothTime, args.smoothMode);
        }
        else {
            SetCursorPos(originalPos.x, originalPos.y);
        }
    }
}

void execute(const CommandLineArgs& args) {
    // Display help if requested or if arguments are invalid
    if (args.help) {
        displayHelp();
    }

    // Only simulate mouse event if all required arguments are valid
    if (args.validArgs) {
        simulateMouseEvent(args);
    }
}

// Console application entry point for better handling of command-line arguments
int main(int argc, char* argv[]) {
    // Open log file for output in the same directory as the executable, do not append
    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir(exePath);
    size_t pos = exeDir.find_last_of("\\/");
    if (pos != std::string::npos) {
        exeDir = exeDir.substr(0, pos + 1);
    } else {
        exeDir = "";
    }
    std::string logPath = exeDir + "log.txt";
    fout.open(logPath, std::ios::out | std::ios::trunc);
    if (!fout.is_open()) {
        return 1;
    }

    // Set DPI awareness
    setProcessDpiAwareness();

    // Get DPI scaling -> no need because we set DPI awareness
    dpiScaling = getCurrentDpiScalingFactor();
    fout << "DPI Scaling Factor: " << dpiScaling << "\n";

    // Parse command line arguments
    execute(parseCommandLine(argc, argv));

    return 0;
}