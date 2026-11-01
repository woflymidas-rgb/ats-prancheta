#pragma once
#include "storage.h"
#include "vendor/json.hpp"
#include <ctime>
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")
namespace clipboard_audio {
using Json=nlohmann::json;
struct EmissionObserver {
    std::string last;
    void prime(const std::filesystem::path& path) {
        try {last=Json::parse(read_text(path)).value("token","");}catch(...){}
    }
    bool consume(const Json& event,long long now,bool audible) {
        const auto token=event.value("token","");
        if(token.empty() || token==last) return false;
        last=token;
        const auto at=event.value("unix_seconds",0LL);
        return audible && at<=now && now-at<=5;
    }
};
inline void publish(const std::filesystem::path& root,const std::string& identity) noexcept {
    try {
        const auto now=static_cast<long long>(std::time(nullptr));
        const auto token=identity+":"+std::to_string(now)+":"+std::to_string(GetTickCount64());
        atomic_text(root/L"emission-event.json",Json{{"token",token},{"unix_seconds",now}}.dump(),true);
    }catch(...){} // Sound failure must never invalidate a committed document.
}
inline void stop() {
    mciSendStringW(L"close ats_page",nullptr,0,nullptr);
    mciSendStringW(L"close ats_print",nullptr,0,nullptr);
}
inline void play(const std::filesystem::path& assets,bool printer) {
    const auto alias=printer?L"ats_print":L"ats_page";
    const auto path=assets/L"sounds"/(printer?L"document-print.mp3":L"page-flip.mp3");
    const auto close=std::wstring(L"close ")+alias;
    mciSendStringW(close.c_str(),nullptr,0,nullptr);
    const auto open=std::wstring(L"open \"")+path.wstring()+L"\" type mpegvideo alias "+alias;
    if(mciSendStringW(open.c_str(),nullptr,0,nullptr)!=0) return;
    const auto play=std::wstring(L"play ")+alias+L" from 0";
    mciSendStringW(play.c_str(),nullptr,0,nullptr);
}
}