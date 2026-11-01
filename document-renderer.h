#pragma once
#include "branding.h"
#include "language.h"
#include <gdiplus.h>
#include <memory>
#pragma comment(lib, "gdiplus.lib")
inline std::unique_ptr<Gdiplus::Image> board_image;
inline std::map<std::wstring,std::unique_ptr<Gdiplus::Image>> logo_images;
inline void release_visual_resources() { logo_images.clear(); board_image.reset(); }
inline std::wstring wide(const std::string& value) {
    const auto n = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring out(n, L' ');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), out.data(), n);
    return out;
}
inline std::string visible(const std::string& s) { return s.empty() ? "—" : s; }
inline std::string date_label(const Json& doc, const char* key) {
    if (!doc.contains(key) || !doc[key].is_number_unsigned()) return "—";
    const auto n = doc[key].get<unsigned>();
    std::ostringstream out; out << std::setfill('0') << std::setw(2) << n % 1440 / 60 << ':' << std::setw(2) << n % 60;
    return i18n::format("date",{{"day",std::to_string(n/1440+1)},{"time",out.str()}});
}
inline std::string printed_number(const std::string& number, bool decimal) {
    try {
        const auto n = std::stod(number);
        if (!std::isfinite(n) || n < 0 || n > 1e10) return "—";
        std::ostringstream out; out << std::fixed << std::setprecision(decimal ? 1 : 0) << n;
        auto s = out.str(); const auto dot = s.find('.'); auto integer = s.substr(0, dot);
        for (int i = static_cast<int>(integer.size()) - 3; i > 0; i -= 3) integer.insert(static_cast<size_t>(i), i18n::tr("thousands"));
        if (decimal && dot != std::string::npos && s.substr(dot + 1) != "0") integer += i18n::tr("decimal") + s.substr(dot + 1);
        return integer;
    } catch (...) { return "—"; }
}
inline void draw_clipboard(Gdiplus::Graphics& g, int width, int height, const Json& envelope, const std::filesystem::path& asset_path, const std::wstring& shortcut=L"F8") {
    using namespace Gdiplus;
    g.Clear(Color(0, 0, 0, 0));
    g.SetSmoothingMode(SmoothingModeAntiAlias); g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.ScaleTransform(static_cast<REAL>(width) / 1024, static_cast<REAL>(height) / 1536);
    auto& board = board_image;
    if (!board) board = std::make_unique<Image>((asset_path / L"clipboard-realistic-v1.png").c_str());
    if (board->GetLastStatus() == Ok) g.DrawImage(board.get(), 0, 0, 1024, 1536);
    {
        SolidBrush hint(Color(255,246,240,222));Font font(i18n::font_family(),18,FontStyleRegular,UnitPixel);
        StringFormat format;format.SetAlignment(StringAlignmentCenter);format.SetTrimming(StringTrimmingEllipsisCharacter);
        const auto toggle=i18n::wtr("toggle")+L"  ·  "+shortcut;
        g.DrawString(toggle.c_str(),-1,&font,RectF(146,1420,732,28),&format,&hint);
        const auto caption=i18n::utf16(i18n::tr("settings_title")+"  ·  Ctrl + F8");
        g.DrawString(caption.c_str(),-1,&font,RectF(146,1451,732,28),&format,&hint);
    }
    if (envelope.is_null() || !envelope.contains("document") || !envelope["document"].is_object()) return;
    const auto& doc = envelope["document"];
    SolidBrush ink(Color(255, 36, 40, 43)), muted(Color(255, 92, 96, 98));
    Pen rule(Color(255, 116, 119, 118), 1.4f);
    auto text = [&](const std::string& s, REAL x, REAL y, REAL w, REAL h, REAL size, bool bold = false, bool label = false) {
        StringFormat format; format.SetTrimming(StringTrimmingEllipsisWord);
        const auto value = wide(s);
        std::unique_ptr<Font> font;
        for(REAL fit=size;;fit-=0.5f) {
            font=std::make_unique<Font>(i18n::font_family(),fit,bold?FontStyleBold:FontStyleRegular,UnitPixel);
            RectF measured;
            INT fitted=0,lines=0;
            g.MeasureString(value.c_str(),static_cast<INT>(value.size()),font.get(),RectF(0,0,w,h),&format,&measured,&fitted,&lines);
            if(fitted>=static_cast<INT>(value.size()) || fit<=size*0.7f) break;
        }
        g.DrawString(value.c_str(), static_cast<INT>(value.size()), font.get(), RectF(x,y,w,h), &format, label ? &muted : &ink);
    };
    const auto company = visible(job_value(doc, "source.company"));
    bool has_logo = false;
    const auto logo = envelope.value("logo_path", "");
    if (!logo.empty()) {
        auto& cache = logo_images;
        const auto path = wide(logo);
        if (!cache.count(path)) cache[path] = std::make_unique<Image>(path.c_str());
        auto* image = cache[path].get();
        if (image->GetLastStatus() == Ok && image->GetHeight()) {
            const REAL h = 61, w = (std::min)(250.0f, h * image->GetWidth() / image->GetHeight());
            g.DrawImage(image, RectF(148,302,w,h)); has_logo = true;
        }
    }
    text(company, 146, has_logo ? 369.0f : 310.0f, 465, 62, has_logo ? 22.0f : 33.0f, true);
    text(i18n::tr("document_number"), 633, 307, 245, 30, 17, false, true);
    text(doc.value("document_id", "").substr(0,12), 633, 338, 245, 44, 27, true);
    text(i18n::tr("title"), 146, 433, 732, 48, 33, true);
    g.DrawLine(&rule, 146.0f, 527.0f, 878.0f, 527.0f);
    auto location = [&](const char* city, const char* state) {
        return visible(i18n::game_name("cities",job_value(doc,(std::string(city)+".id").c_str()),job_value(doc,city))) + " / " + (doc.contains(state) && doc[state].is_string() ? doc[state].get<std::string>() : "—");
    };
    text(i18n::tr("sender"), 146, 551, 342, 30, 18, true, true);
    text(i18n::tr("recipient"), 532, 551, 346, 30, 18, true, true);
    text(company, 146, 590, 342, 78, 27, true);
    text(visible(job_value(doc,"destination.company")), 532, 590, 346, 78, 27, true);
    text(location("source.city","source_state"), 146, 675, 342, 40, 25);
    text(location("destination.city","destination_state"), 532, 675, 346, 40, 25);
    g.DrawLine(&rule, 146.0f, 732.0f, 878.0f, 732.0f);
    SolidBrush band(Color(255, 228, 228, 221)); g.FillRectangle(&band, RectF(146,766,732,44));
    g.DrawRectangle(&rule, RectF(146,766,732,154));
    g.DrawLine(&rule, 608.0f, 766.0f, 608.0f, 920.0f); g.DrawLine(&rule, 704.0f, 766.0f, 704.0f, 920.0f);
    text(i18n::tr("cargo"), 160, 776, 433, 27, 17, true);
    text(i18n::tr("quantity"), 620, 776, 80, 27, 17, true); text(i18n::tr("weight"), 718, 776, 148, 27, 17, true);
    text(visible(i18n::game_name("cargo",job_value(doc,"cargo.id"),job_value(doc,"cargo"))), 160, 835, 433, 73, 27);
    text(visible(job_value(doc,"cargo.unit.count")), 620, 835, 80, 73, 27);
    text(printed_number(job_value(doc,"cargo.mass"),true), 718, 835, 148, 73, 25);
    text(i18n::tr("collection"),146,961,342,27,18,true,true); text(i18n::tr("deadline"),532,961,346,27,18,true,true);
    text(date_label(doc,"collection_game_minutes"),146,997,342,43,27);
    Json deadline;
    try { deadline["minutes"] = static_cast<unsigned>(std::stoul(job_value(doc,"delivery.time"))); } catch (...) {}
    text(date_label(deadline,"minutes"),532,997,346,43,27);
    text(i18n::tr("distance"),146,1071,342,27,18,true,true); text(i18n::tr("available"),532,1071,346,27,18,true,true);
    text(printed_number(job_value(doc,"planned_distance.km"),false)+" km",146,1107,342,43,27);
    std::string duration = "—";
    if (doc.contains("available_minutes_at_collection") && doc["available_minutes_at_collection"].is_number_integer()) {
        const auto n = doc["available_minutes_at_collection"].get<long long>(); duration = i18n::format("duration",{{"h",std::to_string(n/60)},{"m",std::to_string(n%60)}});
    }
    text(duration,532,1107,346,43,27);
    g.DrawLine(&rule,146.0f,1268.0f,472.0f,1268.0f); g.DrawLine(&rule,552.0f,1268.0f,878.0f,1268.0f);
    text(i18n::tr("dispatch"),146,1284,326,28,17,false,true); text(i18n::tr("carrier"),552,1284,326,28,17,false,true);
}
