#pragma once

class Console
{
public:
    Console();

    HRESULT Create();

    void Info(const std::string &text);

    void Success(const std::string &text);

    void Warn(const std::string &text);

    void Error(const std::string &text);

    void Update();

    void Render();

private:
    struct Line
    {
        Line(const std::wstring &text, D3DCOLOR color);

        std::wstring Text;
        D3DCOLOR Color;
    };

    static const uint32_t s_DisplayWidth = 1280;
    static const uint32_t s_DisplayHeight = 720;
    static const uint32_t s_SafeAreaPercentage = 90;
    static const uint32_t s_SafeAreaWidth = s_DisplayWidth * s_SafeAreaPercentage / 100;
    static const uint32_t s_SafeAreaHeight = s_DisplayHeight * s_SafeAreaPercentage / 100;
    static const uint32_t s_SafeAreaOffsetX = (s_DisplayWidth - s_SafeAreaWidth) / 2;
    static const uint32_t s_SafeAreaOffsetY = (s_DisplayHeight - s_SafeAreaHeight) / 2;

    static D3DPRESENT_PARAMETERS s_d3dpp;
    static ATG::D3DDevice *s_pd3dDevice;

    ATG::Font m_Font;

    float m_LineHeight;
    size_t m_MaxLinesToDisplay;
    uint32_t m_FirstLineIndex;
    std::vector<Line> m_Lines;

    void Print(const Line &line);
};

extern Console g_Console;
