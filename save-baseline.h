#pragma once
#include "document.h"
#include "storage.h"
#include <shlobj.h>
#include <regex>
#include <algorithm>
#include "sii-reader-source/SII-Decrypt-cpp-main/src/core/sii_decryptor.h"
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

struct SaveBaseline {
    bool empty = false;
    std::optional<unsigned> minutes;
    std::string source;
};
inline SaveBaseline inspect_decoded_baseline(const std::string& text) {
    SaveBaseline result;
    // Decoder emits canonical unit headers. Avoid recursive regex over multi-MB saves.
    auto property = [&](const char* unit, const char* name) -> std::string {
        const std::string marker = std::string("\n") + unit + " : ";
        const auto start = text.find(marker);
        if (start == std::string::npos || text.find(marker, start + marker.size()) != std::string::npos) return "";
        const auto end = text.find("\n}", start);
        if (end == std::string::npos) return "";
        std::istringstream lines(text.substr(start, end - start));
        std::string line;
        while (std::getline(lines, line)) {
            std::istringstream words(line); std::string key, value, extra;
            if (words >> key >> value && key == std::string(name) + ":" && !(words >> extra)) return value;
        }
        return "";
    };
    if (property("player", "current_job") != "null") return result;
    const auto minutes = property("economy", "game_time");
    if (minutes.empty() || minutes.find_first_not_of("0123456789") != std::string::npos) return result;
    try { result.minutes = static_cast<unsigned>(std::stoul(minutes)); result.empty = true; } catch (...) {}
    return result;
}
inline SaveBaseline read_loaded_save_baseline(const std::filesystem::path& config_root) {
    SaveBaseline unknown;
    try {
        wchar_t custom[32768] = {};
        GetPrivateProfileStringW(L"Paths", L"GameHome", L"", custom, 32768, (config_root / L"settings.ini").c_str());
        std::filesystem::path home(custom);
        if (home.empty()) {
            PWSTR docs = nullptr;
            if (FAILED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs))) return unknown;
            home = std::filesystem::path(docs) / L"American Truck Simulator";
            CoTaskMemFree(docs);
        }
        const auto log_path = home / L"game.log.txt";
        const auto log_text = read_text(log_path);
        const auto position = log_text.rfind("Loading save.");
        if (position == std::string::npos) return unknown;
        const auto load_line = log_text.substr(position, log_text.find('\n', position) - position);
        std::smatch match;
        if (!std::regex_search(load_line, match, std::regex("/steam/profiles/([0-9A-Fa-f]+)/save/([A-Za-z0-9_]+)/game\\.sii"))) return unknown;
        const std::string profile = match[1], slot = match[2];
        std::vector<std::filesystem::path> steam_roots;
        wchar_t pf[32768] = {};
        if (GetEnvironmentVariableW(L"ProgramFiles(x86)", pf, 32768)) steam_roots.push_back(std::filesystem::path(pf) / L"Steam");
        GetPrivateProfileStringW(L"Paths", L"SteamRoot", L"", custom, 32768, (config_root / L"settings.ini").c_str());
        if (custom[0]) steam_roots.insert(steam_roots.begin(), std::filesystem::path(custom));
        std::vector<std::filesystem::path> candidates;
        for (const auto& steam : steam_roots) {
            if (!std::filesystem::exists(steam / L"userdata")) continue;
            for (const auto& account : std::filesystem::directory_iterator(steam / L"userdata")) {
                if (!account.is_directory()) continue;
                const auto path = account.path() / L"270880" / L"remote" / L"profiles" / profile / L"save" / slot / L"game.sii";
                if (std::filesystem::exists(path) && std::find(candidates.begin(), candidates.end(), path) == candidates.end()) candidates.push_back(path);
            }
        }
        if (candidates.size() != 1) return unknown;
        // Read into RAM; decoder receives memory and never gets an original save path to write.
        const auto before = std::filesystem::last_write_time(candidates[0]);
        const auto bytes = read_text(candidates[0]);
        if (bytes.empty() || bytes.size() > 32 * 1024 * 1024) return unknown;
        SIIDecryptor decoder;
        decoder.ReraiseExceptions = false;
        std::vector<uint8_t> decoded;
        size_t size = 0;
        const auto status = decoder.DecryptAndDecodeMemory(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), nullptr, &size, &decoded);
        if (status != rSuccess || decoded.empty()) return unknown;
        if (before != std::filesystem::last_write_time(candidates[0])) return unknown;
        const auto again = read_text(log_path);
        const auto last = again.rfind("Loading save.");
        if (last == std::string::npos || again.substr(last, again.find('\n', last) - last) != load_line) return unknown;
        auto result = inspect_decoded_baseline(std::string(decoded.begin(), decoded.end()));
        result.source = profile + "/" + slot;
        return result;
    } catch (...) { return unknown; }
}
