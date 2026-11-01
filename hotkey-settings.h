#pragma once
#include "profile.h"
#include "language.h"
#include <commctrl.h>
#pragma comment(lib,"comctl32.lib")

namespace hotkey_ui {
struct Shortcut { unsigned vk=VK_F8, modifiers=0; };
inline bool valid(Shortcut shortcut) {
    if(shortcut.vk<VK_BACK || shortcut.vk>254 || shortcut.modifiers>7) return false;
    for(const auto forbidden:{VK_SHIFT,VK_CONTROL,VK_MENU,VK_LSHIFT,VK_RSHIFT,VK_LCONTROL,VK_RCONTROL,VK_LMENU,VK_RMENU,VK_LWIN,VK_RWIN,VK_SNAPSHOT,VK_PACKET,VK_PROCESSKEY})
        if(shortcut.vk==static_cast<unsigned>(forbidden)) return false;
    if(shortcut.vk==VK_F4 && (shortcut.modifiers&4)) return false;
    if(shortcut.vk==VK_TAB && (shortcut.modifiers&4)) return false;
    if(shortcut.vk==VK_ESCAPE && (shortcut.modifiers&5)) return false;
    if(shortcut.vk==VK_DELETE && (shortcut.modifiers&5)==5) return false;
    if(shortcut.vk=='Q' && shortcut.modifiers==3) return false;
    if(shortcut.vk==VK_F8 && shortcut.modifiers==1) return false; // Recovery shortcut opens settings.
    return true;
}
inline std::wstring label(Shortcut shortcut) {
    std::wstring text;
    if(shortcut.modifiers&1) text+=L"Ctrl + ";
    if(shortcut.modifiers&2) text+=L"Shift + ";
    if(shortcut.modifiers&4) text+=L"Alt + ";
    wchar_t name[80]={};
    auto scan=MapVirtualKeyW(shortcut.vk,MAPVK_VK_TO_VSC);
    if(shortcut.vk==VK_INSERT || shortcut.vk==VK_DELETE || shortcut.vk==VK_HOME || shortcut.vk==VK_END || shortcut.vk==VK_PRIOR || shortcut.vk==VK_NEXT || shortcut.vk==VK_LEFT || shortcut.vk==VK_RIGHT || shortcut.vk==VK_UP || shortcut.vk==VK_DOWN || shortcut.vk==VK_DIVIDE) scan|=0x100;
    if(GetKeyNameTextW(static_cast<LONG>(scan<<16),name,80)>0) text+=name;
    else text+=L"Tecla "+std::to_wstring(shortcut.vk);
    return text;
}
inline WORD control_value(Shortcut shortcut) {
    unsigned flags=0;
    if(shortcut.modifiers&1) flags|=HOTKEYF_CONTROL;
    if(shortcut.modifiers&2) flags|=HOTKEYF_SHIFT;
    if(shortcut.modifiers&4) flags|=HOTKEYF_ALT;
    return MAKEWORD(shortcut.vk,flags);
}
inline Shortcut from_control(WORD value) {
    const auto flags=HIBYTE(value);
    return {LOBYTE(value), ((flags&HOTKEYF_CONTROL)?1u:0u)|((flags&HOTKEYF_SHIFT)?2u:0u)|((flags&HOTKEYF_ALT)?4u:0u)};
}
inline void save(const std::filesystem::path& directory,Shortcut shortcut) {
    if(!valid(shortcut)) throw std::runtime_error("Invalid shortcut");
    const auto path=directory/L"settings.ini";
    const auto temporary=directory/(L"settings.pending."+std::to_wstring(GetCurrentProcessId())+L".ini");
    try {
        if(std::filesystem::exists(path)) std::filesystem::copy_file(path,temporary,std::filesystem::copy_options::overwrite_existing);
        else { std::ofstream file(temporary,std::ios::binary); file.write("\xff\xfe",2); if(!file) throw std::runtime_error("Cannot create settings"); }
        const auto vk=std::to_wstring(shortcut.vk), modifiers=std::to_wstring(shortcut.modifiers);
        if(!WritePrivateProfileStringW(L"Hotkey",L"VirtualKey",vk.c_str(),temporary.c_str()) || !WritePrivateProfileStringW(L"Hotkey",L"Modifiers",modifiers.c_str(),temporary.c_str())) throw std::runtime_error("Cannot write settings");
        WritePrivateProfileStringW(nullptr,nullptr,nullptr,temporary.c_str());
        if(std::filesystem::exists(path)) {
            GUID id={};wchar_t name[40]={};
            if(FAILED(CoCreateGuid(&id)) || !StringFromGUID2(id,name,40)) throw std::runtime_error("Cannot name backup");
            std::filesystem::copy_file(path,directory/(L"settings.backup."+std::wstring(name)+L".ini"));
        }
        if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH|MOVEFILE_COPY_ALLOWED)) throw std::runtime_error("Cannot commit settings");
    } catch(...) { DeleteFileW(temporary.c_str()); throw; }
}
inline std::string token(unsigned vk) {
    if(vk>='A' && vk<='Z') return std::string(1,static_cast<char>(vk+'a'-'A'));
    if(vk>='0' && vk<='9') return "key"+std::to_string(vk-'0');
    if(vk>=VK_F1 && vk<=VK_F24) return "f"+std::to_string(vk-VK_F1+1);
    if(vk>=VK_NUMPAD0 && vk<=VK_NUMPAD9) return "num"+std::to_string(vk-VK_NUMPAD0);
    const std::map<unsigned,std::string> names={{VK_SPACE,"space"},{VK_BACK,"backspace"},{VK_INSERT,"ins"},{VK_DELETE,"del"},{VK_HOME,"home"},{VK_END,"end"},{VK_PRIOR,"pgup"},{VK_NEXT,"pgdn"},{VK_LEFT,"larrow"},{VK_RIGHT,"rarrow"},{VK_UP,"uarrow"},{VK_DOWN,"darrow"},{VK_PAUSE,"pause"},{VK_SCROLL,"scrollock"},{VK_CAPITAL,"caps"},{VK_NUMLOCK,"numlock"},{VK_ADD,"numplus"},{VK_SUBTRACT,"numminus"},{VK_MULTIPLY,"nummultiply"},{VK_DIVIDE,"numslash"},{VK_DECIMAL,"numperiod"}};
    const auto found=names.find(vk); return found==names.end()?"":found->second;
}
inline std::filesystem::path game_home(const std::filesystem::path& settings) {
    wchar_t value[32768]={}; GetPrivateProfileStringW(L"Paths",L"GameHome",L"",value,32768,(settings/L"settings.ini").c_str());
    if(value[0]) return value;
    PWSTR docs=nullptr; if(FAILED(SHGetKnownFolderPath(FOLDERID_Documents,0,nullptr,&docs))) return {};
    auto result=std::filesystem::path(docs)/L"American Truck Simulator"; CoTaskMemFree(docs); return result;
}
struct Conflicts {
    std::string controls;
    bool controls_available=false, brake_plugin=false;
    unsigned brake_menu=VK_F10, brake_calibration=VK_F7;
    std::wstring message(Shortcut shortcut) const {
        if(!valid(shortcut)) return i18n::wtr("invalid");
        const auto warning=i18n::wtr("conflict");
        if(brake_plugin && (shortcut.vk==VK_F9 || shortcut.vk==brake_menu || shortcut.vk==brake_calibration)) return warning;
        const auto name=token(shortcut.vk);
        if(!name.empty() && controls.find("keyboard."+name+"?")!=std::string::npos) {
            return warning;
        }
        if(!controls_available || name.empty()) return i18n::wtr("unknown");
        return i18n::wtr("clear");
    }
};
inline Conflicts inspect(const std::filesystem::path& directory,HWND game) {
    Conflicts result;
    try {
        const auto profile=read_profile(directory);
        const bool steam=profile.rfind("steam-",0)==0;
        if(!profile.empty()) {
            const auto id=profile.substr(6);
            result.controls=read_text(game_home(directory)/(steam?L"steam_profiles":L"profiles")/id/L"controls.sii");
            result.controls_available=true;
        }
    } catch(...) { /* Continue inspecting the plugin if the profile controls are unavailable. */ }
    try {
        DWORD pid=0; GetWindowThreadProcessId(game,&pid);
        HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
        if(process) {
            wchar_t executable[32768]; DWORD size=32768;
            const bool ok=QueryFullProcessImageNameW(process,0,executable,&size)!=0; CloseHandle(process);
            if(ok) {
                const auto plugins=std::filesystem::path(executable).parent_path()/L"plugins";
                result.brake_plugin=std::filesystem::exists(plugins/L"ets2_brake_fade.dll");
                result.brake_menu=GetPrivateProfileIntW(L"BrakeFade",L"menu_key",VK_F10,(plugins/L"brake_fade_v07.ini").c_str());
                result.brake_calibration=GetPrivateProfileIntW(L"BrakeFade",L"calibration_key",VK_F7,(plugins/L"brake_fade_v07.ini").c_str());
            }
        }
    } catch(...) { /* Missing controls do not prevent choosing a shortcut. */ }
    return result;
}
}
