#pragma once
#include "save-baseline.h"
inline std::string profile_from_log(const std::string& text) {
    const auto position = text.rfind("Loading save.");
    if (position == std::string::npos) return "";
    // Selecting another profile invalidates the previous loaded-save association.
    const auto selected = text.rfind("New profile selected:");
    const auto finished = text.rfind("Set profile finished:");
    if ((selected != std::string::npos && selected > position) || (finished != std::string::npos && finished > position)) return "";
    const auto line = text.substr(position, text.find('\n', position) - position);
    std::smatch match;
    if (!std::regex_search(line, match, std::regex("/(steam/profiles|home/profiles)/([0-9A-Fa-f]{2,256})/save/[A-Za-z0-9_]+/game\\.sii"))) return "";
    auto id = match[2].str();
    if (id.size() % 2) return "";
    std::transform(id.begin(), id.end(), id.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return (match[1] == "steam/profiles" ? "steam-" : "local-") + id;
}
inline std::string read_profile(const std::filesystem::path& config_root) {
    try {
        wchar_t custom[32768] = {};
        GetPrivateProfileStringW(L"Paths", L"GameHome", L"", custom, 32768, (config_root / L"settings.ini").c_str());
        std::filesystem::path home(custom);
        if (home.empty()) {
            PWSTR docs = nullptr;
            if (FAILED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs))) return "";
            home = std::filesystem::path(docs) / L"American Truck Simulator"; CoTaskMemFree(docs);
        }
        return profile_from_log(read_text(home / L"game.log.txt"));
    } catch (...) { return ""; }
}
