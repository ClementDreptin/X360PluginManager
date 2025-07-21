#pragma once

namespace UI
{

extern const float DisplayWidth;
extern const float DisplayHeight;
extern const float SafeAreaPercentage;
extern const float SafeAreaWidth;
extern const float SafeAreaHeight;
extern const float SafeAreaOffsetX;
extern const float SafeAreaOffsetY;

HRESULT Init();

void BeginFrame();

void EndFrame();

}
