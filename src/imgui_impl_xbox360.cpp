#include "pch.h"
#include "imgui_impl_xbox360.h"

struct ImGui_ImplXbox360_Data
{
    INT64 Time;
    INT64 TicksPerSecond;

    ImGui_ImplXbox360_Data() { memset(this, 0, sizeof(*this)); }
};

static ImGui_ImplXbox360_Data *ImGui_ImplXbox360_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplXbox360_Data *)ImGui::GetIO().BackendPlatformUserData : NULL;
}

bool ImGui_ImplXbox360_Init()
{
    ImGuiIO &io = ImGui::GetIO();
    IM_ASSERT(io.BackendPlatformUserData == NULL && "Already initialized a platform backend!");

    INT64 perf_frequency, perf_counter;
    if (!::QueryPerformanceFrequency((LARGE_INTEGER *)&perf_frequency))
        return false;
    if (!::QueryPerformanceCounter((LARGE_INTEGER *)&perf_counter))
        return false;

    // Setup backend capabilities flags
    ImGui_ImplXbox360_Data *bd = IM_NEW(ImGui_ImplXbox360_Data)();
    io.BackendPlatformUserData = (void *)bd;
    io.BackendPlatformName = "imgui_impl_xbox360";

    bd->TicksPerSecond = perf_frequency;
    bd->Time = perf_counter;

    // Keyboard mapping. Dear ImGui will use those indices to peek into the io.KeysDown[] array that we will update during the application lifetime.
    io.KeyMap[ImGuiKey_Tab] = VK_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
    io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
    io.KeyMap[ImGuiKey_Home] = VK_HOME;
    io.KeyMap[ImGuiKey_End] = VK_END;
    io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
    io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
    io.KeyMap[ImGuiKey_Space] = VK_SPACE;
    io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = VK_RETURN;
    io.KeyMap[ImGuiKey_A] = 'A';
    io.KeyMap[ImGuiKey_C] = 'C';
    io.KeyMap[ImGuiKey_V] = 'V';
    io.KeyMap[ImGuiKey_X] = 'X';
    io.KeyMap[ImGuiKey_Y] = 'Y';
    io.KeyMap[ImGuiKey_Z] = 'Z';

    return true;
}

static void ImGui_ImplXbox360_UpdateGamepads()
{
    ImGuiIO &io = ImGui::GetIO();
    memset(io.NavInputs, 0, sizeof(io.NavInputs));
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
        return;

    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    XINPUT_STATE xinput_state;
    if (XInputGetState(0, &xinput_state) == ERROR_SUCCESS)
    {
        const XINPUT_GAMEPAD &gamepad = xinput_state.Gamepad;
        io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

#define MAP_BUTTON(NAV_NO, BUTTON_ENUM) \
    { \
        io.NavInputs[NAV_NO] = (gamepad.wButtons & BUTTON_ENUM) ? 1.0f : 0.0f; \
    }
#define MAP_ANALOG(NAV_NO, VALUE, V0, V1) \
    { \
        float vn = (float)(VALUE - V0) / (float)(V1 - V0); \
        if (vn > 1.0f) \
            vn = 1.0f; \
        if (vn > 0.0f && io.NavInputs[NAV_NO] < vn) \
            io.NavInputs[NAV_NO] = vn; \
    }
        MAP_BUTTON(ImGuiNavInput_Activate, XINPUT_GAMEPAD_A);               // Cross / A
        MAP_BUTTON(ImGuiNavInput_Cancel, XINPUT_GAMEPAD_B);                 // Circle / B
        MAP_BUTTON(ImGuiNavInput_Menu, XINPUT_GAMEPAD_X);                   // Square / X
        MAP_BUTTON(ImGuiNavInput_Input, XINPUT_GAMEPAD_Y);                  // Triangle / Y
        MAP_BUTTON(ImGuiNavInput_DpadLeft, XINPUT_GAMEPAD_DPAD_LEFT);       // D-Pad Left
        MAP_BUTTON(ImGuiNavInput_DpadRight, XINPUT_GAMEPAD_DPAD_RIGHT);     // D-Pad Right
        MAP_BUTTON(ImGuiNavInput_DpadUp, XINPUT_GAMEPAD_DPAD_UP);           // D-Pad Up
        MAP_BUTTON(ImGuiNavInput_DpadDown, XINPUT_GAMEPAD_DPAD_DOWN);       // D-Pad Down
        MAP_BUTTON(ImGuiNavInput_FocusPrev, XINPUT_GAMEPAD_LEFT_SHOULDER);  // L1 / LB
        MAP_BUTTON(ImGuiNavInput_FocusNext, XINPUT_GAMEPAD_RIGHT_SHOULDER); // R1 / RB
        MAP_BUTTON(ImGuiNavInput_TweakSlow, XINPUT_GAMEPAD_LEFT_SHOULDER);  // L1 / LB
        MAP_BUTTON(ImGuiNavInput_TweakFast, XINPUT_GAMEPAD_RIGHT_SHOULDER); // R1 / RB
        MAP_ANALOG(ImGuiNavInput_LStickLeft, gamepad.sThumbLX, -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, -32768);
        MAP_ANALOG(ImGuiNavInput_LStickRight, gamepad.sThumbLX, +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, +32767);
        MAP_ANALOG(ImGuiNavInput_LStickUp, gamepad.sThumbLY, +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, +32767);
        MAP_ANALOG(ImGuiNavInput_LStickDown, gamepad.sThumbLY, -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, -32767);
#undef MAP_BUTTON
#undef MAP_ANALOG
    }
}

static void ImGui_ImplXbox360_UpdateKeyboard()
{
    ImGuiIO &io = ImGui::GetIO();

    // Get the keyboard state
    XINPUT_KEYSTROKE keystroke = {};
    DWORD result = XInputGetKeystroke(XUSER_INDEX_ANY, XINPUT_FLAG_KEYBOARD, &keystroke);
    if (result != ERROR_SUCCESS)
        return;

    // Copy the key state into io.KeysDown
    bool down = keystroke.Flags & XINPUT_KEYSTROKE_KEYDOWN;
    if (keystroke.VirtualKey < 256)
        io.KeysDown[keystroke.VirtualKey] = down;

    // Modifiers
    io.KeyCtrl = (keystroke.Flags & XINPUT_KEYSTROKE_CTRL) != 0;
    io.KeyShift = (keystroke.Flags & XINPUT_KEYSTROKE_SHIFT) != 0;
    io.KeyAlt = (keystroke.Flags & XINPUT_KEYSTROKE_ALT) != 0;

    // If the current key is a character, add it for the text inputs
    if (keystroke.Flags & XINPUT_KEYSTROKE_VALIDUNICODE)
        io.AddInputCharacterUTF16(keystroke.Unicode);
}

void ImGui_ImplXbox360_NewFrame()
{
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplXbox360_Data *bd = ImGui_ImplXbox360_GetBackendData();
    IM_ASSERT(bd != NULL && "Did you call ImGui_ImplXbox360_Init()?");

    // Definition is always 720p on Xbox 360, other definitions are created by the
    // hardware scaler
    io.DisplaySize = ImVec2(1280.0f, 720.0f);

    // Setup time step
    INT64 current_time = 0;
    ::QueryPerformanceCounter((LARGE_INTEGER *)&current_time);
    io.DeltaTime = (float)(current_time - bd->Time) / bd->TicksPerSecond;
    bd->Time = current_time;

    // Update game controllers (if enabled and available)
    ImGui_ImplXbox360_UpdateGamepads();

    // Update the keyboard (if enabled and available)
    ImGui_ImplXbox360_UpdateKeyboard();
}
