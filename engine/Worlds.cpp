#include "Worlds.h"

#include <fstream>

#include <nlohmann/json.hpp>

std::vector<WorldDesc> listWorlds(const std::string& scenarioPath) {
    std::vector<WorldDesc> out;
    try {
        std::ifstream in(scenarioPath);
        if (!in) return out;
        const nlohmann::json root = nlohmann::json::parse(in);
        for (const auto& w : root.at("worlds"))
            out.push_back({w.at("key").get<std::string>(), w.value("name", w.at("key").get<std::string>()),
                           w.value("desc", "")});
    } catch (const std::exception&) {
        out.clear();
    }
    return out;
}
