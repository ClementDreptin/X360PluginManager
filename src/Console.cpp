#include "pch.h"
#include "Console.h"

#include "UI.h"

Console::Console()
{
}

void Console::Info(const std::string &text)
{
    Print(Line(text, ImVec4(0.80f, 0.84f, 0.96f, 1.0f)));
}

void Console::Success(const std::string &text)
{
    Print(Line(text, ImVec4(0.65f, 0.89f, 0.63f, 1.0f)));
}

void Console::Warn(const std::string &text)
{
    Print(Line(text, ImVec4(0.98f, 0.87f, 0.67f, 1.0f)));
}

void Console::Error(const std::string &text)
{
    Print(Line(text, ImVec4(0.95f, 0.55f, 0.66f, 1.0f)));
}

void Console::Render()
{
    const ImVec2 &padding = ImGui::GetStyle().WindowPadding;
    ImVec2 windowPos(UI::SafeAreaOffsetX, UI::DisplayHeight / 2.0f + padding.y / 2.0f);
    ImVec2 windowSize(UI::SafeAreaWidth, UI::SafeAreaHeight / 2.0f - padding.y / 2.0f);

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove;
    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(windowSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
    ImGui::Begin("Logs", nullptr, windowFlags);

    for (size_t i = 0; i < m_Lines.size(); i++)
        ImGui::TextColored(m_Lines[i].Color, m_Lines[i].Text.c_str());

    ImGui::End();
    ImGui::PopStyleVar();
}

Console::Line::Line(const std::string &text, const ImVec4 &color)
    : Text(text), Color(color)
{
}

void Console::Print(const Line &line)
{
    m_Lines.emplace_back(line);
}
