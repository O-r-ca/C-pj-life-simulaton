// ImGui 화면. 엔진은 Engine 인터페이스로만 접근한다.
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "SimEngine.h"
#include "Theme.h"
#include "Worlds.h"
#include "Widgets.h"

using namespace theme;

namespace {

// ───────────── 데이터 ─────────────

const int kStartAge = 19;
const int kEndAge = 50;

// 화면 칸(슬롯): 0 = 세계 A (플레이어가 사는 세계), 1 = 세계 B (비교 세계)
const char* const kSlotName[2] = {u8"세계 A", u8"세계 B"};
const ImU32 kSlotColor[2] = {Coral, Teal};

struct World {
    int slot = 0;
    int index = 0;  // worldList 안의 번호
    std::unique_ptr<Engine> engine;
};

enum class Screen { Start, Life, Compare };

struct App {
    std::string dataDir;
    std::vector<WorldDesc> worldList;  // 시나리오 파일의 세계들 (엔진이 읽어 준다)
    Screen screen = Screen::Start;
    std::string name;
    unsigned seed = 0;
    World worlds[2];
    int sel[2] = {0, 1};        // 시작 화면에서 고른 세계
    std::vector<int> picks;     // 지금까지의 선택 (세계를 바꾸면 다시 재생)
    int version = 0;            // 선택할 때마다 +1 (자동 스크롤용)
    int feedVersion = -1;
    int panelVersion[2] = {-1, -1};
    int metric = 0;

    // 시작 화면 입력
    char nameBuf[64] = "";
    char seedBuf[16] = "";
    bool showSeed = false;

    std::string storyPath() const { return dataDir + "/story/life.json"; }
    std::string scenarioPath() const { return dataDir + "/scenarios/worlds.json"; }
    const WorldDesc& desc(const World& w) const { return worldList[w.index]; }
    std::string title(const World& w) const { return std::string(kSlotName[w.slot]) + u8" · " + desc(w).name; }
};

// 그래프 지표. 속성 이름은 지금 스토리(life.json)의 속성을 따른다.
double attr(const YearRecord& r, const char* name) {
    auto it = r.values.find(name);
    return it == r.values.end() ? 0.0 : it->second;
}

struct Metric {
    const char* label;
    const char* unit;
    double (*value)(const YearRecord&);
};

double netWorth(const YearRecord& r) {
    return attr(r, "assets") + attr(r, "deposit") + attr(r, "house_value") - attr(r, "debt");
}

const Metric kMetrics[] = {
    {u8"순자산", u8"만원", [](const YearRecord& r) { return netWorth(r); }},
    {u8"모아둔 돈", u8"만원", [](const YearRecord& r) { return attr(r, "assets"); }},
    {u8"대출", u8"만원", [](const YearRecord& r) { return attr(r, "debt"); }},
    {u8"연봉", u8"만원", [](const YearRecord& r) { return attr(r, "income"); }},
    {u8"연금", u8"만원", [](const YearRecord& r) { return attr(r, "pension"); }},
    {u8"건강", u8"점", [](const YearRecord& r) { return attr(r, "health"); }},
};
const int kMetricCount = static_cast<int>(sizeof(kMetrics) / sizeof(kMetrics[0]));

YearRecord latest(Engine& e) {
    auto rs = e.yearlyRecords();
    return rs.empty() ? YearRecord{0, {}} : rs.back();
}

std::string money(double v) { return ui::formatNumber(v) + u8"만원"; }

// "29세, 결혼했다." -> 29, "결혼했다."
bool splitAge(const std::string& line, int& age, std::string& text) {
    const std::string sep = u8"세, ";
    size_t p = line.find(sep);
    if (p == std::string::npos || p == 0 || p > 4) return false;
    age = std::atoi(line.substr(0, p).c_str());
    text = line.substr(p + sep.size());
    return true;
}

std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// 칸 하나의 세계를 (다시) 만들고, 지금까지의 선택을 같은 시드로 그대로 다시 넣는다.
// 같은 스토리, 같은 시드, 같은 선택. 세계 키만 다르다. 진짜 엔진은 여기서만 만든다.
void buildWorld(App& app, int slot, int index) {
    World& w = app.worlds[slot];
    w.slot = slot;
    w.index = index;
    w.engine = std::make_unique<SimEngine>(app.storyPath(), app.scenarioPath(), app.worldList[index].key, app.seed);
    for (int p : app.picks) w.engine->choose(p);
    app.version++;
}

void startLife(App& app, const std::string& name, unsigned seed) {
    app.name = name;
    app.seed = seed;
    app.picks.clear();
    for (int i = 0; i < 2; ++i) buildWorld(app, i, app.sel[i]);
    app.screen = Screen::Life;
}

void chooseAll(App& app, int index) {
    for (World& w : app.worlds) w.engine->choose(index);
    app.picks.push_back(index);
    if (app.worlds[0].engine->yearlyRecords().size() <= 1) app.picks.clear();  // "처음부터 다시"였다
    app.version++;
}

// 세계 고르기 콤보. 바뀌면 true
bool worldCombo(const char* id, App& app, int* index, float width) {
    bool changed = false;
    ImGui::SetNextItemWidth(width);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 8));
    if (ImGui::BeginCombo(id, app.worldList[*index].name.c_str())) {
        for (int i = 0; i < static_cast<int>(app.worldList.size()); ++i) {
            const bool on = i == *index;
            if (ImGui::Selectable(app.worldList[i].name.c_str(), on) && !on) {
                *index = i;
                changed = true;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", app.worldList[i].desc.c_str());
        }
        ImGui::EndCombo();
    }
    ImGui::PopStyleVar();
    return changed;
}

// ───────────── 시작 화면 ─────────────

void drawStart(App& app) {
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float cardW = 560;
    ImGui::SetCursorPos(ImVec2((avail.x - cardW) * 0.5f, std::max(40.0f, avail.y * 0.5f - 280)));
    ui::beginCard("start", cardW);

    ImGui::PushFont(fonts.title);
    ImGui::TextUnformatted(u8"N년 후 한국");
    ImGui::PopFont();
    ImGui::PushFont(fonts.bold);
    ImGui::PushStyleColor(ImGuiCol_Text, Gold);
    ImGui::TextUnformatted(u8"인생 시뮬레이션");
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0, 4));

    ImGui::PushStyleColor(ImGuiCol_Text, TextDim);
    ImGui::PushTextWrapPos(0);
    ImGui::TextUnformatted(u8"같은 사람이 같은 선택을 해도, 사는 세계의 조건이 다르면 인생이 달라진다. "
                           u8"이 프로그램은 미래를 예측하지 않는다. 조건이 바뀌면 어떻게 달라지는지 보여주는 실험 도구다.");
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 8));

    ui::label(u8"이름");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##name", u8"비워 두면 무작위", app.nameBuf, sizeof(app.nameBuf));
    ImGui::Dummy(ImVec2(0, 2));

    ImGui::PushFont(fonts.small);
    ImGui::PushStyleColor(ImGuiCol_Text, TextDim);
    if (ImGui::Selectable(app.showSeed ? u8"- 시드 지정 (선택)" : u8"+ 시드 지정 (선택)", false)) app.showSeed = !app.showSeed;
    ImGui::PopStyleColor();
    ImGui::PopFont();
    if (app.showSeed) {
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##seed", u8"비워 두면 무작위 · 같은 시드면 같은 운", app.seedBuf, sizeof(app.seedBuf),
                                 ImGuiInputTextFlags_CharsDecimal);
    }
    ImGui::Dummy(ImVec2(0, 2));

    // 세계 고르기: A = 내가 살 세계, B = 같은 선택을 넣어 볼 세계
    const float half = (ImGui::GetContentRegionAvail().x - 12) * 0.5f;
    const ImVec2 rowStart = ImGui::GetCursorScreenPos();
    ImGui::BeginGroup();
    ui::label(u8"살아갈 세계 (A)");
    worldCombo("##selA", app, &app.sel[0], half);
    ImGui::EndGroup();
    // SameLine을 쓰면 글자 기준선이 콤보 높이에 맞춰 내려가서, 위치를 직접 지정한다
    ImGui::SetCursorScreenPos(ImVec2(rowStart.x + half + 12, rowStart.y));
    ImGui::BeginGroup();
    ui::label(u8"비교할 세계 (B)");
    worldCombo("##selB", app, &app.sel[1], half);
    ImGui::EndGroup();
    ImGui::PushFont(fonts.small);
    ImGui::PushStyleColor(ImGuiCol_Text, TextFaint);
    ImGui::PushTextWrapPos(0);
    ImGui::TextUnformatted(app.worldList[app.sel[1]].desc.c_str());
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0, 6));

    if (ui::goldButton(u8"태어나기", ImVec2(150, 52))) {
        static const char* names[] = {u8"서연", u8"민준", u8"지우", u8"하준", u8"서윤",
                                      u8"도윤", u8"하은", u8"시우", u8"지아", u8"유준"};
        std::random_device rd;
        std::string name = app.nameBuf[0] ? app.nameBuf : names[rd() % (sizeof(names) / sizeof(names[0]))];
        unsigned seed = app.seedBuf[0] ? static_cast<unsigned>(std::strtoul(app.seedBuf, nullptr, 10)) : rd();
        startLife(app, name, seed);
    }
    ui::endCard();
}

// ───────────── 상단 바 ─────────────

void drawTopBar(App& app) {
    const float barH = 76;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Panel);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(26, 8));
    ImGui::BeginChild("top", ImVec2(0, barH), ImGuiChildFlags_AlwaysUseWindowPadding);

    const int age = latest(*app.worlds[0].engine).age;

    ImGui::PushFont(fonts.big);
    ImGui::TextUnformatted(std::to_string(age).c_str());
    ImGui::PopFont();
    ImGui::SameLine(0, 6);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + fonts.big->FontSize - fonts.bold->FontSize - 8);
    ImGui::PushFont(fonts.bold);
    ImGui::TextDisabled(u8"세");
    ImGui::PopFont();

    const char* tabs[] = {u8"인생 진행", u8"평행 세계 비교"};
    const float rightW = ui::segmentedWidth(tabs, 2) + 12 + ImGui::CalcTextSize(u8"새 인생").x + 40;

    ImGui::SameLine(0, 28);
    const float barW = std::max(60.0f, ImGui::GetContentRegionAvail().x - rightW - 36);
    ImGui::SetCursorPosY(barH * 0.5f - 2);
    ui::progressBar(float(age - kStartAge) / float(kEndAge - kStartAge), barW, 5, Gold);

    // SameLine은 줄의 처음 높이로 돌아가므로, 오른쪽 버튼들은 화면 좌표로 직접 놓는다
    const ImVec2 win = ImGui::GetWindowPos();
    const float controlsY = win.y + barH * 0.5f - 18;
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(ImVec2(ImGui::GetItemRectMax().x + 36, controlsY));
    int cur = app.screen == Screen::Compare ? 1 : 0;
    if (ui::segmented("tabs", tabs, 2, &cur)) app.screen = cur == 0 ? Screen::Life : Screen::Compare;
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(ImVec2(ImGui::GetItemRectMax().x + 12, controlsY));
    if (ui::ghostButton(u8"새 인생")) app.screen = Screen::Start;

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddLine(ImVec2(a.x, b.y), ImVec2(b.x, b.y), Border);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y);  // 아래 영역과 틈 없이
}

// ───────────── 인생 진행 화면 ─────────────

void drawSidebar(App& app, World& w) {
    const YearRecord r = latest(*w.engine);

    ui::beginCard("person");
    ImGui::PushFont(fonts.title);
    ImGui::TextUnformatted(app.name.c_str());
    ImGui::PopFont();
    std::string sub = attr(r, "in_seoul") != 0 ? u8"서울" : u8"지방";
    sub += attr(r, "married") != 0 ? u8" · 기혼" : u8" · 미혼";
    if (attr(r, "children") > 0) sub += u8" · 아이 " + ui::formatNumber(attr(r, "children")) + u8"명";
    ImGui::PushFont(fonts.small);
    ImGui::PushStyleColor(ImGuiCol_Text, TextDim);
    ImGui::TextUnformatted(sub.c_str());
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ui::endCard();

    ui::beginCard("money");
    ui::label((u8"살림 · 순자산 " + money(netWorth(r))).c_str());
    ui::statBar(u8"현금", money(attr(r, "assets")), float(attr(r, "assets") / 50000), Green);
    if (attr(r, "house_value") > 0)
        ui::statBar(u8"집", money(attr(r, "house_value")), float(attr(r, "house_value") / 150000), Gold);
    else
        ui::statBar(u8"보증금", money(attr(r, "deposit")), float(attr(r, "deposit") / 80000), Gold);
    ui::statBar(u8"대출", money(attr(r, "debt")), float(attr(r, "debt") / 80000), Pink);
    ui::statBar(u8"연봉", money(attr(r, "income")), float(attr(r, "income") / 8000), Blue);
    ui::statBar(u8"연금", money(attr(r, "pension")), float(attr(r, "pension") / 30000), Teal);
    ui::endCard();

    ui::beginCard("life");
    ui::label(u8"몸과 일");
    ui::statBar(u8"건강", ui::formatNumber(attr(r, "health")), float(attr(r, "health") / 100), Green);
    ui::keyValue(u8"일", attr(r, "income") <= 0 ? u8"없음"
                         : attr(r, "job_stable") != 0 ? u8"정규직" : u8"계약직 · 불안정");
    if (attr(r, "caring") > 0) ui::keyValue(u8"가족", u8"부모님 돌봄 중");
    ui::endCard();

    ui::beginCard("world");
    ImGui::PushFont(fonts.bold);
    ImGui::PushStyleColor(ImGuiCol_Text, kSlotColor[w.slot]);
    ImGui::TextUnformatted(app.title(w).c_str());
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::PushFont(fonts.small);
    ImGui::PushStyleColor(ImGuiCol_Text, TextDim);
    ImGui::PushTextWrapPos(0);
    ImGui::TextUnformatted(app.desc(w).desc.c_str());
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0, 2));
    ui::keyValue(u8"시드", std::to_string(app.seed));
    ui::endCard();
}

void drawFeed(App& app) {
    World& w = app.worlds[0];
    const float avail = ImGui::GetContentRegionAvail().x;
    const float colW = std::min(780.0f, avail - 80);
    const float childW = colW + ImGui::GetStyle().ScrollbarSize + 8;
    ImGui::SetCursorPosX((avail - childW) * 0.5f);
    ImGui::BeginChild("feed", ImVec2(childW, 0));

    const float W = colW;
    ImGui::Dummy(ImVec2(0, 26));

    int lastAge = -1;
    for (const std::string& line : w.engine->chronicle()) {
        int age = lastAge;
        std::string text = line;
        splitAge(line, age, text);
        if (age != lastAge) {
            ui::agePill(std::to_string(age) + u8"세", false);
            lastAge = age;
        }
        ui::narrative(text, fonts.serif, TextDim, Border, W);
        ImGui::Dummy(ImVec2(0, 8));
    }

    ui::agePill(std::to_string(latest(*w.engine).age) + u8"세", true);
    ImGui::Dummy(ImVec2(0, 4));
    const EventView ev = w.engine->currentEvent();
    ui::narrative(ev.text, fonts.serif, Text, Gold, W);
    ImGui::Dummy(ImVec2(0, 16));

    int picked = -1;
    for (int i = 0; i < static_cast<int>(ev.choices.size()); ++i) {
        ImGui::PushID(i);
        if (ui::choiceButton("##choice", ev.choices[i], W)) picked = i;
        ImGui::PopID();
        ImGui::Dummy(ImVec2(0, 0));
    }
    ImGui::Dummy(ImVec2(0, 40));

    if (app.feedVersion != app.version) {
        ImGui::SetScrollHereY(1.0f);
        app.feedVersion = app.version;
    }
    ImGui::EndChild();

    if (picked >= 0) chooseAll(app, picked);
}

void drawLife(App& app) {
    const float sideW = 320;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Panel);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
    ImGui::BeginChild("side", ImVec2(sideW, 0), ImGuiChildFlags_AlwaysUseWindowPadding);
    drawSidebar(app, app.worlds[0]);
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    const ImVec2 b = ImGui::GetItemRectMax(), a = ImGui::GetItemRectMin();
    ImGui::GetWindowDrawList()->AddLine(ImVec2(b.x, a.y), ImVec2(b.x, b.y), Border);

    ImGui::SameLine(0, 0);
    ImGui::BeginChild("main", ImVec2(0, 0));
    drawFeed(app);
    ImGui::EndChild();
}

// ───────────── 평행 세계 비교 화면 ─────────────

void drawPlot(App& app) {
    if (!ImPlot::BeginPlot("##compare", ImVec2(-1, 210), ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect)) return;
    ImPlot::SetupAxes(u8"나이", kMetrics[app.metric].unit, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
    ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Outside | ImPlotLegendFlags_Horizontal);
    for (World& w : app.worlds) {
        std::vector<double> xs, ys;
        for (const YearRecord& r : w.engine->yearlyRecords()) {
            xs.push_back(r.age);
            ys.push_back(kMetrics[app.metric].value(r));
        }
        const ImVec4 c = ImGui::ColorConvertU32ToFloat4(kSlotColor[w.slot]);
        ImPlot::SetNextLineStyle(c, 2.5f);
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 3.5f, c, 0, c);
        const std::string label = app.title(w) + "##" + std::to_string(w.slot);  // 같은 세계 둘이어도 따로
        ImPlot::PlotLine(label.c_str(), xs.data(), ys.data(), static_cast<int>(xs.size()));
    }
    ImPlot::EndPlot();
}

void drawTimeline(World& w, const std::vector<std::string>& other) {
    const std::vector<std::string> lines = w.engine->chronicle();
    const float W = ImGui::GetContentRegionAvail().x;
    if (lines.empty()) ui::label(u8"아직 기록이 없다.");
    int lastAge = -1;
    for (size_t i = 0; i < lines.size(); ++i) {
        int age = lastAge;
        std::string text = lines[i];
        splitAge(lines[i], age, text);
        if (age != lastAge) {
            ui::agePill(std::to_string(age) + u8"세", false);
            lastAge = age;
        }
        const bool differs = i >= other.size() || other[i] != lines[i];
        ui::narrative(text, fonts.body, differs ? Text : TextFaint, differs ? kSlotColor[w.slot] : Border, W);
        ImGui::Dummy(ImVec2(0, 4));
    }
}

void drawCompare(App& app) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(28, 22));
    ImGui::BeginChild("compare", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);

    ImGui::PushFont(fonts.title);
    ImGui::TextUnformatted(u8"같은 선택, 다른 세계");
    ImGui::PopFont();
    ImGui::PushStyleColor(ImGuiCol_Text, TextDim);
    ImGui::PushTextWrapPos(0);
    ImGui::Text(u8"%s의 선택을 두 세계에 똑같이 넣었다. 시드 %u, 운도 같다. 달라진 것은 세계의 조건뿐이다. "
                u8"색이 진한 줄이 두 세계에서 갈라진 부분이다.",
                app.name.c_str(), app.seed);
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 4));

    const char* labels[kMetricCount];
    for (int i = 0; i < kMetricCount; ++i) labels[i] = kMetrics[i].label;
    ui::segmented("metric", labels, kMetricCount, &app.metric);

    ui::beginCard("plot");
    drawPlot(app);
    ui::endCard();

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float gap = 16;
    const float half = (avail.x - gap) * 0.5f;
    const float h = std::max(240.0f, avail.y);
    std::vector<std::string> chron[2] = {app.worlds[0].engine->chronicle(), app.worlds[1].engine->chronicle()};

    for (int i = 0; i < 2; ++i) {
        World& w = app.worlds[i];
        if (i > 0) ImGui::SameLine(0, gap);
        ui::beginPanel(i == 0 ? "worldA" : "worldB", ImVec2(half, h), Card);

        // 머리: ● 세계 A  [세계 고르기 ▾]  — 바꾸면 같은 선택을 새 세계에 다시 넣는다
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(p.x + 6, p.y + 19), 5, kSlotColor[w.slot]);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
        ImGui::PushFont(fonts.bold);
        ImGui::TextUnformatted(kSlotName[w.slot]);
        ImGui::PopFont();
        ImGui::SameLine(0, 12);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 8);
        int index = w.index;
        if (worldCombo(i == 0 ? "##cmpA" : "##cmpB", app, &index, std::min(240.0f, ImGui::GetContentRegionAvail().x)))
            buildWorld(app, i, index);
        ImGui::PushFont(fonts.small);
        ImGui::PushStyleColor(ImGuiCol_Text, TextFaint);
        ImGui::PushTextWrapPos(0);
        ImGui::TextUnformatted(app.desc(w).desc.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
        ImGui::PopFont();

        const YearRecord r = latest(*w.engine);
        const std::string summary = u8"순자산 " + money(netWorth(r)) + u8" · 대출 " + money(attr(r, "debt")) +
                                    u8" · 아이 " + ui::formatNumber(attr(r, "children")) + u8"명 · 건강 " +
                                    ui::formatNumber(attr(r, "health"));
        ui::label(summary.c_str());
        ImGui::Separator();

        // 머리는 고정, 타임라인만 스크롤
        ImGui::BeginChild("timeline", ImVec2(0, 0));
        drawTimeline(w, chron[1 - i]);
        if (app.panelVersion[i] != app.version) {
            ImGui::SetScrollHereY(1.0f);
            app.panelVersion[i] = app.version;
        }
        ImGui::EndChild();
        ui::endPanel();
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
}

// ───────────── 개발용: 명령줄로 자동 진행 + 스크린샷 ─────────────
// 예: lifesim --name 테스트 --seed 2045 --picks 0,1,0,0 --screen compare --shot out.bmp

struct DevArgs {
    bool autoStart = false;
    std::string name = "Test";
    unsigned seed = 2045;
    std::string worldA, worldB;  // 세계 키 (--a, --b)
    std::vector<int> picks;
    std::string screen;
    std::string shot;
};

DevArgs parseArgs(int argc, char** argv) {
    DevArgs a;
    for (int i = 1; i + 1 < argc; i += 2) {
        std::string k = argv[i], v = argv[i + 1];
        a.autoStart = true;
        if (k == "--name") a.name = v;
        else if (k == "--seed") a.seed = static_cast<unsigned>(std::strtoul(v.c_str(), nullptr, 10));
        else if (k == "--screen") a.screen = v;
        else if (k == "--a") a.worldA = v;
        else if (k == "--b") a.worldB = v;
        else if (k == "--shot") a.shot = v;
        else if (k == "--picks") {
            std::stringstream ss(v);
            std::string tok;
            while (std::getline(ss, tok, ',')) a.picks.push_back(std::atoi(tok.c_str()));
        }
    }
    return a;
}

bool writeBmp(const std::string& path, int w, int h) {
    std::vector<unsigned char> px(static_cast<size_t>(w) * h * 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    const int row = (w * 3 + 3) & ~3;
    const unsigned size = 54 + row * h;
    unsigned char hdr[54] = {'B', 'M'};
    auto put32 = [&](int at, unsigned v) { for (int i = 0; i < 4; ++i) hdr[at + i] = (v >> (8 * i)) & 0xFF; };
    put32(2, size);
    put32(10, 54);
    put32(14, 40);
    put32(18, w);
    put32(22, h);
    hdr[26] = 1;
    hdr[28] = 24;
    std::fwrite(hdr, 1, 54, f);
    std::vector<unsigned char> line(row, 0);
    for (int y = 0; y < h; ++y) {  // OpenGL도 BMP도 아래 줄부터
        for (int x = 0; x < w; ++x) {
            const unsigned char* s = &px[(static_cast<size_t>(y) * w + x) * 4];
            line[x * 3 + 0] = s[2];
            line[x * 3 + 1] = s[1];
            line[x * 3 + 2] = s[0];
        }
        std::fwrite(line.data(), 1, row, f);
    }
    std::fclose(f);
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* window = glfwCreateWindow(1280, 820, u8"N년 후 한국: 인생 시뮬레이션", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    App app;
    app.dataDir = LIFESIM_DATA_DIR;
    app.worldList = listWorlds(app.scenarioPath());
    if (app.worldList.empty()) app.worldList.push_back({"baseline", u8"(시나리오를 읽지 못함)", app.scenarioPath()});
    const int last = static_cast<int>(app.worldList.size()) - 1;
    app.sel[0] = 0;
    app.sel[1] = std::min(1, last);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    loadFonts(io, readFile(app.storyPath()) + readFile(app.scenarioPath()));
    theme::apply();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    const DevArgs dev = parseArgs(argc, argv);
    for (int i = 0; i <= last; ++i) {
        if (app.worldList[i].key == dev.worldA) app.sel[0] = i;
        if (app.worldList[i].key == dev.worldB) app.sel[1] = i;
    }
    if (dev.autoStart && dev.screen != "start") {
        startLife(app, dev.name, dev.seed);
        for (int p : dev.picks) chooseAll(app, p);
        if (dev.screen == "compare") app.screen = Screen::Compare;
    }

    int frame = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::Begin("root", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoBringToFrontOnFocus);

        if (app.screen == Screen::Start) {
            drawStart(app);
        } else {
            drawTopBar(app);
            if (app.screen == Screen::Life) drawLife(app);
            else drawCompare(app);
        }

        ImGui::End();

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        const ImVec4 bg = ImGui::ColorConvertU32ToFloat4(Bg);
        glClearColor(bg.x, bg.y, bg.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (!dev.shot.empty() && ++frame == 5) {  // 레이아웃·스크롤이 자리 잡은 뒤 저장하고 종료
            writeBmp(dev.shot, w, h);
            glfwSetWindowShouldClose(window, 1);
        }
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
