#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>
#pragma comment(lib, "bcrypt.lib")

inline std::filesystem::path data_root() {
    wchar_t location[32768];
    const auto n = GetEnvironmentVariableW(L"LOCALAPPDATA", location, 32768);
    if (!n || n >= 32768) throw std::runtime_error("LOCALAPPDATA unavailable");
    auto root = std::filesystem::path(location) / L"ATSPrancheta" / L"prototype-single-profile";
    std::filesystem::create_directories(root / L"documents");
    return root;
}
inline std::string digest(const std::string& value) {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) throw std::runtime_error("SHA256 unavailable");
    unsigned char result[32] = {};
    const auto status = BCryptHash(algorithm, nullptr, 0, reinterpret_cast<PUCHAR>(const_cast<char*>(value.data())), static_cast<ULONG>(value.size()), result, 32);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (status < 0) throw std::runtime_error("SHA256 failed");
    std::ostringstream o;
    for (auto c : result) o << std::hex << std::setw(2) << std::setfill('0') << unsigned(c);
    return o.str();
}
inline std::string read_text(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Read failed");
    return std::string(std::istreambuf_iterator<char>(f), {});
}
inline void atomic_text(const std::filesystem::path& path, const std::string& text, bool replace) {
    const auto temporary = path.wstring() + L".tmp." + std::to_wstring(GetCurrentProcessId());
    { std::ofstream f(temporary, std::ios::binary | std::ios::trunc); f.write(text.data(), static_cast<std::streamsize>(text.size())); f.flush(); if (!f) throw std::runtime_error("Write failed"); }
    if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH | (replace ? MOVEFILE_REPLACE_EXISTING : 0))) {
        DeleteFileW(temporary.c_str()); throw std::runtime_error("Commit failed");
    }
}
