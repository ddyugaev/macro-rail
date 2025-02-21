#pragma once

#include <Arduino.h>
#include "Globals.h"
#include "FastManualMode.h"

class FromPositionMode : public FastManualMode {
public:
  FromPositionMode() : FastManualMode(F("From position")) {};

protected:
  void onApply() override {
    long oldPosition = g_stepper.getPosition();
    g_settings.toPosition = g_settings.toPosition - oldPosition;
    g_stepper.resetPosition();
  };
};
