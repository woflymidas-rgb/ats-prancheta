#pragma once
#include "hotkey-settings.h"
namespace hotkey_ui {
inline HWND panel=nullptr, capture=nullptr, status=nullptr, previous_game=nullptr;
inline HFONT panel_font=nullptr;
inline std::string panel_font_locale;
inline std::filesystem::path settings_directory;
inline Conflicts conflicts;
inline void (*saved_callback)(Shortcut)=nullptr;
inline Shortcut initial;
inline Shortcut pending;
inline LRESULT CALLBACK capture_proc(HWND window,UINT message,WPARAM w,LPARAM l) {
    switch(message) {
    case HKM_SETHOTKEY:
        pending=from_control(static_cast<WORD>(w)); InvalidateRect(window,nullptr,FALSE); return 0;
    case HKM_GETHOTKEY: return control_value(pending);
    case HKM_SETRULES: return 0;
    case WM_SETFONT: SetWindowLongPtrW(window,GWLP_USERDATA,static_cast<LONG_PTR>(w)); if(l) InvalidateRect(window,nullptr,FALSE); return 0;
    case WM_LBUTTONDOWN: SetFocus(window); return 0;
    case WM_SETFOCUS:
    case WM_KILLFOCUS: InvalidateRect(window,nullptr,FALSE); return 0;
    case WM_GETDLGCODE: return DLGC_WANTALLKEYS;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if(w==VK_SHIFT || w==VK_CONTROL || w==VK_MENU || w==VK_LWIN || w==VK_RWIN) return 0;
        pending={static_cast<unsigned>(w),((GetKeyState(VK_CONTROL)&0x8000)?1u:0u)|((GetKeyState(VK_SHIFT)&0x8000)?2u:0u)|((GetKeyState(VK_MENU)&0x8000)?4u:0u)};
        InvalidateRect(window,nullptr,FALSE);
        SendMessageW(GetParent(window),WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(window),EN_CHANGE),reinterpret_cast<LPARAM>(window)); return 0;
    case WM_CHAR:
    case WM_SYSCHAR: return 0;
    case WM_ERASEBKGND: return 1;
    case WM_PRINTCLIENT:
    case WM_PAINT: {
        PAINTSTRUCT ps={};HDC dc=message==WM_PAINT?BeginPaint(window,&ps):reinterpret_cast<HDC>(w);
        RECT rect;GetClientRect(window,&rect);
        HBRUSH background=CreateSolidBrush(RGB(250,250,252));FillRect(dc,&rect,background);DeleteObject(background);
        HBRUSH edge=CreateSolidBrush(GetFocus()==window?RGB(30,110,190):RGB(130,135,140));FrameRect(dc,&rect,edge);DeleteObject(edge);
        HFONT font=reinterpret_cast<HFONT>(GetWindowLongPtrW(window,GWLP_USERDATA));auto previous=SelectObject(dc,font?font:GetStockObject(DEFAULT_GUI_FONT));
        SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(30,35,40));rect.left+=12;rect.right-=12;
        const auto text=label(pending);DrawTextW(dc,text.c_str(),-1,&rect,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
        SelectObject(dc,previous);if(message==WM_PAINT) EndPaint(window,&ps);return 0;
    }
    }
    return DefWindowProcW(window,message,w,l);
}
inline int px(int value,UINT dpi) { return MulDiv(value,static_cast<int>(dpi),96); }
inline Shortcut selected() { return from_control(static_cast<WORD>(SendMessageW(capture,HKM_GETHOTKEY,0,0))); }
inline void update_message() {
    if(!status || !capture) return;
    const auto shortcut=selected();
    SetWindowTextW(status,conflicts.message(shortcut).c_str());
    EnableWindow(GetDlgItem(panel,IDOK),valid(shortcut));
}
inline void localize_panel() {
    if(!panel || !status || !capture) return;
    if(panel_font_locale!=i18n::language) {
        const auto previous=panel_font;
        panel_font=CreateFontW(-px(17,GetDpiForWindow(panel)),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,i18n::font_family());
        EnumChildWindows(panel,[](HWND child,LPARAM font)->BOOL {SendMessageW(child,WM_SETFONT,static_cast<WPARAM>(font),TRUE);return TRUE;},reinterpret_cast<LPARAM>(panel_font));
        if(previous) DeleteObject(previous);panel_font_locale=i18n::language;
    }
    SetWindowTextW(panel,(i18n::wtr("settings_title")+L" — ATS Clipboard").c_str());
    SetDlgItemTextW(panel,101,i18n::utf16(i18n::format("current_key",{{"key",i18n::utf8(label(initial))}})).c_str());
    SetDlgItemTextW(panel,102,i18n::wtr("instruction").c_str());
    SetDlgItemTextW(panel,IDOK,i18n::wtr("save").c_str());
    SetDlgItemTextW(panel,105,i18n::wtr("restore").c_str());
    SetDlgItemTextW(panel,IDCANCEL,i18n::wtr("cancel").c_str());
    update_message();
}
inline LRESULT CALLBACK panel_proc(HWND window,UINT message,WPARAM w,LPARAM l) {
    switch(message) {
    case WM_CREATE: {
        panel=window;
        const UINT dpi=GetDpiForWindow(window);
        panel_font=CreateFontW(-px(17,dpi),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,i18n::font_family());
        panel_font_locale=i18n::language;
        const auto instance=reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(window,GWLP_HINSTANCE));
        auto control=[&](const wchar_t* cls,const wchar_t* text,int id,DWORD style,int x,int y,int width,int height) {
            HWND child=CreateWindowExW(0,cls,text,WS_CHILD|WS_VISIBLE|style,px(x,dpi),px(y,dpi),px(width,dpi),px(height,dpi),window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),instance,nullptr);
            SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(panel_font),TRUE); return child;
        };
        control(L"STATIC",L"",101,SS_NOPREFIX,24,22,530,28);
        control(L"STATIC",L"",102,SS_NOPREFIX,24,63,530,52);
        WNDCLASSW capture_class={};capture_class.lpfnWndProc=capture_proc;capture_class.hInstance=instance;capture_class.hCursor=LoadCursor(nullptr,IDC_IBEAM);capture_class.lpszClassName=L"ATSPranchetaKeyCapture";
        RegisterClassW(&capture_class);
        capture=control(capture_class.lpszClassName,L"",103,WS_TABSTOP,24,128,530,36);
        status=control(L"STATIC",L"",104,SS_NOPREFIX,24,184,530,100);
        control(L"BUTTON",L"",IDOK,WS_TABSTOP|BS_DEFPUSHBUTTON,24,304,170,36);
        control(L"BUTTON",L"",105,WS_TABSTOP|BS_PUSHBUTTON,204,304,170,36);
        control(L"BUTTON",L"",IDCANCEL,WS_TABSTOP|BS_PUSHBUTTON,384,304,170,36);
        SendMessageW(capture,HKM_SETRULES,0,0);
        SendMessageW(capture,HKM_SETHOTKEY,control_value(initial),0);
        localize_panel();
        return 0;
    }
    case WM_COMMAND:
        if(LOWORD(w)==103 && HIWORD(w)==EN_CHANGE) { update_message(); return 0; }
        if(LOWORD(w)==105) {
            SendMessageW(capture,HKM_SETHOTKEY,control_value({VK_F8,0}),0);
            update_message(); SetFocus(capture); return 0;
        }
        if(LOWORD(w)==IDOK) {
            const auto shortcut=selected();
            if(!valid(shortcut)) { update_message(); return 0; }
            try {
                save(settings_directory,shortcut);
                if(saved_callback) saved_callback(shortcut);
                DestroyWindow(window);
            } catch(...) { SetWindowTextW(status,i18n::wtr("save_error").c_str()); }
            return 0;
        }
        if(LOWORD(w)==IDCANCEL) { DestroyWindow(window); return 0; }
        break;
    case WM_CLOSE: DestroyWindow(window); return 0;
    case WM_ACTIVATE:
        if(LOWORD(w)!=WA_INACTIVE) { i18n::refresh(settings_directory,true);localize_panel(); }
        return 0;
    case WM_CTLCOLORSTATIC: SetBkMode(reinterpret_cast<HDC>(w),TRANSPARENT);return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
    case WM_DESTROY:
        panel=nullptr; capture=nullptr; status=nullptr;
        if(panel_font) { DeleteObject(panel_font); panel_font=nullptr; }
        if(previous_game && IsWindow(previous_game)) {
            if(IsIconic(previous_game)) ShowWindow(previous_game,SW_RESTORE);
            SetForegroundWindow(previous_game);
        }
        return 0;
    }
    return DefWindowProcW(window,message,w,l);
}
inline void show(HWND owner,const std::filesystem::path& directory,Shortcut shortcut,HWND game,void(*callback)(Shortcut),bool display=true) {
    i18n::refresh(directory,true);
    if(panel && IsWindow(panel)) { SetForegroundWindow(panel); return; }
    settings_directory=directory; initial=shortcut; previous_game=game; saved_callback=callback;
    conflicts=inspect(directory,game);
    INITCOMMONCONTROLSEX controls={sizeof(controls),ICC_HOTKEY_CLASS}; InitCommonControlsEx(&controls);
    const auto instance=reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(owner,GWLP_HINSTANCE));
    WNDCLASSW wc={}; wc.lpfnWndProc=panel_proc; wc.hInstance=instance; wc.hCursor=LoadCursor(nullptr,IDC_ARROW); wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1); wc.lpszClassName=L"ATSPranchetaHotkeyPanel";
    RegisterClassW(&wc);
    const UINT dpi=GetDpiForWindow(owner);
    RECT bounds={0,0,px(580,dpi),px(365,dpi)};
    const DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU;
    AdjustWindowRectEx(&bounds,style,FALSE,WS_EX_TOPMOST|WS_EX_DLGMODALFRAME);
    MONITORINFO monitor={}; monitor.cbSize=sizeof(monitor); GetMonitorInfoW(MonitorFromWindow(game,MONITOR_DEFAULTTONEAREST),&monitor);
    const int width=bounds.right-bounds.left, height=bounds.bottom-bounds.top;
    panel=CreateWindowExW(WS_EX_TOPMOST|WS_EX_DLGMODALFRAME,wc.lpszClassName,(i18n::wtr("settings_title")+L" — ATS Clipboard").c_str(),style,
        monitor.rcWork.left+(monitor.rcWork.right-monitor.rcWork.left-width)/2,monitor.rcWork.top+(monitor.rcWork.bottom-monitor.rcWork.top-height)/2,
        width,height,owner,nullptr,instance,nullptr);
    if(panel && display) { ShowWindow(panel,SW_SHOWNORMAL); SetForegroundWindow(panel); SetFocus(capture); }
}
}
