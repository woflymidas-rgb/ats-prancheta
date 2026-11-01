#include "hotkey-settings.h"
#include <cassert>
#include <iostream>
int main() {
    using namespace hotkey_ui;
    i18n::initialize(std::filesystem::current_path()/L"assets",std::filesystem::current_path()); i18n::load("pt_br");
    assert(valid({VK_F8,0}) && valid({VK_F6,3}));
    assert(!valid({VK_F8,1}) && !valid({VK_F4,4}));
    assert(valid({VK_ESCAPE,0}) && valid({VK_TAB,0}) && valid({VK_RETURN,0}));
    assert(!valid({VK_TAB,4}) && !valid({VK_ESCAPE,1}));
    assert(!valid({'Q',3}) && !valid({VK_CONTROL,0}) && !valid({0,0}));
    auto roundtrip=from_control(control_value({VK_F6,3})); assert(roundtrip.vk==VK_F6 && roundtrip.modifiers==3);
    Conflicts c; c.controls_available=true; c.controls="mix wipers `keyboard.p?0`";
    const auto warning=c.message({'P',0});
    assert(warning.find(L"salvá-la mesmo assim")!=std::wstring::npos);
    c.brake_plugin=true; assert(c.message({VK_F9,0})==warning);
    assert(c.message({VK_F7,0})==warning && c.message({VK_F10,0})==warning);
    const auto folder=std::filesystem::current_path()/L"settings-check-data";
    assert(!std::filesystem::exists(folder)); std::filesystem::create_directory(folder);
    save(folder,{VK_F8,0}); const auto path=folder/L"settings.ini";
    assert(GetPrivateProfileIntW(L"Hotkey",L"VirtualKey",0,path.c_str())==VK_F8);
    assert(WritePrivateProfileStringW(L"Paths",L"GameHome",L"C:\\Teste São Paulo",path.c_str()));
    const auto old=read_text(path);
    bool rejected=false; try { save(folder,{VK_F8,1}); } catch(...) { rejected=true; }
    assert(rejected && read_text(path)==old);
    save(folder,{VK_F6,3});
    assert(GetPrivateProfileIntW(L"Hotkey",L"VirtualKey",0,path.c_str())==VK_F6);
    assert(GetPrivateProfileIntW(L"Hotkey",L"Modifiers",0,path.c_str())==3);
    wchar_t preserved[100]={}; GetPrivateProfileStringW(L"Paths",L"GameHome",L"",preserved,100,path.c_str());
    assert(std::wstring(preserved)==L"C:\\Teste São Paulo");
    bool backup=false;
    for(const auto& entry:std::filesystem::directory_iterator(folder)) {
        if(entry.path().filename().wstring().find(L"settings.backup.")==0) { assert(read_text(entry.path())==old); backup=true; }
    }
    assert(backup);
    for(const auto vk:{static_cast<unsigned>('P'),static_cast<unsigned>(VK_ESCAPE),static_cast<unsigned>(VK_TAB),static_cast<unsigned>(VK_RETURN)}) {
        save(folder,{vk,0}); assert(GetPrivateProfileIntW(L"Hotkey",L"VirtualKey",0,path.c_str())==vk);
    }
    save(folder,{VK_F8,0}); assert(GetPrivateProfileIntW(L"Hotkey",L"Modifiers",99,path.c_str())==0);
    for(const auto& entry:std::filesystem::directory_iterator(folder)) std::filesystem::remove(entry.path());
    std::filesystem::remove(folder);
    std::cout<<"PASS: reserved combinations; assigned game keys including P/Esc/Tab/Enter can be saved; uniform conflict warnings; persistence; Unicode settings preserved; backup; restore F8\n";
}
