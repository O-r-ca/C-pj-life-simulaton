#define IMGUI_DEFINE_MATH_OPERATORS
#include "Widgets.h"

#include <algorithm>
#include <cmath>

#include "Theme.h"

namespace ui {

using namespace theme;

void beginCard(const char* id, float width) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Card);
    ImGui::PushStyleColor(ImGuiCol_Border, Border);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18, 16));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 8));
    ImGui::BeginChild(id, ImVec2(width, 0),
                      ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY |
                          ImGuiChildFlags_AlwaysUseWindowPadding);
}

void endCard() {
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

void beginPanel(const char* id, ImVec2 size, ImU32 bg) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg);
    ImGui::PushStyleColor(ImGuiCol_Border, Border);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18, 16));
    ImGui::BeginChild(id, size, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
}

void endPanel() {
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

void label(const char* text) {
    ImGui::PushFont(fonts.small);
    ImGui::PushStyleColor(ImGuiCol_Text, TextDim);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

void statBar(const char* name, const std::string& value, float fraction, ImU32 color) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    const float lineH = fonts.bold->FontSize;
    const float nameW = 48, valueW = 92;
    const float barW = std::max(20.0f, width - nameW - valueW - 12);

    ImGui::PushFont(fonts.small);
    dl->AddText(ImVec2(p.x, p.y + (lineH - fonts.small->FontSize) * 0.5f), TextDim, name);
    ImGui::PopFont();

    const float barY = p.y + lineH * 0.5f - 3;
    const ImVec2 b0(p.x + nameW, barY), b1(p.x + nameW + barW, barY + 6);
    dl->AddRectFilled(b0, b1, Track, 3);
    const float f = std::clamp(fraction, 0.0f, 1.0f);
    if (f > 0) dl->AddRectFilled(b0, ImVec2(b0.x + barW * f, b1.y), color, 3);

    ImGui::PushFont(fonts.bold);
    const ImVec2 ts = ImGui::CalcTextSize(value.c_str());
    dl->AddText(ImVec2(p.x + width - ts.x, p.y), Text, value.c_str());
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(width, lineH + 2));
}

void keyValue(const char* key, const std::string& value) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::PushFont(fonts.small);
    dl->AddText(ImVec2(p.x, p.y + 2), TextDim, key);
    ImGui::PopFont();
    dl->AddText(ImVec2(p.x + 48, p.y), Text, value.c_str());
    ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, fonts.body->FontSize));
}

void agePill(const std::string& text, bool active) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::PushFont(fonts.bold);
    const ImVec2 ts = ImGui::CalcTextSize(text.c_str());
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    const float h = ts.y + 8;
    const float padX = 14;
    const ImVec2 a(p.x + (w - ts.x) * 0.5f - padX, p.y);
    const ImVec2 b(a.x + ts.x + padX * 2, p.y + h);
    const float cy = p.y + h * 0.5f;

    dl->AddLine(ImVec2(p.x, cy), ImVec2(a.x - 10, cy), Border);
    dl->AddLine(ImVec2(b.x + 10, cy), ImVec2(p.x + w, cy), Border);
    if (active) {
        dl->AddRectFilled(a, b, Gold, h * 0.5f);
        dl->AddText(ImVec2(a.x + padX, p.y + 4), Bg, text.c_str());
    } else {
        dl->AddRectFilled(a, b, Card, h * 0.5f);
        dl->AddRect(a, b, Border, h * 0.5f);
        dl->AddText(ImVec2(a.x + padX, p.y + 4), Gold, text.c_str());
    }
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(w, h));
}

void narrative(const std::string& text, ImFont* font, ImU32 textColor, ImU32 barColor, float wrapWidth) {
    const float indent = 20;
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float startX = ImGui::GetCursorPosX();

    ImGui::PushFont(font);
    ImGui::PushStyleColor(ImGuiCol_Text, textColor);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, font->FontSize * 0.35f));  // 줄(문단) 간격
    ImGui::PushTextWrapPos(startX + wrapWidth);
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t nl = text.find('\n', pos);
        if (nl == std::string::npos) nl = text.size();
        ImGui::SetCursorPosX(startX + indent);
        ImGui::TextUnformatted(text.c_str() + pos, text.c_str() + nl);
        pos = nl + 1;
    }
    ImGui::PopTextWrapPos();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::PopFont();

    const float endY = ImGui::GetItemRectMax().y;
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(start.x, start.y - 2), ImVec2(start.x + 3, endY + 2), barColor);
}

bool choiceButton(const char* id, const std::string& text, float width) {
    ImGui::PushFont(fonts.body);
    const float wrap = width - 56;
    const ImVec2 ts = ImGui::CalcTextSize(text.c_str(), nullptr, false, wrap);
    const float h = std::max(48.0f, ts.y + 26);
    const ImVec2 p = ImGui::GetCursorScreenPos();

    const bool pressed = ImGui::InvisibleButton(id, ImVec2(width, h));
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 q(p.x + width, p.y + h);
    dl->AddRectFilled(p, q, held ? Border : (hovered ? CardHover : Card), 9);
    dl->AddRect(p, q, hovered ? Gold : Border, 9, 0, hovered ? 1.5f : 1.0f);
    dl->AddText(ImVec2(p.x + 20, p.y + (h - fonts.body->FontSize) * 0.5f), Gold, u8"›");
    dl->AddText(fonts.body, fonts.body->FontSize, ImVec2(p.x + 40, p.y + (h - ts.y) * 0.5f),
                hovered ? Text : rgb(210, 206, 196), text.c_str(), nullptr, wrap);
    ImGui::PopFont();
    if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    return pressed;
}

bool goldButton(const char* text, ImVec2 size) {
    ImGui::PushFont(fonts.bold);
    const ImVec2 ts = ImGui::CalcTextSize(text);
    if (size.x <= 0) size.x = ts.x + 48;
    if (size.y <= 0) size.y = ts.y + 26;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton(text, size);
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, p + size, hovered ? GoldHover : Gold, 9);
    dl->AddText(ImVec2(p.x + (size.x - ts.x) * 0.5f, p.y + (size.y - ts.y) * 0.5f), rgb(28, 22, 10), text);
    ImGui::PopFont();
    if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    return pressed;
}

bool ghostButton(const char* text) {
    ImGui::PushFont(fonts.small);
    const ImVec2 ts = ImGui::CalcTextSize(text);
    const ImVec2 size(ts.x + 28, 36);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton(text, size);
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, p + size, hovered ? CardHover : Card, 7);
    dl->AddRect(p, p + size, hovered ? Gold : Border, 7);
    dl->AddText(ImVec2(p.x + 14, p.y + (size.y - ts.y) * 0.5f), hovered ? Text : TextDim, text);
    ImGui::PopFont();
    if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    return pressed;
}

float segmentedWidth(const char* const* labels, int count) {
    ImGui::PushFont(fonts.small);
    float w = 0;
    for (int i = 0; i < count; ++i) w += ImGui::CalcTextSize(labels[i]).x + 28;
    ImGui::PopFont();
    return w + 2 * (count - 1);
}

bool segmented(const char* id, const char* const* labels, int count, int* current) {
    ImGui::PushID(id);
    ImGui::PushFont(fonts.small);
    bool changed = false;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    // SameLine은 줄의 처음 높이로 되돌아가므로, 위치를 직접 정해서 높이를 맞춘다
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    float x = origin.x;
    for (int i = 0; i < count; ++i) {
        if (i > 0) ImGui::SameLine();
        const ImVec2 ts = ImGui::CalcTextSize(labels[i]);
        const ImVec2 size(ts.x + 28, 36);
        const ImVec2 p(x, origin.y);
        ImGui::SetCursorScreenPos(p);
        x += size.x + 2;
        ImGui::PushID(i);
        if (ImGui::InvisibleButton("##seg", size) && *current != i) {
            *current = i;
            changed = true;
        }
        const bool hovered = ImGui::IsItemHovered();
        ImGui::PopID();
        const bool on = *current == i;
        dl->AddRectFilled(p, p + size, on ? Gold : (hovered ? CardHover : Card), 7);
        if (!on) dl->AddRect(p, p + size, Border, 7);
        dl->AddText(ImVec2(p.x + 14, p.y + (size.y - ts.y) * 0.5f), on ? rgb(28, 22, 10) : TextDim, labels[i]);
        if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
    ImGui::PopFont();
    ImGui::PopID();
    return changed;
}

void progressBar(float fraction, float width, float height, ImU32 color) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + width, p.y + height), Track, height * 0.5f);
    const float f = std::clamp(fraction, 0.0f, 1.0f);
    if (f > 0) dl->AddRectFilled(p, ImVec2(p.x + width * f, p.y + height), color, height * 0.5f);
    ImGui::Dummy(ImVec2(width, height));
}

std::string formatNumber(double v) {
    long long n = std::llround(v);
    const bool neg = n < 0;
    const std::string digits = std::to_string(neg ? -n : n);
    std::string out;
    for (size_t i = 0; i < digits.size(); ++i) {
        if (i > 0 && (digits.size() - i) % 3 == 0) out += ',';
        out += digits[i];
    }
    return neg ? "-" + out : out;
}

}  // namespace ui
