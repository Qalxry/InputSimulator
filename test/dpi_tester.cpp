#include <windows.h>
#include <iostream>
#include <string>

// 声明DPI感知相关API
#include <ShellScalingAPI.h>
#pragma comment(lib, "Shcore.lib")
#pragma comment(lib, "User32.lib")

// 设置进程DPI感知级别
void setProcessDpiAwareness() {
    // 尝试设置Per Monitor v2 DPI感知（Windows 10 1703+）
    auto setDpiAwarenessContext = [](DPI_AWARENESS_CONTEXT context) {
        using SetDpiAwarenessContextFn = DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT);
        static auto fn = reinterpret_cast<SetDpiAwarenessContextFn>(
            GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext"));
        return fn && fn(context);
    };

    // 尝试设置DPI感知级别（Windows 8.1+）
    auto setDpiAwareness = [](PROCESS_DPI_AWARENESS value) {
        using SetDpiAwarenessFn = HRESULT(WINAPI*)(PROCESS_DPI_AWARENESS);
        static auto fn = reinterpret_cast<SetDpiAwarenessFn>(
            GetProcAddress(GetModuleHandleW(L"shcore.dll"), "SetProcessDpiAwareness"));
        return fn && SUCCEEDED(fn(value));
    };

    // 尝试设置DPI感知（Vista+）
    auto setDpiAware = []() {
        using SetDpiAwareFn = BOOL(WINAPI*)();
        static auto fn = reinterpret_cast<SetDpiAwareFn>(
            GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDPIAware"));
        return fn && fn();
    };

    // 优先级：Per Monitor v2 > Per Monitor > System > Legacy
    if (!setDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        if (!setDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE)) {
            if (!setDpiAwareness(PROCESS_SYSTEM_DPI_AWARE)) {
                setDpiAware();
            }
        }
    }
}

// 获取当前鼠标位置所在显示器的DPI缩放因子
double getCurrentDpiScalingFactor() {
    // 获取当前鼠标位置
    POINT pt;
    GetCursorPos(&pt);
    
    // 获取显示器句柄
    HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    
    // 尝试使用GetDpiForMonitor（Windows 8.1+）
    using GetDpiForMonitorFn = HRESULT(WINAPI*)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);
    static auto getDpiForMonitor = reinterpret_cast<GetDpiForMonitorFn>(
        GetProcAddress(GetModuleHandleW(L"shcore.dll"), "GetDpiForMonitor"));
    
    if (getDpiForMonitor) {
        UINT dpiX = 0, dpiY = 0;
        if (SUCCEEDED(getDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
            return static_cast<double>(dpiX) / 96.0;
        }
    }
    
    // 回退方法：获取主显示器DPI
    HDC hdc = GetDC(NULL);
    const int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(NULL, hdc);
    
    return static_cast<double>(dpiX) / 96.0;
}

// 获取显示器原始分辨率
void getMonitorRawResolution(int& width, int& height) {
    DEVMODEW dm = {0};
    dm.dmSize = sizeof(dm);
    if (EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dm)) {
        width = dm.dmPelsWidth;
        height = dm.dmPelsHeight;
    } else {
        // 回退到系统指标
        width = GetSystemMetrics(SM_CXSCREEN);
        height = GetSystemMetrics(SM_CYSCREEN);
    }
}

int main() {
    // 设置DPI感知
    setProcessDpiAwareness();
    
    // 获取DPI缩放因子
    double dpiScalingFactor = getCurrentDpiScalingFactor();
    
    // 获取显示器原始分辨率
    int rawWidth = 0, rawHeight = 0;
    getMonitorRawResolution(rawWidth, rawHeight);
    
    // 获取当前鼠标位置
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    
    // 输出详细信息
    std::cout << "System Configuration:\n";
    std::cout << "----------------------\n";
    std::cout << "  DPI Scaling Factor: " << dpiScalingFactor << "\n";
    std::cout << "  Raw Resolution: " << rawWidth << "x" << rawHeight << "\n";
    std::cout << "  Scaled Resolution: " 
              << static_cast<int>(rawWidth / dpiScalingFactor) << "x"
              << static_cast<int>(rawHeight / dpiScalingFactor) << "\n";
    std::cout << "\nMouse Position:\n";
    std::cout << "----------------------\n";
    std::cout << "  Physical Coordinates: (" << cursorPos.x << ", " << cursorPos.y << ")\n";
    std::cout << "  Logical Coordinates: (" 
              << static_cast<int>(cursorPos.x / dpiScalingFactor) << ", "
              << static_cast<int>(cursorPos.y / dpiScalingFactor) << ")\n";
    
    return 0;
}
