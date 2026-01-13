#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>

// Vincular las bibliotecas necesarias
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdi32.lib")

// Constantes
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TRAY_STARTUP_ENABLE 1002
#define ID_TRAY_STARTUP_DISABLE 1003
#define ID_TRAY_ABOUT 1004
#define ID_TRAY_ALLOW_CLOSE 1005
#define ID_TRAY_FORBID_CLOSE 1006

#define TIMER_WORK_ID 1
#define TIMER_REST_ID 2

// Configuración
const int WORK_DURATION_MS = 1180 * 1000; // 1180 segundos (tiempo de trabajo)
const int REST_DURATION_MS = 20 * 1000;   // 20 segundos (tiempo de descanso)
const wchar_t* CLASS_NAME = L"EyeCareReminder_HUD";

// Variables Globales
HWND g_hMainWnd = NULL; // Ventana oculta que maneja la bandeja del sistema y los temporizadores
HWND g_hHudWnd = NULL;  // La ventana visual del recordatorio
NOTIFYICONDATAW g_nid = { 0 };
bool g_bAllowCloseWindow = false; // Prohibir cerrar la Ventana de Recordatorio por defecto

// --- Ayudantes del Registro (Inicio Automático) ---
bool IsAutoStartEnabled() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return false;

    DWORD dwType, dwSize = 0;
    bool exists = RegQueryValueExW(hKey, L"EyeCareReminder", NULL, &dwType, NULL, &dwSize) == ERROR_SUCCESS;
    RegCloseKey(hKey);
    return exists;
}

void SetAutoStart(bool enable) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey) != ERROR_SUCCESS)
        return;

    if (enable) {
        wchar_t szPath[MAX_PATH];
        GetModuleFileNameW(NULL, szPath, MAX_PATH);
        RegSetValueExW(hKey, L"EyeCareReminder", 0, REG_SZ, (BYTE*)szPath, (wcslen(szPath) + 1) * sizeof(wchar_t));
    }
    else {
        RegDeleteValueW(hKey, L"EyeCareReminder");
    }
    RegCloseKey(hKey);
}

// --- Lógica de la Ventana de Recordatorio (HUD) ---
LRESULT CALLBACK HudWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        // Iniciar la cuenta regresiva de 20 segundos para cerrar esta ventana
        SetTimer(hWnd, TIMER_REST_ID, REST_DURATION_MS, NULL);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // 1. Dibujar Fondo Moderno Oscuro
        RECT rc;
        GetClientRect(hWnd, &rc);
        HBRUSH hBrush = CreateSolidBrush(RGB(40, 40, 40)); // Gris Oscuro
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);

        // 2. Dibujar Texto
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255)); // Texto Blanco

        // Crear una fuente grande y agradable
        HFONT hFont = CreateFontW(
            48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI"
        );
        SelectObject(hdc, hFont);

        // Dibujar Mensaje Principal
        RECT rcText = rc;
        rcText.top += 50; // Desplazar hacia abajo 50 píxeles
        rcText.bottom -= 20; // Ajustar el límite inferior
        DrawTextW(hdc, L"Descansa un instante,\nlevántate y pasea.", -1, &rcText, DT_CENTER | DT_VCENTER);

        // Dibujar Mensaje Secundario (más pequeño)
        DeleteObject(hFont);
        hFont = CreateFontW(
            30, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI"
        );
        SelectObject(hdc, hFont);
        
        rcText = rc;
        rcText.top += 110; // Desplazar hacia abajo
        // DrawTextW(hdc, L"20秒后自动隐藏...", -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        DrawTextW(hdc, L"Se ocultará en 20 segundos...", -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        DeleteObject(hFont);
        EndPaint(hWnd, &ps);
    }
    return 0;

    case WM_TIMER:
        if (wParam == TIMER_REST_ID) {
            DestroyWindow(hWnd); // Cerrarse a sí misma
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hWnd, TIMER_REST_ID);
        g_hHudWnd = NULL;
        return 0;
        
    // Permitir hacer clic en la ventana para cerrarla inmediatamente
    case WM_LBUTTONUP:
        if (g_bAllowCloseWindow) {
            DestroyWindow(hWnd);
        }
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void ShowReminderHUD() {
    if (g_hHudWnd) return; // Ya se está mostrando

    // Obtener dimensiones de la pantalla
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int width = 500;
    int height = 300;
    int x = (screenW - width) / 2;
    int y = (screenH - height) / 2;

    // Crear la ventana
    g_hHudWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED, // En capas para transparencia
        CLASS_NAME,
        L"Reminder",
        WS_POPUP, // Sin borde, sin barra de título
        x, y, width, height,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (g_hHudWnd) {
        // Establecer Opacidad (Alpha = 230/255)
        SetLayeredWindowAttributes(g_hHudWnd, 0, 230, LWA_ALPHA);
        
        // Mostrar sin robar el foco (SW_SHOWNOACTIVATE)
        ShowWindow(g_hHudWnd, SW_SHOWNOACTIVATE);
        UpdateWindow(g_hHudWnd);
    }
}

// --- Bandeja del Sistema y Lógica Principal ---
void ShowTrayMenu(HWND hWnd) {
    HMENU hMenu = CreatePopupMenu();

    if (IsAutoStartEnabled()) {
        AppendMenuW(hMenu, MF_STRING, ID_TRAY_STARTUP_DISABLE, L"关闭开机启动");
    }
    else {
        AppendMenuW(hMenu, MF_STRING, ID_TRAY_STARTUP_ENABLE, L"启用开机启动");
    }

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    
    if (g_bAllowCloseWindow) {
        AppendMenuW(hMenu, MF_STRING, ID_TRAY_FORBID_CLOSE, L"禁止关闭弹窗");
    }
    else {
        AppendMenuW(hMenu, MF_STRING, ID_TRAY_ALLOW_CLOSE, L"允许关闭弹窗");
    }

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_ABOUT, L"关于");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"退出");

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hWnd); // Necesario para que el menú se cierre correctamente al hacer clic fuera
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        // Temporizador principal para el intervalo de 1180 segundos
        SetTimer(hWnd, TIMER_WORK_ID, WORK_DURATION_MS, NULL);
        break;

    case WM_POWERBROADCAST:
        // Notificaciones de suspensión/reanudación del sistema: detener temporizadores y destruir HUD al suspender, reiniciar temporizador principal al reanudar
        switch (wParam) {
        case PBT_APMSUSPEND:
            KillTimer(hWnd, TIMER_WORK_ID);
            if (g_hHudWnd) DestroyWindow(g_hHudWnd);
            break;
        case PBT_APMRESUMECRITICAL:
        case PBT_APMRESUMESUSPEND:
        case PBT_APMRESUMEAUTOMATIC:
            // Resetear el temporizador principal en al reanudación
            KillTimer(hWnd, TIMER_WORK_ID);
            SetTimer(hWnd, TIMER_WORK_ID, WORK_DURATION_MS, NULL);
            break;
        }
        return TRUE;

    case WM_TIMER:
        if (wParam == TIMER_WORK_ID) {
            ShowReminderHUD();
        }
        break;

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            ShowTrayMenu(hWnd);
        }
        else if (lParam == WM_LBUTTONDBLCLK) {
            // Opcional: Doble clic en la bandeja para probar el recordatorio inmediatamente
            ShowReminderHUD(); 
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_TRAY_EXIT:
            DestroyWindow(hWnd); // Desencadena WM_DESTROY
            break;
        case ID_TRAY_STARTUP_ENABLE:
            SetAutoStart(true);
            break;
        case ID_TRAY_STARTUP_DISABLE:
            SetAutoStart(false);
            break;
        case ID_TRAY_ALLOW_CLOSE:
            g_bAllowCloseWindow = true;
            break;
        case ID_TRAY_FORBID_CLOSE:
            g_bAllowCloseWindow = false;
            break;
        case ID_TRAY_ABOUT:
            MessageBoxW(NULL, L"EyeCare Reminder v2.0\n\n作者：delfino @qhdou\n\n功能：每1180秒提醒休息20秒", L"关于", MB_OK | MB_ICONINFORMATION);
            break;
        }
        break;

    case WM_DESTROY:
        if (g_hHudWnd) DestroyWindow(g_hHudWnd);
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Verificación de Instancia Única
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"EyeCareReminderMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }

    // Registrar Clase de Ventana HUD
    WNDCLASSEXW wch = { 0 };
    wch.cbSize = sizeof(WNDCLASSEXW);
    wch.lpfnWndProc = HudWndProc;
    wch.hInstance = hInstance;
    wch.hCursor = LoadCursor(NULL, IDC_ARROW);
    wch.lpszClassName = CLASS_NAME;
    RegisterClassExW(&wch);

    // Registrar Clase de Ventana Principal (Oculta)
    WNDCLASSEXW wcm = { 0 };
    wcm.cbSize = sizeof(WNDCLASSEXW);
    wcm.lpfnWndProc = MainWndProc;
    wcm.hInstance = hInstance;
    wcm.lpszClassName = L"EyeCareHidden";
    RegisterClassExW(&wcm);

    // Crear Ventana Principal Oculta
    g_hMainWnd = CreateWindowW(L"EyeCareHidden", L"EyeCareService", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    // Configurar Icono de Bandeja
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = g_hMainWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    
    // Intentar cargar icono personalizado, recurrir al icono de información del sistema
    // Nota: Convertido a WString para manejo de rutas
    wchar_t szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, MAX_PATH);
    std::wstring iconPath = szPath;
    iconPath = iconPath.substr(0, iconPath.find_last_of(L"\\")) + L"\\tray.ico";
    
    g_nid.hIcon = (HICON)LoadImageW(NULL, iconPath.c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
    if (!g_nid.hIcon) g_nid.hIcon = LoadIcon(NULL, IDI_INFORMATION); // Recurso predeterminado
    
    wcscpy_s(g_nid.szTip, L"EyeCare Reminder - kuhli @qhdou");
    Shell_NotifyIconW(NIM_ADD, &g_nid);

    // Bucle de Mensajes
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(hMutex) CloseHandle(hMutex);
    return 0;
}