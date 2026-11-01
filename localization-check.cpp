#include "document-renderer.h"
#include "hotkey-settings.h"
#include <cassert>
#include <iostream>
int main() {
    namespace fs=std::filesystem;
    const auto root=fs::current_path();
    i18n::initialize(root/L"assets",root);
    assert(i18n::detect("uset g_lang_init \"ru_ru\"\nuset g_lang_set \"ja_jp\"").empty());
    assert(i18n::detect("Selected language: pt_br\nuset g_lang \"DE-de\"")=="de_de");
    assert(i18n::detect("uset g_lang \"ru_ru\"\nSelected language: ja_jp")=="ja_jp");
    const auto english=i18n::english;
    unsigned count=0;
    for(const auto& file:fs::directory_iterator(root/L"assets"/L"i18n")) {
        const auto code=file.path().stem().string();if(!i18n::code_valid(code)) continue;
        i18n::load(code);assert(i18n::language==code);
        for(auto it=english.begin();it!=english.end();++it) if(it.key()[0]!='_') {
            assert(i18n::strings.contains(it.key()) && i18n::strings[it.key()].is_string());
            const auto value=i18n::strings[it.key()].get<std::string>();assert(!value.empty());
            assert(i18n::utf8(i18n::utf16(value))==value);
        }
        assert(i18n::format("date",{{"day","2"},{"time","17:56"}}).find('{')==std::string::npos);
        assert(i18n::format("duration",{{"h","7"},{"m","9"}}).find('{')==std::string::npos);
        hotkey_ui::Conflicts conflicts;conflicts.controls_available=true;conflicts.controls="mix wipers `keyboard.p?0`";
        const auto warning=conflicts.message({'P',0});conflicts.brake_plugin=true;
        assert(warning==conflicts.message({VK_F9,0}) && warning==i18n::wtr("conflict"));
        assert(i18n::game_name("cargo","mob_screener","CUSTOM MOD NAME")=="CUSTOM MOD NAME");
        ++count;
    }
    assert(count==43);
    i18n::load("pt_br");assert(i18n::game_name("cargo","scissor_lift","Scissor Lift")=="Plataforma pantográfica");
    assert(i18n::game_name("cargo","scissor_lift","Custom Lift")=="Custom Lift");
    assert(printed_number("39009.5",true)=="39.009,5");
    i18n::load("en_gb");assert(printed_number("39009.5",true)=="39,009.5");
    assert(i18n::game_name("cargo","mob_screener","Peneira móvel")=="Mobile Screener");
    i18n::load("../bad");assert(i18n::language=="en_gb");
    i18n::load("xx_yy");assert(i18n::language=="en_gb");
    std::cout<<"PASS: 43 locales, complete UI strings, Unicode, detection, fallback, uniform warnings, numeric formatting, official cargo translation, custom names preserved\n";
}
