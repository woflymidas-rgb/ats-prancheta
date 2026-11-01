#include "document.h"
#include "storage.h"
#include "save-baseline.h"
#include "profile.h"
#include "branding.h"
#include "clipboard-audio.h"
#include "companion.h"
#include <future>
#include "scssdk_telemetry.h"
#include "amtrucks/scssdk_ats.h"
#include <cstdio>
#include <cstring>

static clipboard::Model model;
static scs_log_t log_message = nullptr;
static std::filesystem::path root;
static bool changed = true, failed = false;
static bool lookup_done = false;
static bool reception_decided = false;
static std::string document_id;
static std::future<SaveBaseline> baseline_task;
static std::optional<unsigned> first_ready_minutes;
static std::future<std::string> profile_task;
static bool profile_dirty = true;
static std::filesystem::path profile_root;
static std::filesystem::path asset_root;
extern "C" IMAGE_DOS_HEADER __ImageBase;
static void clear_current() {
    atomic_text(root / L"current.txt", "", true);
    atomic_text(root / L"current-render.json", "null", true);
}
static void log(const char* message) { if (log_message) log_message(SCS_LOG_TYPE_message, message); }

static SCSAPI_VOID clock_callback(scs_string_t, scs_u32_t, const scs_value_t* v, scs_context_t) {
    model.clock_known = v && v->type == SCS_VALUE_TYPE_u32;
    if (model.clock_known) model.minutes = v->value_u32.value;
}
static SCSAPI_VOID connection_callback(scs_string_t, scs_u32_t, const scs_value_t* v, scs_context_t) {
    model.connected_known = v && v->type == SCS_VALUE_TYPE_bool;
    model.connected = model.connected_known && v->value_bool.value;
}
static SCSAPI_VOID on_event(scs_event_t kind, const void* data, scs_context_t) {
    try {
        if (kind == SCS_TELEMETRY_EVENT_frame_start && data) {
            const auto* f = static_cast<const scs_telemetry_frame_start_t*>(data);
            if (f->flags & SCS_TELEMETRY_FRAME_START_FLAG_timer_restart) {
                profile_dirty = true;
                reception_decided = false;first_ready_minutes.reset();baseline_task = {};
                model.saw_empty_running = false;
                model.new_job_observed = false;
                model.saw_unloaded = false;
                model.saw_disconnected = false;
                model.observed_collection = false;
                model.clock_known = false;
            }
        }
        if (kind == SCS_TELEMETRY_EVENT_started) model.running = true;
        if (kind == SCS_TELEMETRY_EVENT_paused) model.running = false;
        if (kind == SCS_TELEMETRY_EVENT_configuration) {
            const auto* c = static_cast<const scs_telemetry_configuration_t*>(data);
            if (!c || strcmp(c->id, "job") != 0) return;
            profile_dirty = true;
            clipboard::Fields fields;
            if (c->attributes) for (auto p = c->attributes; p->name; ++p) {
                std::string value;
                switch (p->value.type) {
                case SCS_VALUE_TYPE_string: if (p->value.value_string.value) value = p->value.value_string.value; break;
                case SCS_VALUE_TYPE_bool: value = p->value.value_bool.value ? "true" : "false"; break;
                case SCS_VALUE_TYPE_u32: value = std::to_string(p->value.value_u32.value); break;
                case SCS_VALUE_TYPE_u64: value = std::to_string(p->value.value_u64.value); break;
                case SCS_VALUE_TYPE_float: { char n[80]; snprintf(n, sizeof(n), "%.9g", p->value.value_float.value); value = n; break; }
                default: continue;
                }
                fields[p->name] = value;
            }
            auto old = model.key;
            model.configure(fields);
            if (old != model.key) { reception_decided = false; changed = true; lookup_done = false; document_id.clear(); first_ready_minutes.reset(); baseline_task = {}; }
        }
        if (kind == SCS_TELEMETRY_EVENT_gameplay && data) {
            const auto* e = static_cast<const scs_telemetry_gameplay_event_t*>(data);
            if (!strcmp(e->id, "job.cancelled") || !strcmp(e->id, "job.delivered")) {
                model.configure({}); changed = true; lookup_done = false; document_id.clear();
            }
        }
        if (kind != SCS_TELEMETRY_EVENT_frame_end || failed) return;
        model.frame();
        if (model.eligible() && !first_ready_minutes) first_ready_minutes = model.minutes;
        if (profile_dirty && !profile_task.valid()) {
            profile_dirty = false;
            const auto config_root = root;
            profile_task = std::async(std::launch::async, [config_root] { return read_profile(config_root); });
            clear_current();
            return;
        }
        if (profile_task.valid()) {
            if (profile_task.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
            const auto profile = profile_task.get();
            if (profile.empty()) {
                profile_root.clear();
                model.observed_collection = false; model.new_job_observed = false;
                model.saw_empty_running = false; model.saw_unloaded = false; model.saw_disconnected = false;
                first_ready_minutes.reset(); baseline_task = {};
                clear_current();
                log("[ATSPrancheta] Perfil desconhecido; nenhum documento compartilhado sera usado.");
                return;
            }
            const auto next_root = root / L"profiles" / profile;
            if (next_root != profile_root) {
                reception_decided = false;
                if (!profile_root.empty()) {
                    model.observed_collection = false; model.new_job_observed = false;
                    model.saw_empty_running = false; model.saw_unloaded = false; model.saw_disconnected = false;
                    first_ready_minutes.reset(); baseline_task = {};
                }
                profile_root = next_root;
                model.emitted = false; lookup_done = false;
                log(("[ATSPrancheta] Armazenamento selecionado: " + profile).c_str());
            }
            model.emitted = false; lookup_done = false; changed = true;
            std::filesystem::create_directories(profile_root / L"documents");
            if (profile_dirty) return; // A configuration changed while the worker was reading.
        }
        if (profile_root.empty() || profile_dirty) return;
        if (model.key.empty()) {
            if (changed) { clear_current(); changed = false; }
            return;
        }
        if (model.emitted) return;
        if (document_id.empty()) document_id = digest(model.key);
        const auto& id = document_id;
        const auto path = profile_root / L"documents" / (id + ".json");
        const auto text_path = profile_root / L"documents" / (id + ".txt");
        const bool existing=std::filesystem::exists(path) && std::filesystem::exists(text_path);
        if(reception_decided && existing) {
            atomic_text(root/L"current.txt",read_text(text_path),true);
            publish_document(root,path,asset_root);
            model.emitted=true;
            return;
        }
        if (!model.eligible()) return;
        if (!first_ready_minutes) first_ready_minutes = model.minutes;
        auto emission = model;
        if (clipboard::field(model.job, "job.market") == "quick_job" && !model.observed_collection) {
            if (!baseline_task.valid()) {
                const auto config_root = root;
                baseline_task = std::async(std::launch::async, [config_root] { return read_loaded_save_baseline(config_root); });
                return;
            }
            if (baseline_task.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
            const auto baseline = baseline_task.get();
            if (baseline.empty && baseline.minutes && *baseline.minutes <= *first_ready_minutes) {
                emission.observed_collection = true;
                emission.minutes = *first_ready_minutes;
                log("[ATSPrancheta] Nova contratacao confirmada: save carregado sem servico; horario do primeiro quadro registrado.");
            } else log("[ATSPrancheta] Origem da coleta nao confirmada; campos historicos permanecem null.");
        }
        if(existing) {
            atomic_text(root/L"current.txt",read_text(text_path),true);
            publish_document(root,path,asset_root);
            if(emission.observed_collection || emission.new_job_observed) {
                clipboard_audio::publish(root,profile_root.u8string()+":"+id);
                log("[ATSPrancheta] Mesmo servico aceito novamente; ficha reapresentada sem alterar arquivo.");
            } else log("[ATSPrancheta] Documento existente recuperado; conteudo preservado.");
            reception_decided=true;model.emitted=true;
            return;
        }
        lookup_done=true;
        clipboard::Document doc(emission, id);
        // JSON is the authoritative immutable record; recover a missing text projection safely.
        if (std::filesystem::exists(path)) throw std::runtime_error("Incomplete document: JSON exists without text; preserve for recovery");
        atomic_text(text_path, doc.text(), false);
        atomic_text(path, doc.json(), false);
        atomic_text(root / L"current.txt", doc.text(), true);
        publish_document(root, path, asset_root);
        clipboard_audio::publish(root,profile_root.u8string()+":"+id);
        reception_decided=true;model.emitted = true;
        log("[ATSPrancheta] Documento emitido. Historico desconhecido permanece null.");
    } catch (const std::exception& e) {
        failed = true;
        log("[ATSPrancheta] Falha ao processar documento; gravação suspensa nesta sessao.");
        log(e.what());
    } catch (...) { failed = true; log("[ATSPrancheta] Falha inesperada; processamento suspenso."); }
}
SCSAPI_RESULT scs_telemetry_init(scs_u32_t version, const scs_telemetry_init_params_t* params) {
    if (version != SCS_TELEMETRY_VERSION_1_01 || !params) return SCS_RESULT_unsupported;
    const auto* p = static_cast<const scs_telemetry_init_params_v101_t*>(params);
    if (!p->common.game_id || strcmp(p->common.game_id, SCS_GAME_ID_ATS)) return SCS_RESULT_unsupported;
    log_message = p->common.log;
    model = {}; failed = false; changed = true; lookup_done = false; reception_decided=false; document_id.clear(); first_ready_minutes.reset(); baseline_task = {};
    profile_task = {}; profile_dirty = true; profile_root.clear();
    try {
        root = data_root(); clear_current();
        wchar_t module[32768];
        GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), module, 32768);
        asset_root = std::filesystem::path(module).parent_path() / L"ATSPrancheta-assets";
    }
    catch (...) { log("[ATSPrancheta] Falha ao abrir armazenamento."); return SCS_RESULT_generic_error; }
    for (auto kind : {SCS_TELEMETRY_EVENT_configuration, SCS_TELEMETRY_EVENT_gameplay, SCS_TELEMETRY_EVENT_started, SCS_TELEMETRY_EVENT_paused, SCS_TELEMETRY_EVENT_frame_start, SCS_TELEMETRY_EVENT_frame_end})
        if (p->register_for_event(kind, on_event, nullptr) != SCS_RESULT_ok) return SCS_RESULT_generic_error;
    if (p->register_for_channel("game.time", SCS_U32_NIL, SCS_VALUE_TYPE_u32, SCS_TELEMETRY_CHANNEL_FLAG_no_value, clock_callback, nullptr) != SCS_RESULT_ok) return SCS_RESULT_generic_error;
    if (p->register_for_channel("trailer.connected", SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_no_value, connection_callback, nullptr) != SCS_RESULT_ok) return SCS_RESULT_generic_error;
    try {if(!companion::launch(asset_root.parent_path())) log("[ATSPrancheta] Visor automatico indisponivel; verifique a instalacao.");}catch(...) {log("[ATSPrancheta] Falha ao iniciar visor; documentos preservados.");}
    log("[ATSPrancheta] Prototipo 0.7 iniciado. Documento estruturado e marca congelada; folha vazia preservada.");
    return SCS_RESULT_ok;
}
SCSAPI_VOID scs_telemetry_shutdown() { baseline_task = {}; profile_task = {}; log("[ATSPrancheta] Encerrado."); log_message = nullptr; model = {}; }

