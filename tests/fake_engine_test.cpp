// 가짜 엔진 스모크 테스트: 스토리 JSON을 읽고, 같은 선택을 두 세계에 넣어 결과를 확인한다.
#include <cstdio>
#include <string>

#include "FakeEngine.h"

static int failures = 0;

static void check(bool ok, const char* what) {
    std::printf("[%s] %s\n", ok ? "OK" : "FAIL", what);
    if (!ok) ++failures;
}

int main() {
    const std::string story = std::string(LIFESIM_DATA_DIR) + "/story/fake_story.json";
    FakeEngine a(story, "default");
    FakeEngine b(story, "housing_support");

    check(a.currentEvent().choices.size() == 3, "첫 장면에 선택지 3개");
    check(a.currentEvent().text.find("수능") != std::string::npos, "첫 장면 글을 JSON에서 읽음");

    // 서울 진학 -> 학업 집중 -> 결혼 -> 아이
    for (int pick : {0, 1, 0, 0}) { a.choose(pick); b.choose(pick); }

    auto ca = a.chronicle();
    auto cb = b.chronicle();
    check(ca.size() == 4 && cb.size() == 4, "연대기 4줄씩");
    check(ca[0] == cb[0], "공통 문장은 두 세계가 같음");
    check(ca[2] == "28세, 대출을 크게 받아 결혼했다.", "세계 A 결혼 문장");
    check(cb[2] == "28세, 신혼부부 전세 지원으로 결혼했다.", "세계 B 결혼 문장");

    a.choose(0);  // 처음부터 다시
    check(a.chronicle().empty() && a.currentEvent().choices.size() == 3, "처음부터 다시");

    FakeEngine missing(story + ".nope", "default");
    check(missing.currentEvent().text.find("불러오지 못했다") != std::string::npos, "파일 없으면 오류 장면");

    std::printf(failures ? "\n%d개 실패\n" : "\n전부 통과\n", failures);
    return failures ? 1 : 0;
}
