// 수치 균형 리포트: 무작위 선택 경로 N개를 모든 세계에 똑같이 넣고, 세계별 평균을 표로 보여준다.
// 같은 경로 번호는 모든 세계에서 같은 선택·같은 시드를 쓴다 (차이는 세계 조건 때문).
#include <algorithm>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

#include "SimEngine.h"
#include "Worlds.h"

namespace {

double at(const std::vector<YearRecord>& recs, int age, const char* key) {
    for (const auto& r : recs)
        if (r.age == age) {
            auto it = r.values.find(key);
            return it == r.values.end() ? 0 : it->second;
        }
    return 0;
}

double net(const std::vector<YearRecord>& recs, int age) {
    return at(recs, age, "assets") + at(recs, age, "deposit") + at(recs, age, "house_value") - at(recs, age, "debt");
}

double median(std::vector<double> v) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

}  // namespace

int main(int argc, char** argv) {
    const int runs = argc > 1 ? std::atoi(argv[1]) : 400;
    const std::string dir = LIFESIM_DATA_DIR;
    const std::string story = dir + "/story/life.json", scenario = dir + "/scenarios/worlds.json";

    // 경로(선택 번호 목록)를 미리 만든다. 선택지 수보다 큰 번호는 엔진이 % 로 맞춘다.
    std::vector<std::vector<int>> paths(runs);
    std::mt19937 rng(12345);
    for (auto& p : paths)
        for (int i = 0; i < 11; ++i) p.push_back(static_cast<int>(rng() % 6));

    std::printf("경로 %d개 (무작위 선택), 단위: 만원\n\n", runs);
    std::printf("%-16s %9s %9s %9s %7s %7s %7s %7s %7s %8s %6s\n", "세계", "순자산35", "순자산50", "중앙50",
                "결혼35", "아이50", "집50", "현금-", "오류", "연금50", "건강");

    for (const WorldDesc& w : listWorlds(scenario)) {
        std::vector<double> n35, n50;
        int married = 0, kids = 0, home = 0, negCash = 0, errors = 0;
        double pension = 0, health = 0;
        for (int r = 0; r < runs; ++r) {
            SimEngine e(story, scenario, w.key, static_cast<unsigned>(r));
            for (int pick : paths[r]) {
                int n = static_cast<int>(e.currentEvent().choices.size());
                e.choose(pick % n);
            }
            if (e.currentEvent().text.find("엔진 오류") != std::string::npos) ++errors;
            const auto recs = e.yearlyRecords();
            n35.push_back(net(recs, 35));
            n50.push_back(net(recs, 50));
            married += at(recs, 35, "married") != 0;
            kids += at(recs, 50, "children") > 0;
            home += at(recs, 50, "house_value") > 0;
            negCash += at(recs, 50, "assets") < 0;
            pension += at(recs, 50, "pension");
            health += at(recs, 50, "health");
        }
        double avg35 = 0, avg50 = 0;
        for (double v : n35) avg35 += v / runs;
        for (double v : n50) avg50 += v / runs;
        auto pct = [&](int c) { return 100.0 * c / runs; };
        std::printf("%-16s %9.0f %9.0f %9.0f %6.0f%% %6.0f%% %6.0f%% %6.0f%% %7d %8.0f %6.0f\n", w.name.c_str(), avg35,
                    avg50, median(n50), pct(married), pct(kids), pct(home), pct(negCash), errors, pension / runs,
                    health / runs);
    }
    return 0;
}
