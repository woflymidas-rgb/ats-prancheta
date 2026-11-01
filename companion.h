#pragma once
#include <windows.h>
#pragma comment(lib,"user32.lib")
#include <filesystem>
#include <string>
namespace companion {
constexpr UINT attach_message=WM_APP+58;
inline unsigned long long birth(HANDLE process) {
    FILETIME created,ended,kernel,user;
    if(!GetProcessTimes(process,&created,&ended,&kernel,&user))return 0;
    return (static_cast<unsigned long long>(created.dwHighDateTime)<<32)|created.dwLowDateTime;
}
inline HANDLE game=nullptr;
inline bool attach(DWORD pid,unsigned long long expected) {
    HANDLE process=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
    if(!process)return false;
    wchar_t path[32768];DWORD size=32768;
    const bool valid=expected && birth(process)==expected && QueryFullProcessImageNameW(process,0,path,&size) && _wcsicmp(std::filesystem::path(path).filename().c_str(),L"amtrucks.exe")==0;
    if(!valid){CloseHandle(process);return false;}
    if(game)CloseHandle(game);game=process;return true;
}
inline bool finished(){return game && WaitForSingleObject(game,0)==WAIT_OBJECT_0;}
inline void close(){if(game){CloseHandle(game);game=nullptr;}}
inline bool launch(const std::filesystem::path& plugin_directory) {
    wchar_t executable[32768]={};
    GetPrivateProfileStringW(L"Companion",L"Executable",L"",executable,32768,(plugin_directory/L"ATSPrancheta-launch.ini").c_str());
    if(!executable[0] || !std::filesystem::is_regular_file(executable))return false;
    const auto pid=GetCurrentProcessId();const auto created=birth(GetCurrentProcess());
    if(const auto existing=FindWindowW(L"ATSPranchetaViewer",nullptr)) {
        DWORD_PTR result=0;
        if(SendMessageTimeoutW(existing,attach_message,pid,static_cast<LPARAM>(created),SMTO_ABORTIFHUNG,1000,&result) && result)return true;
    }
    std::wstring command=L"\""+std::wstring(executable)+L"\" --game-pid="+std::to_wstring(pid)+L" --game-born="+std::to_wstring(created);
    STARTUPINFOW startup={};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;
    PROCESS_INFORMATION process={};const auto working=std::filesystem::path(executable).parent_path();
    if(!CreateProcessW(executable,command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,working.c_str(),&startup,&process))return false;
    CloseHandle(process.hThread);CloseHandle(process.hProcess);return true;
}
}
