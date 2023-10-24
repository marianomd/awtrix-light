#ifndef AppS_H
#define AppS_H

#include <map>
#include "MatrixDisplayUi.h"
#include "effects.h"

struct CustomApp
{
    bool hasCustomColor = false;
    uint8_t currentFrame = 0;
    String iconName;
    String drawInstructions;
    float scrollposition = 0;
    int16_t scrollDelay = 0;
    byte lifetimeMode = 0;
    String text;
    uint32_t color;
    File icon;
    bool isGif;
    bool rainbow;
    bool center;
    int fade = 0;
    int blink = 0;
    int effect = -1;
    long duration = 0;
    byte textCase = 0;
    int16_t repeat = 0;
    int16_t currentRepeat = 0;
    String name;
    byte pushIcon = 0;
    float iconPosition = 0;
    bool iconWasPushed = false;
    int barData[16] = {0};
    int lineData[16] = {0};
    int gradient[2] = {0};
    int barSize;
    int lineSize;
    long lastUpdate;
    uint64_t lifetime;
    std::vector<uint32_t> colors;
    std::vector<String> fragments;
    int textOffset;
    int iconOffset;
    int progress = -1;
    uint32_t pColor;
    uint32_t background = 0;
    uint32_t pbColor;
    float scrollSpeed = 100;
    bool topText = true;
    bool noScrolling = true;
    bool lifeTimeEnd = false;
};

extern std::vector<std::pair<String, AppCallback>> Apps;
extern String currentCustomApp;
extern std::map<String, CustomApp> customApps;
extern void (*customAppCallbacks[20])(MatrixPanel_I2S_DMA *, MatrixDisplayUiState *, int16_t, int16_t, GifPlayer *);

CustomApp *getCustomAppByName(String name);

String getAppNameByFunction(AppCallback AppFunction);

String getAppNameAtIndex(int index);

int findAppIndexByName(const String &name);

const char *getTimeFormat();

void TimeApp(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

void DateApp(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

void TempApp(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

void HumApp(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

#ifndef awtrix2_upgrade
void BatApp(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
#endif

void ShowCustomApp(String name, MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

// Unattractive to have a function for every customapp wich does the same, but currently still no other option found TODO
void CApp1(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
void CApp2(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
void CApp3(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
void CApp4(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
void CApp5(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
void CApp6(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
void CApp7(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
void CApp8(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
void CApp9(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
void CApp10(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
void CApp11(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
void CApp12(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
void CApp13(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
void CApp14(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
void CApp15(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
void CApp16(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
void CApp17(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
void CApp18(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
void CApp19(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
void CApp20(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
#endif