#include <windows.h>
#include <iostream>

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
        WS_EX_TOOLWINDOW,         // Prevents showing in taskbar
        CLASS_NAME,
        "Temporary Focus Window",
        WS_POPUP,                 // No border, title bar, etc.
        0, 0,                     // Position (0,0)
        0, 0,                     // Size (0x0)
        NULL,                     // No parent window
        NULL,                     // No menu
        GetModuleHandle(NULL),
        NULL
    );
    
    if (tempWindow == NULL) {
        std::cerr << "Failed to create temporary window. Error: " << GetLastError() << std::endl;
        return FALSE;
    }
    
    // Show and activate the window (steal focus)
    ShowWindow(tempWindow, SW_SHOW);
    SetForegroundWindow(tempWindow);
    
    // Wait for the specified time
    Sleep(sleepTimeMs);
    
    // Return focus to the original window
    // Attach thread input to help with focus transfer
    if (AttachThreadInput(currentThreadId, originalThreadId, TRUE)) {
        SetForegroundWindow(originalForegroundWindow);
        AttachThreadInput(currentThreadId, originalThreadId, FALSE);
    } else {
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

int main() {
    // Example usage: Switch focus with a 100ms delay
    if (SwitchFocus(1000)) {
        std::cout << "Focus has been successfully switched and returned." << std::endl;
    } else {
        std::cerr << "Focus switching failed." << std::endl;
        return 1;
    }
    
    return 0;
}