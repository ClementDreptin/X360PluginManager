#pragma once

class Console
{
public:
    Console();

    void Info(const std::string &text);

    void Success(const std::string &text);

    void Warn(const std::string &text);

    void Error(const std::string &text);

    void Render();

private:
    struct Line
    {
        Line(const std::string &text, const ImVec4 &color);

        std::string Text;
        ImVec4 Color;
    };

    std::vector<Line> m_Lines;

    void Print(const Line &line);
};

extern Console g_Console;
