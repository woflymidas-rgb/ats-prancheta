#include "companion.h"
#include <cassert>
#include <iostream>
int wmain(int argc,wchar_t** argv) {
    if(argc==3 && std::wstring(argv[1])==L"--fixture-child") {
        HANDLE event=OpenEventW(SYNCHRONIZE,FALSE,argv[2]);if(!event)return 1;
        const auto result=WaitForSingleObject(event,10000);CloseHandle(event);return result==WAIT_OBJECT_0?0:1;
    }
    const auto exe=std::filesystem::current_path()/L"build"/L"lifecycle-fixture"/L"amtrucks.exe";
    const auto event_name=L"Local\\ATSPranchetaLifecycleTest-"+std::to_wstring(GetCurrentProcessId());
    HANDLE event=CreateEventW(nullptr,TRUE,FALSE,event_name.c_str());assert(event);
    std::wstring command=L"\""+exe.wstring()+L"\" --fixture-child "+event_name;
    STARTUPINFOW startup={};startup.cb=sizeof(startup);PROCESS_INFORMATION child={};
    assert(CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&child));
    const auto born=companion::birth(child.hProcess);assert(born);
    assert(!companion::attach(child.dwProcessId,born+1));
    assert(companion::attach(child.dwProcessId,born));assert(!companion::finished());
    assert(!companion::attach(child.dwProcessId,born+1));assert(!companion::finished());
    SetEvent(event);assert(WaitForSingleObject(child.hProcess,5000)==WAIT_OBJECT_0);
    assert(companion::finished());companion::close();assert(!companion::finished());
    CloseHandle(child.hThread);CloseHandle(child.hProcess);CloseHandle(event);
    assert(!companion::attach(GetCurrentProcessId(),companion::birth(GetCurrentProcess())));
    std::cout<<"PASS: process name and creation time verified; wrong PID instance rejected; exit detected; handles released; no game launched\n";
}
