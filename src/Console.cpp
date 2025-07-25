#include "pch.h"
#include "Console.h"

#include "UI.h"

Console::Console()
{
    InitializeCriticalSection(&m_LinesCriticalSection);
}

Console::~Console()
{
    DeleteCriticalSection(&m_LinesCriticalSection);
}

void Console::Info(const std::string &text)
{
    Print(Line(text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
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
    // Place the window on the right half of the screen
    const ImVec2 &padding = ImGui::GetStyle().WindowPadding;
    ImVec2 windowPos(UI::SafeAreaOffsetX + UI::SafeAreaWidth / 2.0f + padding.x / 2.0f, UI::SafeAreaOffsetY);
    ImVec2 windowSize(UI::SafeAreaWidth / 2.0f - padding.x / 2.0f, UI::SafeAreaHeight);
    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(windowSize);

    // Create the window
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize;
    ImGui::Begin("Logs", nullptr, windowFlags);

    // Lock the lines before rendering them, this is because the print functions can be
    // called from other threads
    EnterCriticalSection(&m_LinesCriticalSection);
    for (size_t i = 0; i < m_Lines.size(); i++)
        ImGui::TextColored(m_Lines[i].Color, m_Lines[i].Text.c_str());
    LeaveCriticalSection(&m_LinesCriticalSection);

    ImGui::End();
}

Console::Line::Line(const std::string &text, const ImVec4 &color)
    : Text(text), Color(color)
{
}

void Console::Print(const Line &line)
{
    // Lock the lines before mutating them, this is because the print functions can be
    // called from other threads
    EnterCriticalSection(&m_LinesCriticalSection);
    m_Lines.emplace_back(line);
    LeaveCriticalSection(&m_LinesCriticalSection);
}
