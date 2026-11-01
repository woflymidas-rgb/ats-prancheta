#include "document.h"
#include "clipboard-audio.h"
#include <cassert>
#include <iostream>
int main() {
 using clipboard_audio::Json;clipboard_audio::EmissionObserver observer;
 Json event={{"token","profile-a:job-a:1"},{"unix_seconds",100LL}};
 assert(observer.consume(event,100,true));assert(!observer.consume(event,100,true));
 event["token"]="profile-b:job-a:2";assert(observer.consume(event,102,true));
 event["token"]="old";assert(!observer.consume(event,110,true));
 event["token"]="unfocused";event["unix_seconds"]=110;assert(!observer.consume(event,110,false));assert(!observer.consume(event,110,true));
 event["token"]="future";assert(!observer.consume(event,109,true));
 assert(!observer.consume(Json::object(),110,true));
 std::cout<<"PASS: emission once; profile identity; stale, background, future and empty events silent\n";
}
