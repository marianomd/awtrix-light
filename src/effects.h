#ifndef EFFECTS_H
#define EFFECTS_H

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <ArduinoJson.h>
#include "DisplayManager.h"

struct EffectSettings
{
    double speed;
    CRGBPalette16 palette;
    bool blend;
    EffectSettings(double s = 2, CRGBPalette16 p = OceanColors_p, bool b = false) : speed(s), palette(p), blend(b) {}
};

typedef void (*EffectFunc)(MatrixPanel_I2S_DMA *, int16_t, int16_t, EffectSettings *);

struct Effect
{
    String name;
    EffectFunc func;
    EffectSettings settings;
};

const int numOfEffects = 19;
extern Effect effects[];
void callEffect(MatrixPanel_I2S_DMA *matrix, int16_t x, int16_t y, u_int8_t index);
int getEffectIndex(String name);
void updateEffectSettings(u_int8_t index, String json);
#endif