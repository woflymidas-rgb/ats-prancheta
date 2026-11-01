#pragma once
#include "branding.h"
namespace i18n {
inline std::string language="en_gb";
inline Json strings,english,known_names;
inline std::filesystem::path resource_directory;
inline std::filesystem::file_time_type log_stamp;
inline std::filesystem::path previous_log_path;
inline bool preview_override=false;
inline ULONGLONG last_probe=0;
inline std::string detect(const std::string& log) {
    const std::regex expression("(?:Selected language:\\s*|uset\\s+g_lang\\s+\")([A-Za-z]{2}[_-][A-Za-z]{2})");
    std::string code;
    for(auto it=std::sregex_iterator(log.begin(),log.end(),expression);it!=std::sregex_iterator();++it) code=(*it)[1];
    std::transform(code.begin(),code.end(),code.begin(),[](unsigned char c){return static_cast<char>(c=='-'?'_':std::tolower(c));});
    return code;
}
inline bool code_valid(const std::string& code) {
    return code.size()==5 && code[2]=='_' && code[0]>='a' && code[0]<='z' && code[1]>='a' && code[1]<='z' && code[3]>='a' && code[3]<='z' && code[4]>='a' && code[4]<='z';
}
inline void load(const std::string& requested) {
    std::string code=code_valid(requested)?requested:"en_gb";
    try { strings=Json::parse(read_text(resource_directory/(code+".json")));language=code; }
    catch(...) {strings=english;language="en_gb";}
}
inline std::string tr(const char* id) {
    if(strings.contains(id) && strings[id].is_string()) return strings[id].get<std::string>();
    if(english.contains(id) && english[id].is_string()) return english[id].get<std::string>();
    if(std::string(id)=="toggle") return "Open/close";
    if(std::string(id)=="issued") return "Document issued";
    if(std::string(id)=="open_error") return "Could not open clipboard storage.";
    return id;
}
inline std::string format(const char* id,const std::map<std::string,std::string>& parameters) {
    auto text=tr(id);
    for(const auto& item:parameters) {
        const auto token="{"+item.first+"}";size_t at=0;
        while((at=text.find(token,at))!=std::string::npos){text.replace(at,token.size(),item.second);at+=item.second.size();}
    }
    return text;
}
inline std::wstring utf16(const std::string& text) {
    const auto size=MultiByteToWideChar(CP_UTF8,0,text.data(),static_cast<int>(text.size()),nullptr,0);
    std::wstring output(size,L' ');MultiByteToWideChar(CP_UTF8,0,text.data(),static_cast<int>(text.size()),output.data(),size);return output;
}
inline std::wstring wtr(const char* id){return utf16(tr(id));}
inline std::string utf8(const std::wstring& text) {
    const auto size=WideCharToMultiByte(CP_UTF8,0,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);
    std::string output(size,' ');WideCharToMultiByte(CP_UTF8,0,text.data(),static_cast<int>(text.size()),output.data(),size,nullptr,nullptr);return output;
}
inline const wchar_t* font_family() {
    if(language=="ja_jp") return L"Yu Gothic UI";
    if(language=="ko_kr") return L"Malgun Gothic";
    if(language=="zh_cn") return L"Microsoft YaHei UI";
    if(language=="zh_tw") return L"Microsoft JhengHei UI";
    if(language=="ka_ge") return L"Sylfaen";
    return L"Segoe UI";
}
inline void refresh(const std::filesystem::path& config_root,bool force=false) {
    if(preview_override) return;
    const auto now=GetTickCount64();
    if(!force && now-last_probe<2000) return;
    last_probe=now;
    try {
        const auto path=current_game_log_path(config_root);
        const auto stamp=std::filesystem::last_write_time(path);
        if(!force && path==previous_log_path && stamp==log_stamp) return;
        const auto code=detect(read_text(path));previous_log_path=path;log_stamp=stamp;
        if(code!=language || force) load(code);
    } catch(...) {if(strings.is_null()) load("en_gb");}
}
inline void initialize(const std::filesystem::path& assets,const std::filesystem::path& config_root) {
    resource_directory=assets/L"i18n";
    english={{"settings_title","Configure key"},{"current_key","Current key: {key}"},{"instruction","Click the field and press a key. Default: F8."},{"restore","Restore F8"},{"conflict","Warning: this key is already in use and may trigger another action. You can save it anyway."},{"invalid","Choose a key. Windows and clipboard recovery shortcuts are reserved."},{"unknown","This key could not be checked against the profile controls. Other apps may use it."},{"clear","No binding found in the controls checked. Other apps are not checked."},{"save_error","Could not save. Your previous key was kept. Try again."},{"open_error","Could not open clipboard storage."},{"document_number","DOCUMENT NUMBER"},{"title","Bill of lading"},{"sender","SHIPPER / ORIGIN"},{"recipient","CONSIGNEE / DESTINATION"},{"cargo","DESCRIPTION OF GOODS"},{"quantity","QTY."},{"weight","WEIGHT (kg)"},{"collection","PICKUP"},{"deadline","DELIVERY DEADLINE"},{"distance","PLANNED DISTANCE"},{"available","TIME ALLOWED AT PICKUP"},{"dispatch","DISPATCH"},{"carrier","CARRIER"},{"date","Day {day}, {time}"},{"duration","{h}h {m}min"},{"decimal","."},{"thousands",","},{"save","Save"},{"cancel","Cancel"}};
    try{english=Json::parse(read_text(resource_directory/L"en_gb.json"));}catch(...){}
    try{known_names=Json::parse(read_text(resource_directory/L"known-game-names.json"));}catch(...){known_names=nullptr;}
    strings=english;language="en_gb";refresh(config_root,true);
}
inline std::string game_name(const char* kind,const std::string& id,const std::string& saved) {
    const auto field=std::string("_")+kind;
    if(id.empty() || saved.empty() || !strings.contains(field) || !strings[field].contains(id) || !known_names.contains(kind) || !known_names[kind].contains(id)) return saved;
    const auto& names=known_names[kind][id];
    if(!names.is_array() || std::find(names.begin(),names.end(),saved)==names.end()) return saved;
    return strings[field][id].get<std::string>();
}
}
