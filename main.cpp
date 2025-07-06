#include <windows.h>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

// Declare DPI awareness related APIs
#include <ShellScalingAPI.h>
#pragma comment(lib, "Shcore.lib")
#pragma comment(lib, "User32.lib")

bool quiet = false;
bool verbose = false;
double dpiScaling = 1;
bool consistent = false;  // Flag for consistent coordinates

// Map to store keyboard virtual key codes
std::map<std::string, WORD> keyCodeMap = {
    // Function keys
    {"key_f1", VK_F1},
    {"key_f2", VK_F2},
    {"key_f3", VK_F3},
    {"key_f4", VK_F4},
    {"key_f5", VK_F5},
    {"key_f6", VK_F6},
    {"key_f7", VK_F7},
    {"key_f8", VK_F8},
    {"key_f9", VK_F9},
    {"key_f10", VK_F10},
    {"key_f11", VK_F11},
    {"key_f12", VK_F12},

    // Control keys
    {"key_ctrl", VK_CONTROL},
    {"key_shift", VK_SHIFT},
    {"key_alt", VK_MENU},
    {"key_win", VK_LWIN},
    {"key_escape", VK_ESCAPE},
    {"key_enter", VK_RETURN},
    {"key_space", VK_SPACE},
    {"key_tab", VK_TAB},
    {"key_backspace", VK_BACK},
    {"key_delete", VK_DELETE},
    {"key_insert", VK_INSERT},

    // Navigation keys
    {"key_home", VK_HOME},
    {"key_end", VK_END},
    {"key_pgup", VK_PRIOR},
    {"key_pgdn", VK_NEXT},
    {"key_left", VK_LEFT},
    {"key_right", VK_RIGHT},
    {"key_up", VK_UP},
    {"key_down", VK_DOWN},

    // Alphanumeric keys
    {"key_a", 'A'},
    {"key_b", 'B'},
    {"key_c", 'C'},
    {"key_d", 'D'},
    {"key_e", 'E'},
    {"key_f", 'F'},
    {"key_g", 'G'},
    {"key_h", 'H'},
    {"key_i", 'I'},
    {"key_j", 'J'},
    {"key_k", 'K'},
    {"key_l", 'L'},
    {"key_m", 'M'},
    {"key_n", 'N'},
    {"key_o", 'O'},
    {"key_p", 'P'},
    {"key_q", 'Q'},
    {"key_r", 'R'},
    {"key_s", 'S'},
    {"key_t", 'T'},
    {"key_u", 'U'},
    {"key_v", 'V'},
    {"key_w", 'W'},
    {"key_x", 'X'},
    {"key_y", 'Y'},
    {"key_z", 'Z'},
    {"key_0", '0'},
    {"key_1", '1'},
    {"key_2", '2'},
    {"key_3", '3'},
    {"key_4", '4'},
    {"key_5", '5'},
    {"key_6", '6'},
    {"key_7", '7'},
    {"key_8", '8'},
    {"key_9", '9'},

    // Special characters
    {"key_minus", VK_OEM_MINUS},    // '-' key
    {"key_plus", VK_OEM_PLUS},      // '+' key
    {"key_comma", VK_OEM_COMMA},    // ',' key
    {"key_period", VK_OEM_PERIOD},  // '.' key
    {"key_semicolon", VK_OEM_1},    // ';' key
    {"key_slash", VK_OEM_2},        // '/' key
    {"key_tilde", VK_OEM_3},        // '`' key
    {"key_lbracket", VK_OEM_4},     // '[' key
    {"key_backslash", VK_OEM_5},    // '\' key
    {"key_rbracket", VK_OEM_6},     // ']' key
    {"key_quote", VK_OEM_7},        // '\'' key
};

// Structure to hold command line arguments
struct CommandLineArgs {
    std::string key = "none";     // Input device and key (none, mouse_left, key_a, etc.)
    std::string action = "none";  // Action to perform (click, doubleclick, keydown, keyup)
    int x = -1;                   // X coordinate for mouse
    int y = -1;                   // Y coordinate for mouse
    std::string mode = "none";    // Mode (none or back)
    std::string smooth = "none";  // Smooth movement (none, linear, ease)
    int smoothTime = 200;         // Smooth movement duration in milliseconds
    int sleep = 0;                // Sleep time in milliseconds
    std::string file = "";        // Input file path
    bool verbose = false;         // Verbose output flag
    bool quiet = false;           // Quiet mode flag
    bool help = false;            // Help flag
    bool validArgs = true;        // Flag to indicate if required args are provided
};

// Window procedure for the temporary window
LRESULT CALLBACK TempWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

/**
 * @brief Function that temporarily steals focus and then returns it to the original window
 * @param sleepTimeMs The time in milliseconds to wait before returning focus
 * @return TRUE if successful, FALSE if any critical step failed
 */
BOOL SwitchFocus(DWORD sleepTimeMs) {
    // Get the current foreground window (the window that has focus)
    HWND originalForegroundWindow = GetForegroundWindow();

    if (originalForegroundWindow == NULL) {
        std::cerr << "Failed to get the current foreground window." << std::endl;
        return FALSE;
    }

    // Store window information for later use
    DWORD originalThreadId = GetWindowThreadProcessId(originalForegroundWindow, NULL);
    DWORD currentThreadId = GetCurrentThreadId();

    // Register the window class for our temporary window
    const char CLASS_NAME[] = "TempFocusWindow";

    WNDCLASSA wc = {};
    wc.lpfnWndProc = TempWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClassA(&wc)) {
        std::cerr << "Failed to register window class. Error: " << GetLastError() << std::endl;
        return FALSE;
    }

    // Create the temporary window (1x1 pixel, no border)
    HWND tempWindow = CreateWindowExA(
        WS_EX_TOOLWINDOW,          // Prevents showing in taskbar
        CLASS_NAME,                // Window class name
        "Temporary Focus Window",  // Window title
        WS_POPUP,                  // No border, title bar, etc.
        0, 0,                      // Position (0,0)
        0, 0,                      // Size (0x0)
        NULL,                      // No parent window
        NULL,                      // No menu
        GetModuleHandle(NULL),     // Instance handle
        NULL                       // Additional application data
    );

    if (tempWindow == NULL) {
        std::cerr << "Failed to create temporary window. Error: " << GetLastError() << std::endl;
        return FALSE;
    }

    // Show and activate the window (steal focus)
    ShowWindow(tempWindow, SW_SHOW);
    SetForegroundWindow(tempWindow);

    // Wait for the specified time
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));

    // Return focus to the original window
    // Attach thread input to help with focus transfer
    if (AttachThreadInput(currentThreadId, originalThreadId, TRUE)) {
        SetForegroundWindow(originalForegroundWindow);
        AttachThreadInput(currentThreadId, originalThreadId, FALSE);
    }
    else {
        // Fallback method if AttachThreadInput fails
        SetForegroundWindow(originalForegroundWindow);
        std::cerr << "AttachThreadInput failed. Error: " << GetLastError() << std::endl;
    }

    // Destroy the temporary window
    DestroyWindow(tempWindow);

    // Process any remaining messages
    MSG msg = {};
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return TRUE;
}

POINT lastPos = {0, 0};

BOOL GetConsistentCursorPos(LPPOINT pt, BOOL reset = FALSE) {
    BOOL result = TRUE;

    // Reset last position to current
    if (reset) result = GetCursorPos(&lastPos);

    if (!consistent) {
        // If consistent mode is disabled, just get the current cursor position
        if (reset)
            *pt = lastPos;
        else
            result = result && GetCursorPos(pt);
    }
    else {
        // Consistent mode: always return the last known position unless reset
        *pt = lastPos;
    }
    return result;
}

BOOL SetConsistentCursorPos(int x, int y) {
    // If consistent mode is enabled, update the last position
    if (consistent) {
        lastPos.x = x;
        lastPos.y = y;
    }
    // Set the cursor position
    return SetCursorPos(x, y);
}



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

// Function to display help information
void displayHelp() {
    std::cout << "Mouse and Keyboard Simulator - Simulates mouse and keyboard events at specified screen coordinates\n\n";
    std::cout << "Usage: MouseClickSimulator [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "    -k, --key           Input type (none, mouse_left, mouse_right, mouse_middle,\n";
    std::cout << "                        mouse_move, wheel_up, wheel_down, key_a, key_b, key_enter,\n";
    std::cout << "                        switch_focus, etc.) [default: none]\n";
    std::cout << "    -a, --action        Action to perform (click, doubleclick, keydown, keyup)\n";
    std::cout << "                        [default: none for key=none, click for mouse_* types]\n";
    std::cout << "    -x                  X coordinate (-1: keep current position)\n";
    std::cout << "    -y                  Y coordinate (-1: keep current position)\n";
    std::cout << "    -m, --mode          Mode of operation (none, back) [default: none]\n";
    std::cout << "    -sm, --smooth       Smooth movement mode (none, linear, ease) [default: none]\n";
    std::cout << "    -smt, --smooth_time Duration of smooth movement in milliseconds. If key is switch_focus,\n";
    std::cout << "                        this is the time to hold focus [default: 200]\n";
    std::cout << "    -s, --sleep         Sleep time in milliseconds after action [default: 0]\n";
    std::cout << "    -f, --file          Path to a text file containing commands (one per line)\n";
    std::cout << "    -c, --consistent    Use consistent coordinates, ignoring external mouse movement [default: true]\n";
    std::cout << "    -q, --quiet         Suppress all output except errors\n";
    std::cout << "    -v, --verbose       Enable verbose output\n";
    std::cout << "    -h, --help          Display this help message and exit\n\n";
    std::cout << "Examples:\n";
    std::cout << "    MouseClickSimulator -k mouse_move -x 500 -y 500                (just move mouse)\n";
    std::cout << "    MouseClickSimulator -k mouse_left -x 500 -y 500                (left click)\n";
    std::cout << "    MouseClickSimulator -k key_enter -a click                      (press Enter key)\n";
    std::cout << "    MouseClickSimulator -k none -s 1000                           (just sleep for 1 second)\n";
    std::cout << "    MouseClickSimulator -k mouse_right -a doubleclick -x 800 -y 600 -m back -sm ease -smt 300\n";
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

        if (arg == "-k" || arg == "--key") {
            if (i + 1 < argc) {
                args.key = argv[++i];
                // Validate key type
                bool validKey =
                    (args.key == "none") ||
                    (args.key == "mouse_left") ||
                    (args.key == "mouse_right") ||
                    (args.key == "mouse_middle") ||
                    (args.key == "mouse_move") ||
                    (args.key == "wheel_up") ||
                    (args.key == "wheel_down") ||
                    (args.key == "switch_focus") ||
                    (args.key.substr(0, 4) == "key_" && keyCodeMap.find(args.key) != keyCodeMap.end());

                if (!validKey) {
                    if (!quiet) std::cout << "Error: Invalid key type '" << args.key << "'.";
                    args.help = true;
                }
            }
        }
        else if (arg == "-a" || arg == "--action") {
            if (i + 1 < argc) {
                args.action = argv[++i];
                if (args.action != "click" && args.action != "doubleclick" &&
                    args.action != "keydown" && args.action != "keyup" && args.action != "none") {
                    if (!quiet) std::cout << "Error: Invalid action. Must be 'none', 'click', 'doubleclick', 'keydown', or 'keyup'.\n";
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
                    if (!quiet) std::cout << "Error: Invalid X coordinate.\n";
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
                    if (!quiet) std::cout << "Error: Invalid Y coordinate.\n";
                    args.help = true;
                }
            }
        }
        else if (arg == "-m" || arg == "--mode") {
            if (i + 1 < argc) {
                args.mode = argv[++i];
                if (args.mode != "none" && args.mode != "back") {
                    if (!quiet) std::cout << "Error: Invalid mode. Must be 'none' or 'back'.\n";
                    args.help = true;
                }
            }
        }
        else if (arg == "-sm" || arg == "--smooth") {
            if (i + 1 < argc) {
                args.smooth = argv[++i];
                if (args.smooth != "none" && args.smooth != "linear" && args.smooth != "ease") {
                    if (!quiet) std::cout << "Error: Invalid smooth mode. Must be 'none', 'linear' or 'ease'.\n";
                    args.help = true;
                }
            }
        }
        else if (arg == "-smt" || arg == "--smooth_time") {
            if (i + 1 < argc) {
                try {
                    args.smoothTime = std::stoi(argv[++i]);
                    if (args.smoothTime < 0) {
                        if (!quiet) std::cout << "Error: Smooth time must be non-negative.\n";
                        args.help = true;
                    }
                } catch (...) {
                    if (!quiet) std::cout << "Error: Invalid smooth time value.\n";
                    args.help = true;
                }
            }
        }
        else if (arg == "-s" || arg == "--sleep") {
            if (i + 1 < argc) {
                try {
                    args.sleep = std::stoi(argv[++i]);
                    if (args.sleep < 0) {
                        if (!quiet) std::cout << "Error: Sleep time must be non-negative.\n";
                        args.help = true;
                    }
                } catch (...) {
                    if (!quiet) std::cout << "Error: Invalid sleep time value.\n";
                    args.help = true;
                }
            }
        }
        else if (arg == "-f" || arg == "--file") {
            if (i + 1 < argc) {
                args.file = argv[++i];
            }
        }
        else if (arg == "-c" || arg == "--consistent") {
            // Consistent coordinates flag
            consistent = true;  // Always true in this implementation
        }
        else if (arg == "-v" || arg == "--verbose") {
            args.verbose = true;
            verbose = true;
        }
        else if (arg == "-h" || arg == "--help") {
            args.help = true;
        }
        else if (arg == "-q" || arg == "--quiet") {
            args.quiet = true;
            quiet = true;
        }
        else {
            if (!quiet) std::cout << "Warning: Unknown option: " << arg << "\n";
            args.help = true;
        }
    }

    // // Validate required parameters for mouse operations
    // if (!args.help) {
    //     // For mouse operations, we need coordinates
    //     if (args.key.substr(0, 6) == "mouse_" || args.key.substr(0, 6) == "wheel_") {
    //         if (!xProvided && !yProvided) {
    //             if (!quiet) std::cout << "Error: At least one of X or Y coordinates must be specified for mouse operations.\n";
    //             args.help = true;
    //         }
    //     }
    // }

    // Set default action based on key type if not provided
    if (args.action == "none" && args.key != "none") {
        if (args.key.substr(0, 6) == "mouse_" || args.key.substr(0, 6) == "wheel_" || args.key.substr(0, 4) == "key_") {
            args.action = "click";  // Default to click
        }
    }

    // Mark arguments as valid
    args.validArgs = !args.help && (args.sleep > 0 || args.key != "none" || !args.file.empty());

    // If quiet mode is enabled, verbose output is suppressed
    if (args.quiet) args.verbose = false;

    return args;
}

// Function to calculate easing for smooth movement
double calculateEasing(double t, const std::string& mode) {
    if (mode == "linear") {
        return t;
    }
    else if (mode == "ease") {
        // Stronger ease-in-out using cubic easing: t^3 for in, 1-(1-t)^3 for out
        if (t < 0.5) {
            return 4 * t * t * t;
        }
        else {
            double f = (t - 1);
            return 1 + 4 * f * f * f;
        }
    }
    // Fallback to linear if mode is not recognized
    return t;
}

// Function to move the mouse cursor smoothly with improved timing
void smoothMoveMouse(int targetX, int targetY, int duration, const std::string& mode) {
    POINT currentPos;
    GetConsistentCursorPos(&currentPos);

    int startX = currentPos.x;
    int startY = currentPos.y;

    // If -1 was specified, keep the current coordinate
    targetX = (targetX != -1) ? targetX : startX;
    targetY = (targetY != -1) ? targetY : startY;

    // Skip if already at target position
    if (startX == targetX && startY == targetY) {
        return;
    }

    // Use high precision timer
    auto startTime = std::chrono::high_resolution_clock::now();
    auto endTime = startTime + std::chrono::milliseconds(duration);

    // Target frame rate - adaptive based on duration
    int targetFPS = (duration > 500) ? 60 : 120;  // Higher frame rate for shorter durations
    auto frameTime = std::chrono::milliseconds(1000 / targetFPS);

    while (true) {
        auto currentTime = std::chrono::high_resolution_clock::now();

        // Check if we've exceeded the duration
        if (currentTime >= endTime) {
            break;
        }

        // Calculate progress (0.0 to 1.0)
        double elapsedMs = std::chrono::duration<double, std::milli>(currentTime - startTime).count();
        double t = elapsedMs / duration;
        if (t > 1.0) t = 1.0;

        // Apply easing function
        double easedT = calculateEasing(t, mode);

        // Calculate new position
        int x = startX + static_cast<int>((targetX - startX) * easedT);
        int y = startY + static_cast<int>((targetY - startY) * easedT);

        // Move the cursor
        SetConsistentCursorPos(x, y);

        // Sleep for next frame, but account for processing time
        std::this_thread::sleep_for(frameTime);
    }

    // Ensure we end up exactly at the target position
    SetConsistentCursorPos(targetX, targetY);
}

// Function to simulate a mouse event
void simulateEvent(const CommandLineArgs& args) {
    if (verbose) {
        std::cout << "Execute: ";
        std::cout << "(" << args.key << ", " << args.action << ") @ (" << args.x << ", " << args.y << ") | ";
        std::cout << "Mode: " << args.mode << " | ";
        std::cout << "Sleep: " << args.sleep << " | ";
        std::cout << "Smooth: (" << args.smooth << ", " << args.smoothTime << ")\n";
    }

    POINT originalPos;
    GetConsistentCursorPos(&originalPos);

    // Handle mouse operations
    if (args.key.substr(0, 6) == "mouse_" || args.key.substr(0, 6) == "wheel_") {
        int targetX = args.x;
        int targetY = args.y;

        // If -1 was specified, keep the current coordinate
        targetX = (targetX != -1) ? targetX : originalPos.x;
        targetY = (targetY != -1) ? targetY : originalPos.y;

        // Move mouse to target position, either smoothly or instantly
        if (args.smooth != "none") {
            if (verbose) std::cout << "    Smoothly moving mouse: (" << originalPos.x << ", " << originalPos.y << ") -> (" << targetX << ", " << targetY << ")\n";
            smoothMoveMouse(targetX, targetY, args.smoothTime, args.smooth);
        }
        else {
            if (verbose) std::cout << "    Moving mouse: (" << originalPos.x << ", " << originalPos.y << ") -> (" << targetX << ", " << targetY << ")\n";
            SetConsistentCursorPos(targetX, targetY);
        }

        // Handle mouse button actions
        if (args.key == "mouse_left" || args.key == "mouse_right" || args.key == "mouse_middle") {
            // Determine the appropriate mouse event constants
            DWORD mouseDownFlag = 0;
            DWORD mouseUpFlag = 0;

            if (args.key == "mouse_left") {
                mouseDownFlag = MOUSEEVENTF_LEFTDOWN;
                mouseUpFlag = MOUSEEVENTF_LEFTUP;
            }
            else if (args.key == "mouse_right") {
                mouseDownFlag = MOUSEEVENTF_RIGHTDOWN;
                mouseUpFlag = MOUSEEVENTF_RIGHTUP;
            }
            else if (args.key == "mouse_middle") {
                mouseDownFlag = MOUSEEVENTF_MIDDLEDOWN;
                mouseUpFlag = MOUSEEVENTF_MIDDLEUP;
            }

            // Perform the requested action
            if (args.action == "click") {
                if (verbose) {
                    std::cout << "    Mouse button " << (args.key == "mouse_left" ? "left" : args.key == "mouse_right" ? "right"
                                                                                                                       : "middle")
                              << " clicked\n";
                }
                mouse_event(mouseDownFlag, 0, 0, 0, 0);
                mouse_event(mouseUpFlag, 0, 0, 0, 0);
            }
            else if (args.action == "doubleclick") {
                if (verbose) {
                    std::cout << "    Mouse button " << (args.key == "mouse_left" ? "left" : args.key == "mouse_right" ? "right"
                                                                                                                       : "middle")
                              << " double-clicked\n";
                }
                mouse_event(mouseDownFlag, 0, 0, 0, 0);
                mouse_event(mouseUpFlag, 0, 0, 0, 0);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Short delay between clicks
                mouse_event(mouseDownFlag, 0, 0, 0, 0);
                mouse_event(mouseUpFlag, 0, 0, 0, 0);
            }
            else if (args.action == "keydown") {
                if (verbose) {
                    std::cout << "    Mouse button " << (args.key == "mouse_left" ? "left" : args.key == "mouse_right" ? "right"
                                                                                                                       : "middle")
                              << " key pressed\n";
                }
                mouse_event(mouseDownFlag, 0, 0, 0, 0);
            }
            else if (args.action == "keyup") {
                if (verbose) {
                    std::cout << "    Mouse button " << (args.key == "mouse_left" ? "left" : args.key == "mouse_right" ? "right"
                                                                                                                       : "middle")
                              << " key released\n";
                }
                mouse_event(mouseUpFlag, 0, 0, 0, 0);
            }
        }
        // Handle wheel actions
        else if (args.key == "wheel_up" || args.key == "wheel_down") {
            if (verbose) std::cout << "    Mouse wheel " << (args.key == "wheel_up" ? "up" : "down") << "\n";
            int wheelDelta = (args.key == "wheel_up") ? WHEEL_DELTA : -WHEEL_DELTA;
            mouse_event(MOUSEEVENTF_WHEEL, 0, 0, wheelDelta, 0);
        }

        // Return to original position if requested
        if (args.mode == "back") {
            if (args.smooth != "none") {
                if (verbose) std::cout << "    Smoothly moving mouse back: (" << targetX << ", " << targetY << ") -> (" << originalPos.x << ", " << originalPos.y << ")\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Small delay before moving back
                smoothMoveMouse(originalPos.x, originalPos.y, args.smoothTime, args.smooth);
            }
            else {
                if (verbose) std::cout << "    Moving mouse back: (" << targetX << ", " << targetY << ") -> (" << originalPos.x << ", " << originalPos.y << ")\n";
                SetConsistentCursorPos(originalPos.x, originalPos.y);
            }
        }
    }
    // Handle keyboard operations
    else if (args.key.substr(0, 4) == "key_" && keyCodeMap.find(args.key) != keyCodeMap.end()) {
        WORD keyCode = keyCodeMap[args.key];

        if (args.action == "click") {
            // Press and release
            if (verbose) std::cout << "    Key " << args.key.substr(4) << " clicked\n";
            keybd_event(static_cast<BYTE>(keyCode), 0, 0, 0);
            keybd_event(static_cast<BYTE>(keyCode), 0, KEYEVENTF_KEYUP, 0);
        }
        else if (args.action == "doubleclick") {
            // Press and release twice
            if (verbose) std::cout << "    Key " << args.key.substr(4) << " double-clicked\n";
            keybd_event(static_cast<BYTE>(keyCode), 0, 0, 0);
            keybd_event(static_cast<BYTE>(keyCode), 0, KEYEVENTF_KEYUP, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            keybd_event(static_cast<BYTE>(keyCode), 0, 0, 0);
            keybd_event(static_cast<BYTE>(keyCode), 0, KEYEVENTF_KEYUP, 0);
        }
        else if (args.action == "keydown") {
            // Only press
            if (verbose) std::cout << "    Key " << args.key.substr(4) << " pressed down\n";
            keybd_event(static_cast<BYTE>(keyCode), 0, 0, 0);
        }
        else if (args.action == "keyup") {
            // Only release
            if (verbose) std::cout << "    Key " << args.key.substr(4) << " released\n";
            keybd_event(static_cast<BYTE>(keyCode), 0, KEYEVENTF_KEYUP, 0);
        }
    }
    // Handle switch focus operation
    else if (args.key == "switch_focus") {
        DWORD sleepTime = (args.smoothTime > 0) ? args.smoothTime : 0;  // Default to 0 ms if no sleep specified
        if (verbose) std::cout << "    Switching focus to the temporary window for " << sleepTime << " ms\n";
        if (!SwitchFocus(sleepTime)) {
            if (!quiet) std::cout << "Error: Failed to switch focus.\n";
        }
    }

    // Sleep if requested
    if (args.sleep > 0) {
        if (verbose) std::cout << "    Sleeping for " << args.sleep << " ms\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(args.sleep));
        if (args.key == "none") return;  // If only sleep was requested, exit early
    }

    // If key is "none", only sleep was needed, so we're done
    if (args.key == "none") {
        if (verbose) std::cout << "    No key specified. Exiting.\n";
        return;
    }
}

// Function to process a file with commands
void processCommandFile(const std::string& filePath) {
    if (verbose) std::cout << "Processing command file: " << filePath << "\n";

    std::ifstream inputFile(filePath);
    if (!inputFile.is_open()) {
        if (!quiet) std::cout << "Error: Could not open file: " << filePath << "\n";
        return;
    }

    std::vector<CommandLineArgs> commands;

    std::string line;
    int lineNumber = 0;

    while (std::getline(inputFile, line)) {
        lineNumber++;
        if (line.empty() || line[0] == '#') {
            // Skip empty lines and comments
            continue;
        }

        if (verbose) std::cout << "Processing line " << lineNumber << ": " << line << "\n";

        // Split line into arguments
        std::vector<std::string> args = {"program_name"};  // First arg is program name
        std::string currentArg;
        bool inQuotes = false;

        for (size_t i = 0; i < line.size(); i++) {
            char c = line[i];

            if (c == '"') {
                inQuotes = !inQuotes;
                continue;
            }

            if (c == ' ' && !inQuotes) {
                if (!currentArg.empty()) {
                    args.push_back(currentArg);
                    currentArg.clear();
                }
            }
            else {
                currentArg += c;
            }
        }

        if (!currentArg.empty()) {
            args.push_back(currentArg);
        }

        if (args.size() > 1) {
            // Convert to C-style arguments
            std::vector<char*> cArgs;
            for (auto& arg : args) {
                cArgs.push_back(&arg[0]);
            }

            // Parse and execute the command
            CommandLineArgs cmdArgs = parseCommandLine(static_cast<int>(cArgs.size()), cArgs.data());
            if (cmdArgs.validArgs) {
                commands.push_back(cmdArgs);
            }
            else {
                if (!quiet) std::cout << "Invalid arguments in line " << lineNumber << ".\n";
                return;
            }
        }
    }

    inputFile.close();
    if (commands.empty()) {
        if (!quiet) std::cout << "No valid commands found in file.\n";
        return;
    }

    // Execute all commands from the file
    if (verbose) std::cout << "Executing commands from file...\n";
    for (const auto& cmdArgs : commands) {
        simulateEvent(cmdArgs);
    }
}

// Function to execute the command based on parsed arguments
void execute(const CommandLineArgs& args) {
    // Display help if requested or if arguments are invalid
    if (args.help) {
        displayHelp();
        return;
    }

    // Only simulate event if all required arguments are valid
    if (args.validArgs) {
        // Process file if provided
        if (args.file.empty()) {
            simulateEvent(args);
        }
        else {
            processCommandFile(args.file);
        }
    }
}

// Console application entry point
int main(int argc, char* argv[]) {
    // Set DPI awareness
    setProcessDpiAwareness();

    GetConsistentCursorPos(&lastPos, TRUE);  // Initialize last position

    // Parse command line arguments
    CommandLineArgs args = parseCommandLine(argc, argv);

    // Set global verbose flag
    verbose = args.verbose;

    // Get DPI scaling
    dpiScaling = getCurrentDpiScalingFactor();
    if (verbose) std::cout << "DPI Scaling Factor: " << dpiScaling << "\n";

    // Execute the command
    execute(args);

    return 0;
}