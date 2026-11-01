#include "production.cpp"
#include <cassert>
#include <iostream>
int main() {
    root = std::filesystem::current_path() / L"plugin-check-data";
    assert(!std::filesystem::exists(root));
    std::filesystem::create_directories(root / L"documents");
    model = {}; changed = true; failed = false; lookup_done = false; document_id.clear();
    scs_named_value_t fields[7] = {};
    const char* names[] = {"cargo.id", "job.market", "cargo.loaded", "delivery.time", "source.city.id", "destination.city.id"};
    const char* values[] = {"mob_crusher", "quick_job", "true", "3364", "fresno", "las_vegas"};
    for (int i = 0; i < 6; ++i) { fields[i].name = names[i]; fields[i].value.type = SCS_VALUE_TYPE_string; fields[i].value.value_string.value = values[i]; }
    scs_telemetry_configuration_t cfg = {}; cfg.id = "job"; cfg.attributes = fields;
    on_event(SCS_TELEMETRY_EVENT_configuration, &cfg, nullptr);
    on_event(SCS_TELEMETRY_EVENT_started, nullptr, nullptr);
    std::promise<std::string> profile_fixture;
    profile_task = profile_fixture.get_future(); profile_fixture.set_value("steam-4142"); profile_dirty = false;
    on_event(SCS_TELEMETRY_EVENT_frame_end, nullptr, nullptr);
    assert(!model.emitted); // No clock yet.
    scs_value_t clock = {}; clock.type = SCS_VALUE_TYPE_u32; clock.value_u32.value = 2522;
    clock_callback("game.time", SCS_U32_NIL, &clock, nullptr);
    std::promise<SaveBaseline> baseline_fixture;
    baseline_task = baseline_fixture.get_future(); baseline_fixture.set_value(SaveBaseline{});
    on_event(SCS_TELEMETRY_EVENT_frame_end, nullptr, nullptr);
    assert(!failed && model.emitted);
    auto id = digest(model.key);
    auto json = profile_root / L"documents" / (id + ".json");
    auto original = read_text(json);
    const auto sound_event=read_text(root/L"emission-event.json");
    assert(original.find("\"collection_game_minutes\": null") != std::string::npos);
    const auto stamp = std::filesystem::last_write_time(json);
    model.connected_known = true; model.connected = false;
    on_event(SCS_TELEMETRY_EVENT_frame_end, nullptr, nullptr);
    model.connected = true;
    on_event(SCS_TELEMETRY_EVENT_frame_end, nullptr, nullptr);
    assert(read_text(json) == original && std::filesystem::last_write_time(json) == stamp);
    model = {}; lookup_done = false; reception_decided=false; document_id.clear();first_ready_minutes.reset();
    on_event(SCS_TELEMETRY_EVENT_configuration, &cfg, nullptr);
    profile_dirty = false;
    // Wait for an eligible frame to distinguish a loaded trip from a repeated acceptance.
    on_event(SCS_TELEMETRY_EVENT_frame_end, nullptr, nullptr);
    assert(!model.emitted && !failed);
    model.running=true;model.clock_known=true;model.minutes=2522;
    std::promise<SaveBaseline> loaded_fixture;baseline_task=loaded_fixture.get_future();loaded_fixture.set_value(SaveBaseline{});
    on_event(SCS_TELEMETRY_EVENT_frame_end,nullptr,nullptr);
    assert(model.emitted && !failed);
    assert(read_text(json) == original && std::filesystem::last_write_time(json) == stamp);
    assert(read_text(root/L"emission-event.json")==sound_event);
    // Same exact offer accepted after loading a pre-job save: replay presentation only.
    model={};reception_decided=false;lookup_done=false;document_id.clear();first_ready_minutes.reset();
    on_event(SCS_TELEMETRY_EVENT_configuration,&cfg,nullptr);profile_dirty=false;
    model.running=true;model.clock_known=true;model.minutes=2523;
    SaveBaseline prejob;prejob.empty=true;prejob.minutes=2500;
    std::promise<SaveBaseline> prejob_fixture;baseline_task=prejob_fixture.get_future();prejob_fixture.set_value(prejob);
    on_event(SCS_TELEMETRY_EVENT_frame_end,nullptr,nullptr);
    assert(model.emitted && !failed && read_text(json)==original);
    const auto repeated_event=read_text(root/L"emission-event.json");assert(repeated_event!=sound_event);
    // A repeated job configuration/profile refresh must not announce it twice.
    on_event(SCS_TELEMETRY_EVENT_configuration,&cfg,nullptr);
    std::promise<std::string> same_profile;profile_task=same_profile.get_future();same_profile.set_value("steam-4142");profile_dirty=false;
    on_event(SCS_TELEMETRY_EVENT_frame_end,nullptr,nullptr);
    assert(model.emitted && read_text(root/L"emission-event.json")==repeated_event);
    std::filesystem::remove(root/L"emission-event.json");
    for (const auto& entry : std::filesystem::directory_iterator(profile_root / L"documents")) std::filesystem::remove(entry.path());
    std::filesystem::remove(profile_root / L"documents"); std::filesystem::remove(profile_root); std::filesystem::remove(root / L"profiles");
    std::filesystem::remove(root / L"current.txt");
    std::filesystem::remove(root / L"current-render.json");
    std::filesystem::remove(root / L"documents"); std::filesystem::remove(root);
    std::cout << "PASS: callback pipeline waits for clock; emits once; reconnect does not rewrite; restart restores exact bytes\n";
}
