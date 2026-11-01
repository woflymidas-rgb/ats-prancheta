#pragma once
#include <string>
inline std::string paper_content(std::string text) {
    // Only a transport document belongs on the simulated sheet.
    if (text.rfind("DOCUMENTO DE TRANSPORTE\r\n", 0) != 0) return "";
    const std::string legacy = "\r\n\r\nDocumento fixo. — = informação não registrada.\r\n";
    if (text.size() >= legacy.size() && text.compare(text.size() - legacy.size(), legacy.size(), legacy) == 0)
        text.erase(text.size() - legacy.size());
    return text;
}
