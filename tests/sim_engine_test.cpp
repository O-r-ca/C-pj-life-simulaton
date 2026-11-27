// SimEngine 테스트: 수식 계산기, 시드 재현성, 같은 선택·다른 세계.
#include <cmath>
#include <random>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

#include "Expr.h"
#include "SimEngine.h"
#include "Worlds.h"

static int failures = 0;

static void check(bool ok, const std::string& what) {
    std::printf("[%s] %s\n", ok ? "OK" : "FAIL", what.c_str());
    if (!ok) ++failures;
}

static double ev(const std::string& s) {
    return evalExpr(s, [](const std::string& n) -> double {
        if (n == "x") return 10;
        if (n == "y") return 4;
        throw std::runtime_error("모르는 이름: " + n);
    });
}

static bool throws(const std::string& s) {
    try { ev(s); } catch (const std::runtime_error&) { return true; }
    return false;
}

static std::vector<std::string> play(const std::string& world, unsigned seed, const std::vector<int>& picks) {
    const std::string dir = LIFESIM_DATA_DIR;
    SimEngine e(dir + "/story/life.json", dir + "/scenarios/worlds.json", world, seed);
    for (int p : picks) e.choose(p);
    return e.chronicle();
}

static void print(const char* title, const std::vector<std::string>& lines) {
    std::printf("  -- %s\n", title);
    for (const auto& l : lines) std::printf("     %s\n", l.c_str());
}

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);  // 중간에 죽어도 출력이 보이게
    // --- 수식 ---
    check(ev("1 + 2 * 3") == 7, "연산 우선순위");
    check(ev("(x - y) / 2") == 3, "괄호와 이름");
    check(ev("-x + 1") == -9, "단항 마이너스");
    check(ev("x > y && !(y > x)") == 1, "비교와 논리");
    check(ev("clamp(x / y, 0, 2) + max(1, 3) + min(1, 3)") == 6, "함수");
    check(ev("0.5 * 4") == 2, "소수");
    check(throws("x +"), "문법 오류는 예외");
    check(throws("z * 2"), "모르는 이름은 예외");
    check(throws("x / 0"), "0으로 나누기는 예외");

    // --- 엔진 ---
    const std::string dir = LIFESIM_DATA_DIR;
    SimEngine e(dir + "/story/life.json", dir + "/scenarios/worlds.json", "baseline", 2045);
    EventView first = e.currentEvent();
    check(first.choices.size() == 3, "첫 장면 선택지 3개");
    check(first.text.find("300만원") != std::string::npos, "첫 장면에 {assets} 값이 들어감");
    check(first.text.find("엔진 오류") == std::string::npos, "JSON 로드 성공");

    // 끝까지: 서울 진학 -> 학업 집중 -> 서울 구직 -> 버틴다 -> 남는다 -> 결혼 -> 가정 -> 집 산다
    //         -> 곁에서 돌봄 -> 회사에 남음 -> 연금 저축  (11번 선택하면 50세 마지막 장면)
    const std::vector<int> picks = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    // 해마다 기록: 19세~50세 빠짐없이 한 줄씩
    for (int p : picks) e.choose(p);
    check(e.currentEvent().text.find("50세") != std::string::npos, "11번 선택하면 50세 마지막 장면");
    check(e.currentEvent().text.find("엔진 오류") == std::string::npos, "끝까지 수식 오류 없음");
    auto recs = e.yearlyRecords();
    bool consecutive = !recs.empty() && recs.front().age == 19;
    for (size_t i = 1; i < recs.size(); ++i) consecutive = consecutive && recs[i].age == recs[i - 1].age + 1;
    check(recs.size() == 32 && consecutive && recs.back().age == 50, "해마다 기록 19~50세 32개");
    check(recs.back().values.count("pension") == 1 && recs.back().values.at("pension") > 0, "연금이 쌓임");
    e.choose(0);  // 처음부터 다시
    check(e.yearlyRecords().size() == 1, "처음부터 다시 하면 기록 초기화");

    auto a1 = play("baseline", 2045, picks);
    auto a2 = play("baseline", 2045, picks);
    auto b1 = play("housing_support", 2045, picks);
    check(a1 == a2, "같은 시드·같은 선택이면 결과가 똑같음 (재현성)");
    check(a1.size() == 11 && b1.size() == 11, "연대기 11줄씩");
    check(a1 != b1, "같은 선택이라도 세계가 다르면 연대기가 다름");
    print("세계 A (시드 2045)", a1);
    print("세계 B (시드 2045)", b1);

    // 모든 갈래(선택지 번호 무작위)를 여러 시드로 돌려도 수식 오류가 없는지
    bool anyError = false;
    std::mt19937 pickRng(7);
    for (unsigned run = 0; run < 300 && !anyError; ++run) {
        for (const char* world : {"baseline", "housing_support"}) {
            SimEngine s(dir + "/story/life.json", dir + "/scenarios/worlds.json", world, run);
            for (int step = 0; step < 11; ++step) {
                int n = static_cast<int>(s.currentEvent().choices.size());
                s.choose(static_cast<int>(pickRng() % n));
            }
            if (s.currentEvent().text.find("엔진 오류") != std::string::npos) {
                anyError = true;
                std::printf("  -- 오류: %s\n", s.currentEvent().text.c_str());
            }
        }
    }
    check(!anyError, "무작위 선택 600번 끝까지 오류 없음");

    // 여러 시드로 돌려서, 주거 지원 세계에서 29세 결혼 성공이 더 잦은지 확인
    auto marriedAt29 = [](const std::vector<std::string>& lines) {
        for (const auto& l : lines)
            if (l.rfind("29세, ", 0) == 0) return l.find("결혼했다") != std::string::npos;
        return false;
    };
    int marriedA = 0, marriedB = 0;
    const int runs = 200;
    for (unsigned s = 1; s <= runs; ++s) {
        if (marriedAt29(play("baseline", s, picks))) ++marriedA;
        if (marriedAt29(play("housing_support", s, picks))) ++marriedB;
    }
    std::printf("  -- 29세 결혼 성공: 세계 A %d/%d, 세계 B %d/%d\n", marriedA, runs, marriedB, runs);
    check(marriedB > marriedA, "주거 지원 세계에서 결혼 성공이 더 많음");

    // 세계 목록: 시나리오에 정의된 순서대로, 모든 세계가 끝까지 오류 없이 돈다
    const auto worlds = listWorlds(dir + "/scenarios/worlds.json");
    check(worlds.size() == 8 && worlds[0].key == "baseline" && !worlds[0].name.empty(), "세계 목록 8개, 순서 유지");
    bool allOk = !worlds.empty();
    for (const auto& w : worlds) {
        SimEngine s(dir + "/story/life.json", dir + "/scenarios/worlds.json", w.key, 2045);
        for (int p : picks) s.choose(p);
        allOk = allOk && s.currentEvent().text.find("엔진 오류") == std::string::npos;
    }
    check(allOk, "모든 세계가 끝까지 오류 없음");
    SimEngine unknown(dir + "/story/life.json", dir + "/scenarios/worlds.json", "nope", 1);
    check(unknown.currentEvent().text.find("없는 세계 키") != std::string::npos, "없는 세계 키는 오류 화면");

    SimEngine broken(dir + "/story/nope.json", dir + "/scenarios/worlds.json", "baseline", 1);
    check(broken.currentEvent().text.find("엔진 오류") != std::string::npos, "파일 없으면 오류 화면");

    std::printf(failures ? "\n%d개 실패\n" : "\n전부 통과\n", failures);
    return failures ? 1 : 0;
}
