#include "document.h"
#include "storage.h"
#include "paper-content.h"
#include <cassert>
#include <iostream>

static clipboard::Fields job(const char* market) {
    return {{"job.market", market}, {"cargo.id", "crusher"}, {"cargo.mass", "40143"}, {"source.city.id", "fresno"}, {"destination.city.id", "las_vegas"}, {"delivery.time", "3364"}, {"planned_distance.km", "567"}, {"cargo.loaded", "true"}};
}
int main() {
    assert(paper_content("").empty());
    assert(paper_content("Nenhum serviço de carga ativo.\r\n").empty());
    assert(paper_content("Aguardando os dados do serviço no ATS…\r\n").empty());
    assert(paper_content("Identificando o perfil do serviço…\r\n").empty());
    const std::string legacy = "DOCUMENTO DE TRANSPORTE\r\nCarga: exemplo\r\n\r\nDocumento fixo. — = informação não registrada.\r\n";
    assert(paper_content(legacy) == "DOCUMENTO DE TRANSPORTE\r\nCarga: exemplo");
    clipboard::Model restored;
    restored.configure({}); restored.configure(job("quick_job"));
    restored.running = true; restored.clock_known = true; restored.minutes = 2522;
    restored.frame(); assert(restored.eligible());
    clipboard::Document late(restored, "test");
    assert(!late.collection_minutes); assert(!late.hours_available_minutes);
    assert(late.json().find("\"collection_game_minutes\": null") != std::string::npos);
    assert(late.source_state == "CA" && late.destination_state == "NV");
    assert(late.json().find("\"source_state\": \"CA\"") != std::string::npos);
    assert(clipboard::state_for_city("city_that_does_not_exist").empty());
    assert(clipboard::state_for_city("seattle") == "WA");
    assert(clipboard::state_for_city("houston") == "TX");
    clipboard::Model unknown_city; auto unknown_job = job("quick_job"); unknown_job["source.city.id"] = "mod_unknown";
    unknown_city.configure(unknown_job); clipboard::Document unknown_doc(unknown_city, "unknown");
    assert(unknown_doc.json().find("\"source_state\": null") != std::string::npos);
    auto before = late.json();
    restored.minutes += 20; restored.job["planned_distance.km"] = "400";
    assert(late.json() == before);
    auto signature = restored.key;
    auto changed = job("quick_job"); changed["planned_distance.km"] = "400";
    assert(clipboard::identity(changed) == signature);
    changed["delivery.time"] = "4000"; assert(clipboard::identity(changed) != signature);

    clipboard::Model cargo;
    auto c = job("cargo_market"); c["cargo.loaded"] = "false";
    cargo.configure(c); cargo.running = true; cargo.clock_known = true; cargo.minutes = 2516;
    cargo.frame(); assert(!cargo.eligible());
    c["cargo.loaded"] = "true"; cargo.configure(c); cargo.frame(); assert(cargo.eligible());
    clipboard::Document picked(cargo, "picked"); assert(picked.collection_minutes == 2516u); assert(picked.hours_available_minutes == 848LL);
    cargo.emitted = true;
    cargo.connected_known = true; cargo.connected = false; cargo.frame();
    cargo.connected = true; cargo.frame(); assert(!cargo.eligible());

    clipboard::Model freight;
    freight.running = true; freight.clock_known = true; freight.frame();
    freight.configure(job("freight_market")); freight.connected_known = true; freight.connected = false;
    freight.frame(); assert(!freight.eligible());
    freight.connected = true; freight.frame(); assert(freight.observed_collection); assert(freight.eligible());
    freight.emitted = true; freight.connected = false; freight.frame(); freight.connected = true; freight.frame(); assert(!freight.eligible());

    clipboard::Model detached_restore;
    detached_restore.configure(job("freight_market")); detached_restore.running = true;
    detached_restore.clock_known = true; detached_restore.connected_known = true;
    detached_restore.connected = false; detached_restore.frame();
    detached_restore.connected = true; detached_restore.frame(); assert(!detached_restore.observed_collection);

    assert(digest("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    const auto temp = std::filesystem::current_path() / L"storage-check.tmp";
    atomic_text(temp, picked.json(), false);
    assert(read_text(temp) == picked.json());
    bool prevented = false; try { atomic_text(temp, "overwrite", false); } catch (...) { prevented = true; }
    assert(prevented); assert(read_text(temp) == picked.json());
    std::filesystem::remove(temp);
    auto sample_job = job("quick_job");
    sample_job["source.company"] = "Waste Management"; sample_job["destination.company"] = "Kiewit";
    sample_job["source.city"] = "Fresno"; sample_job["destination.city"] = "Las Vegas";
    sample_job["cargo"] = "Triturador móvel";
    clipboard::Model sample; sample.configure(sample_job);
    clipboard::Document sample_doc(sample, digest(clipboard::identity(sample_job)));
    atomic_text(std::filesystem::current_path() / L"preview.txt", sample_doc.text(), true);
    std::cout << "PASS: restored job nulls; frozen document; stable identity; cargo load; freight coupling; reconnect; SHA256; no-overwrite persistence\n";
}
