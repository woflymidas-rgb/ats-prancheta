#pragma once
#include "profile.h"
#include "vendor/json.hpp"
using Json = nlohmann::json;
inline std::string job_value(const Json& doc, const char* key) {
    if (!doc.contains("job") || !doc["job"].is_object()) return "";
    const auto& job = doc["job"];
    if (!job.contains(key) || !job[key].is_string()) return "";
    return job[key].get<std::string>();
}
inline std::filesystem::path select_logo(const Json& doc, const Json& catalog, const std::filesystem::path& assets, const std::string& log_text) {
    const auto id = job_value(doc, "source.company.id"), name = job_value(doc, "source.company");
    if (id.empty() || name.empty() || !catalog.contains("base")) return {};
    const Json* entry = nullptr;
    std::string folder = "logos-base";
    if (catalog["base"].contains(id)) entry = &catalog["base"][id];
    std::string active;
    const std::regex group("\\[mods\\] Active [0-9]+ mods");
    for (auto i = std::sregex_iterator(log_text.begin(), log_text.end(), group); i != std::sregex_iterator(); ++i)
        active = log_text.substr(static_cast<size_t>(i->position()));
    if (catalog.contains("overrides")) for (const auto& overlay : catalog["overrides"]) {
        const auto workshop = overlay.value("workshop_id", "");
        if (!workshop.empty() && active.find("[mods] Active workshop mod ID " + workshop + " ") != std::string::npos && overlay["companies"].contains(id)) {
            entry = &overlay["companies"][id]; folder = "logos-" + workshop;
        }
    }
    if (!entry || entry->value("name", "") != name) return {};
    const auto texture = entry->value("texture", "");
    if (texture.empty() || texture.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_") != std::string::npos) return {};
    const auto path = assets / folder / (texture + ".png");
    return std::filesystem::exists(path) ? path : std::filesystem::path{};
}
inline std::filesystem::path current_game_log_path(const std::filesystem::path& config_root) {
    wchar_t custom[32768] = {};
    GetPrivateProfileStringW(L"Paths", L"GameHome", L"", custom, 32768, (config_root / L"settings.ini").c_str());
    std::filesystem::path home(custom);
    if (home.empty()) {
        PWSTR docs = nullptr;
        if (FAILED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs))) return "";
        home = std::filesystem::path(docs) / L"American Truck Simulator"; CoTaskMemFree(docs);
    }
    return home / L"game.log.txt";
}
inline std::string current_game_log(const std::filesystem::path& config_root) {
    try { return read_text(current_game_log_path(config_root)); } catch (...) { return ""; }
}
inline void publish_document(const std::filesystem::path& config_root, const std::filesystem::path& json_path, const std::filesystem::path& asset_root) {
    const auto doc = Json::parse(read_text(json_path));
    auto logo_path = json_path; logo_path.replace_extension("logo.png");
    if (!std::filesystem::exists(logo_path)) {
        try {
            const auto catalog = Json::parse(read_text(asset_root / L"company-catalog.json"));
            const auto source = select_logo(doc, catalog, asset_root, current_game_log(config_root));
            if (!source.empty()) std::filesystem::copy_file(source, logo_path); // Never overwrite a frozen logo.
        } catch (...) { /* A missing logo must not prevent consulting the document. */ }
    }
    Json envelope = {{"document", doc}, {"logo_path", std::filesystem::exists(logo_path) ? logo_path.u8string() : ""}};
    atomic_text(config_root / L"current-render.json", envelope.dump(), true);
}
