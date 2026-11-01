#include "document.h"
#include "storage.h"
#include "document-renderer.h"
#include "hotkey-panel.h"
#include "clipboard-audio.h"

#include <algorithm>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

static std::filesystem::path root;
static Json rendered_document;
static std::filesystem::path visual_assets;
static bool showing = false, was_pressed = false, preview = false;
static int width = 600, height = 900, target_x = 0, target_y = 0, start_x = 0, start_y = 0;
static ULONGLONG animation_start = 0;
static unsigned viewer_hotkey = VK_F8;
static unsigned viewer_modifiers = 0;
static std::filesystem::file_time_type last_write;
static bool settings_was_pressed=false;
static clipboard_audio::EmissionObserver emission_observer;
static ULONGLONG last_audio_poll=0;
static void apply_shortcut(hotkey_ui::Shortcut shortcut) {
    viewer_hotkey=shortcut.vk; viewer_modifiers=shortcut.modifiers; was_pressed=true;
    if(hotkey_ui::panel) InvalidateRect(GetWindow(hotkey_ui::panel,GW_OWNER),nullptr,FALSE);
}
static void show_settings(HWND owner) {
    hotkey_ui::show(owner,root,{viewer_hotkey,viewer_modifiers},GetForegroundWindow(),apply_shortcut);
}
static bool game_focused(HWND window) {
    static HWND cached_window = nullptr;
    static bool cached_result = false;
    const HWND foreground = GetForegroundWindow();

    if (window == foreground) return true;
    if (foreground == cached_window) return cached_result;
    cached_window = foreground;
    cached_result = false;
    DWORD pid = 0;
    GetWindowThreadProcessId(foreground, &pid);
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return false;
    wchar_t path[32768]; DWORD size = 32768;
    const bool found = QueryFullProcessImageNameW(process, 0, path, &size) && _wcsicmp(std::filesystem::path(path).filename().c_str(), L"amtrucks.exe") == 0;
    CloseHandle(process);
    cached_result = found;
    return found;
}
static void refresh(HWND window) {
    const auto previous_language=i18n::language;
    i18n::refresh(root);
    if(previous_language!=i18n::language) {InvalidateRect(window,nullptr,FALSE);}
    try {
        const auto path = root / L"current-render.json";
        if (!std::filesystem::exists(path)) { rendered_document = nullptr; InvalidateRect(window,nullptr,FALSE); return; }
        const auto stamp = std::filesystem::last_write_time(path);
        if (stamp != last_write) {
            rendered_document = Json::parse(read_text(path)); last_write = stamp;
            InvalidateRect(window,nullptr,FALSE);
        }
    } catch (...) { rendered_document = nullptr; InvalidateRect(window,nullptr,FALSE); }
}
static void open_clipboard(HWND window) {
    i18n::refresh(root,true);
    MONITORINFO info = {}; info.cbSize = sizeof(info);
    GetMonitorInfoW(MonitorFromWindow(GetForegroundWindow(), MONITOR_DEFAULTTONEAREST), &info);
    const auto& r = info.rcWork;
    height = (std::min)(900, static_cast<int>(r.bottom - r.top - 40));
    width = height * 2 / 3;
    target_x = r.right - width - 24; target_y = r.top + (r.bottom - r.top - height) / 2;
    start_x = r.right - width / 3; start_y = r.bottom - height / 5;
    animation_start = GetTickCount64(); showing = true;
    if(!preview) clipboard_audio::play(visual_assets,false);
    refresh(window);
    SetWindowPos(window, HWND_TOPMOST, start_x, start_y, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    
    InvalidateRect(window, nullptr, FALSE);
}
static LRESULT CALLBACK procedure(HWND window, UINT message, WPARAM w, LPARAM l) {
    switch (message) {

    case WM_TIMER: {
        const bool focused=preview || game_focused(window);

        const auto audio_now=GetTickCount64();
        if(audio_now-last_audio_poll>=250) {
            last_audio_poll=audio_now;
            try { if(emission_observer.consume(Json::parse(read_text(root/L"emission-event.json")),static_cast<long long>(std::time(nullptr)),focused && !preview)) { clipboard_audio::play(visual_assets,true); if(!showing) open_clipboard(window);else refresh(window); } }catch(...){}
        }
        if(hotkey_ui::panel && IsWindowVisible(hotkey_ui::panel)) return 0;
        if (!preview && !game_focused(window)) { if (showing) { ShowWindow(window, SW_HIDE);  showing = false; } was_pressed = false; settings_was_pressed=false; return 0; }
        const bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        const bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        const bool win = ((GetAsyncKeyState(VK_LWIN) | GetAsyncKeyState(VK_RWIN)) & 0x8000) != 0;
        const unsigned modifiers = (ctrl ? 1u : 0u) | (shift ? 2u : 0u) | (alt ? 4u : 0u) | (win ? 8u : 0u);
        const bool configure=modifiers==1 && (GetAsyncKeyState(VK_F8)&0x8000)!=0;
        if(configure && !settings_was_pressed) { settings_was_pressed=true; show_settings(window); return 0; }
        settings_was_pressed=configure;
        const bool pressed = modifiers == viewer_modifiers && ((GetAsyncKeyState(static_cast<int>(viewer_hotkey)) & 0x8000) != 0);
        if (pressed && !was_pressed) {
            if (showing) { ShowWindow(window, SW_HIDE);  showing = false; }
            else open_clipboard(window);
        }
        was_pressed = pressed;
        if (ctrl && shift && (GetAsyncKeyState('Q') & 0x8000)) { DestroyWindow(window); return 0; }
        if (showing && !pressed && (GetAsyncKeyState(VK_ESCAPE) & 0x8000)) { ShowWindow(window, SW_HIDE);  showing = false; }
        if (showing) {
            const double t = (std::min)(1.0, static_cast<double>(GetTickCount64() - animation_start) / 300.0);
            const double eased = 1 - (1 - t) * (1 - t) * (1 - t);
            if (t < 1) SetWindowPos(window, HWND_TOPMOST, start_x + static_cast<int>((target_x - start_x) * eased), start_y + static_cast<int>((target_y - start_y) * eased), 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
            else SetWindowPos(window, HWND_TOPMOST, target_x, target_y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
            
            static ULONGLONG last_refresh = 0;
            const auto now = GetTickCount64();
            if (now - last_refresh > 500) { refresh(window); last_refresh = now; }
        }
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps = {}; BeginPaint(window,&ps);
        RECT rect; GetWindowRect(window,&rect);
        const int wpx=rect.right-rect.left, hpx=rect.bottom-rect.top;
        Gdiplus::Bitmap canvas(wpx,hpx,PixelFormat32bppPARGB);
        { Gdiplus::Graphics graphics(&canvas); draw_clipboard(graphics,wpx,hpx,rendered_document,visual_assets,hotkey_ui::label({viewer_hotkey,viewer_modifiers})); }
        HDC screen=GetDC(nullptr), memory=CreateCompatibleDC(screen);
        BITMAPINFO info={}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth=wpx; info.bmiHeader.biHeight=-hpx; info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
        void* pixels=nullptr; HBITMAP bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
        if(bitmap && pixels) {
            Gdiplus::BitmapData data={}; Gdiplus::Rect bounds(0,0,wpx,hpx);
            if(canvas.LockBits(&bounds,Gdiplus::ImageLockModeRead,PixelFormat32bppPARGB,&data)==Gdiplus::Ok) {
                for(int y=0;y<hpx;++y) memcpy(static_cast<unsigned char*>(pixels)+static_cast<size_t>(y)*wpx*4,static_cast<unsigned char*>(data.Scan0)+static_cast<ptrdiff_t>(y)*data.Stride,static_cast<size_t>(wpx)*4);
                canvas.UnlockBits(&data);
                auto old=SelectObject(memory,bitmap);
                POINT pos={rect.left,rect.top}, origin={0,0}; SIZE size={wpx,hpx}; BLENDFUNCTION blend={AC_SRC_OVER,0,255,AC_SRC_ALPHA};
                UpdateLayeredWindow(window,screen,&pos,&size,memory,&origin,0,&blend,ULW_ALPHA);
                SelectObject(memory,old);
            }
            DeleteObject(bitmap);
        }
        DeleteDC(memory); ReleaseDC(nullptr,screen); EndPaint(window,&ps); return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_DESTROY: clipboard_audio::stop();KillTimer(window, 1); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(window, message, w, l);
}
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR args, int) {
    Gdiplus::GdiplusStartupInput startup; ULONG_PTR token=0;
    if(Gdiplus::GdiplusStartup(&token,&startup,nullptr)!=Gdiplus::Ok) return 1;
    struct Shutdown { ULONG_PTR value; ~Shutdown(){ release_visual_resources(); Gdiplus::GdiplusShutdown(value); } } shutdown{token};

    const bool render_settings=args && wcsstr(args,L"--render-settings");
    const bool render_only = render_settings || (args && wcsstr(args, L"--render-preview"));
    HANDLE singleton = CreateMutexW(nullptr, TRUE, render_only ? L"Local\\ATSPranchetaRender" : L"Local\\ATSPranchetaViewerPrototype");
    if (!singleton || GetLastError() == ERROR_ALREADY_EXISTS) { if (singleton) CloseHandle(singleton); return 0; }
    SetProcessDPIAware();
    try { root = render_only ? std::filesystem::current_path() : data_root(); } catch (...) { MessageBoxW(nullptr, i18n::wtr("open_error").c_str(), L"ATS Clipboard", MB_OK); CloseHandle(singleton); return 1; }
    wchar_t module[32768]; GetModuleFileNameW(nullptr,module,32768); visual_assets=std::filesystem::path(module).parent_path()/L"assets";
    i18n::initialize(visual_assets,root);
    if(render_only && args) if(const auto option=wcsstr(args,L"--locale=")) {
        const auto code=i18n::utf8(std::wstring(option+9).substr(0,5));i18n::preview_override=true;i18n::load(code);
    }
    viewer_hotkey = GetPrivateProfileIntW(L"Hotkey", L"VirtualKey", VK_F8, (root / L"settings.ini").c_str());
    viewer_modifiers = GetPrivateProfileIntW(L"Hotkey", L"Modifiers", 0, (root / L"settings.ini").c_str());
    if (viewer_hotkey < VK_BACK || viewer_hotkey > 254) viewer_hotkey = VK_F8;
    if (viewer_modifiers > 15) viewer_modifiers = 0;
    if(!hotkey_ui::valid({viewer_hotkey,viewer_modifiers})) { viewer_hotkey=VK_F8; viewer_modifiers=0; }
    preview = args && wcsstr(args, L"--preview");
    WNDCLASSW wc = {}; wc.lpfnWndProc = procedure; wc.hInstance = instance; wc.lpszClassName = L"ATSPranchetaViewer"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);
    HWND window = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT, wc.lpszClassName, L"ATS Clipboard", WS_POPUP, 0, 0, width, height, nullptr, nullptr, instance, nullptr);
    if (!window) { CloseHandle(singleton); return 1; }
    if(render_settings) {
        hotkey_ui::show(window,root,{VK_F8,0},nullptr,nullptr,false);
        if(!hotkey_ui::panel) {DestroyWindow(window);CloseHandle(singleton);return 1;}
        bool fits=true;
        EnumChildWindows(hotkey_ui::panel,[](HWND child,LPARAM data)->BOOL {
            RECT parent_bounds,child_bounds; const auto parent=GetParent(child);
            GetClientRect(parent,&parent_bounds); GetWindowRect(child,&child_bounds);
            MapWindowPoints(nullptr,parent,reinterpret_cast<POINT*>(&child_bounds),2);
            if(child_bounds.left<0 || child_bounds.top<0 || child_bounds.right>parent_bounds.right || child_bounds.bottom>parent_bounds.bottom) *reinterpret_cast<bool*>(data)=false;
            return TRUE;
        },reinterpret_cast<LPARAM>(&fits));
        RECT rectangle; GetWindowRect(hotkey_ui::panel,&rectangle);
        HDC screen=GetDC(nullptr),dc=CreateCompatibleDC(screen);
        HBITMAP bitmap=CreateCompatibleBitmap(screen,rectangle.right-rectangle.left,rectangle.bottom-rectangle.top);
        auto old=SelectObject(dc,bitmap); SendMessageW(hotkey_ui::panel,WM_PRINT,reinterpret_cast<WPARAM>(dc),PRF_CLIENT|PRF_NONCLIENT|PRF_CHILDREN|PRF_ERASEBKGND); GdiFlush();
        Gdiplus::Bitmap image(bitmap,nullptr);
        UINT count=0,bytes=0;Gdiplus::GetImageEncodersSize(&count,&bytes);
        std::vector<unsigned char> encoders(bytes);auto codecs=reinterpret_cast<Gdiplus::ImageCodecInfo*>(encoders.data());Gdiplus::GetImageEncoders(count,bytes,codecs);
        Gdiplus::Status result=Gdiplus::GenericError;
        for(UINT i=0;i<count;++i) if(wcscmp(codecs[i].MimeType,L"image/png")==0){result=image.Save((root/L"settings-preview.png").c_str(),&codecs[i].Clsid,nullptr);break;}
        SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);ReleaseDC(nullptr,screen);
        DestroyWindow(hotkey_ui::panel);DestroyWindow(window);CloseHandle(singleton);
        return fits && result==Gdiplus::Ok?0:1;
    }
    if (render_only) {
        try { rendered_document=Json::parse(read_text(root/L"preview-render.json")); } catch (...) { rendered_document=nullptr; }
        Gdiplus::Bitmap canvas(1024,1536,PixelFormat32bppPARGB);
        { Gdiplus::Graphics graphics(&canvas); draw_clipboard(graphics,1024,1536,rendered_document,visual_assets,hotkey_ui::label({viewer_hotkey,viewer_modifiers})); }
        UINT count=0,bytes=0; Gdiplus::GetImageEncodersSize(&count,&bytes);
        std::vector<unsigned char> encoders(bytes); auto codecs=reinterpret_cast<Gdiplus::ImageCodecInfo*>(encoders.data()); Gdiplus::GetImageEncoders(count,bytes,codecs);
        Gdiplus::Status status=Gdiplus::GenericError;
        for(UINT i=0;i<count;++i) if(wcscmp(codecs[i].MimeType,L"image/png")==0) {status=canvas.Save((root/L"preview-realistic.png").c_str(),&codecs[i].Clsid,nullptr);break;}
        DestroyWindow(window); CloseHandle(singleton); return status==Gdiplus::Ok?0:1;
    }
    emission_observer.prime(root/L"emission-event.json");

    SetTimer(window, 1, 20, nullptr);
    if (preview) open_clipboard(window);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if(hotkey_ui::panel && IsDialogMessageW(hotkey_ui::panel,&msg)) continue;
        TranslateMessage(&msg); DispatchMessageW(&msg);
    }
    CloseHandle(singleton);
    return 0;
}



