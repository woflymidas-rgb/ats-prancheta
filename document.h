#pragma once
#include <map>
#include <string>
#include <optional>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "city-catalog.h"

namespace clipboard {
using Fields = std::map<std::string, std::string>;
inline std::string field(const Fields& f, const char* name) {
    auto i = f.find(name); return i == f.end() ? "" : i->second;
}
inline std::string quote(const std::string& s) {
    std::ostringstream o; o << '"';
    for (unsigned char c : s) {
        if (c == '"' || c == '\\') o << '\\' << c;
        else if (c < 32) o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << unsigned(c) << std::dec;
        else o << c;
    }
    o << '"'; return o.str();
}
inline std::string identity(const Fields& f) {
    // Cargo-loaded and route length are deliberately excluded.
    std::string result;
    for (const auto* name : {"job.market", "cargo.id", "cargo.mass", "source.company.id", "destination.company.id", "source.city.id", "destination.city.id", "delivery.time", "is.special.job"}) {
        const auto s = field(f, name);
        result += std::to_string(s.size()) + ":" + s;
    }
    return result;
}
struct Model {
    Fields job;
    std::string key;
    bool running = false, clock_known = false, connected_known = false, connected = false;
    bool saw_empty_running = false, saw_unloaded = false, saw_disconnected = false;
    bool observed_collection = false, emitted = false, new_job_observed = false;
    unsigned minutes = 0;
    void configure(const Fields& next) {
        const auto next_key = field(next, "cargo.id").empty() ? "" : identity(next);
        if (next_key != key) {
            new_job_observed = !next_key.empty() && saw_empty_running;
            saw_empty_running = false;
            key = next_key; emitted = false; observed_collection = false;
            saw_unloaded = false; saw_disconnected = false;
        }
        job = next;
        if (!key.empty() && field(job, "cargo.loaded") == "false") saw_unloaded = true;
    }
    void frame() {
        if (!running) return;
        if (key.empty()) { saw_empty_running = true; return; }
        if (field(job, "job.market") == "cargo_market") {
            if (saw_unloaded && field(job, "cargo.loaded") == "true") observed_collection = true;
        } else if (field(job, "job.market") == "freight_market") {
            if (connected_known && !connected) saw_disconnected = true;
            if (new_job_observed && saw_disconnected && connected_known && connected) observed_collection = true;
        } else if (field(job, "job.market") == "quick_job") {
            if (new_job_observed) observed_collection = true;
        }
    }
    bool eligible() const {
        if (key.empty() || !running || !clock_known || emitted) return false;
        const auto market = field(job, "job.market");
        if (market == "quick_job") return field(job, "cargo.loaded") == "true";
        if (market == "cargo_market") return field(job, "cargo.loaded") == "true";
        if (market == "freight_market") return connected_known && connected;
        return false; // External contracts need their own deadline validation.
    }
};
struct Document {
    Fields job;
    std::optional<unsigned> collection_minutes;
    std::optional<long long> hours_available_minutes;
    std::string id;
    std::string source_state, destination_state;
    explicit Document(const Model& m, std::string number) : job(m.job), id(std::move(number)) {
        source_state = state_for_city(field(job, "source.city.id"));
        destination_state = state_for_city(field(job, "destination.city.id"));
        if (m.observed_collection && m.clock_known) {
            collection_minutes = m.minutes;
            const auto deadline = field(job, "delivery.time");
            if (!deadline.empty()) {
                try { hours_available_minutes = std::stoll(deadline) - m.minutes; } catch (...) {}
            }
        }
    }
    std::string json() const {
        std::ostringstream o;
        o << "{\n  \"schema\": 1,\n  \"document_id\": " << quote(id)
          << ",\n  \"collection_game_minutes\": " << (collection_minutes ? std::to_string(*collection_minutes) : "null")
          << ",\n  \"available_minutes_at_collection\": " << (hours_available_minutes ? std::to_string(*hours_available_minutes) : "null")
          << ",\n  \"source_state\": " << (source_state.empty() ? "null" : quote(source_state))
          << ",\n  \"destination_state\": " << (destination_state.empty() ? "null" : quote(destination_state)) << ",\n  \"job\": {";
        bool first = true;
        for (const auto& p : job) { if (!first) o << ','; first = false; o << '\n' << "    " << quote(p.first) << ": " << quote(p.second); }
        o << "\n  }\n}\n"; return o.str();
    }
    std::string text() const {
        auto display = [&](const char* name) { auto s = field(job, name); return s.empty() ? std::string("—") : s; };
        auto time = [](unsigned n) { std::ostringstream o; o << "Dia " << n / 1440 + 1 << ", " << std::setw(2) << std::setfill('0') << (n % 1440) / 60 << ':' << std::setw(2) << n % 60 << " (relógio do jogo)"; return o.str(); };
        std::ostringstream o;
        std::string weight = "—";
        try {
            const double mass = std::stod(field(job, "cargo.mass"));
            if (std::isfinite(mass) && mass >= 0 && mass < 1e9) {
                std::ostringstream n; n << std::fixed << std::setprecision(1) << mass;
                auto value = n.str(); const auto dot = value.find('.');
                auto whole = value.substr(0, dot);
                for (int i = static_cast<int>(whole.size()) - 3; i > 0; i -= 3) whole.insert(static_cast<size_t>(i), ".");
                weight = whole;
                if (dot != std::string::npos && value.substr(dot + 1) != "0") weight += "," + value.substr(dot + 1);
            }
        } catch (...) {}
        o << "DOCUMENTO DE TRANSPORTE\r\n" << "Nº " << id.substr(0, 12) << "\r\n\r\n"
          << "COLETA\r\n" << display("source.company") << "\r\n" << display("source.city") << " / " << (source_state.empty() ? "Estado: —" : source_state) << "\r\n\r\n"
          << "ENTREGA\r\n" << display("destination.company") << "\r\n" << display("destination.city") << " / " << (destination_state.empty() ? "Estado: —" : destination_state) << "\r\n\r\n"
          << "MERCADORIA\r\n" << display("cargo") << "\r\nPeso: " << weight << " kg\r\n"
          << "Distância planejada: " << display("planned_distance.km") << " km\r\n\r\n"
          << "Coleta: " << (collection_minutes ? time(*collection_minutes) : "—") << "\r\n";
        const auto deadline = field(job, "delivery.time");
        std::string end = "—";
        try { if (!deadline.empty()) end = time(static_cast<unsigned>(std::stoul(deadline))); } catch (...) {}
        o << "Prazo final: " << end << "\r\nHoras disponíveis na coleta: ";
        if (hours_available_minutes) o << *hours_available_minutes / 60 << "h " << *hours_available_minutes % 60 << "min";
        else o << "—";
        o << "\r\n";
        return o.str();
    }
};
}
