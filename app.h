#pragma once

#include "imgui.h"

const int BASE_ANGLE = 90;
const int MIN_ANGLE = 80;
const int MAX_ANGLE = 100;

namespace App {
  void RenderUI () {
    ImGui::Begin ("Variables");

    static int angle = BASE_ANGLE;
    ImGui::Button("Start");
    ImGui::SliderInt("Angle of Attack", &angle, MIN_ANGLE, MAX_ANGLE);

    ImGui::End();
  }
}
