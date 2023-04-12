#include <DisplayManager.h>
#include <FastLED_NeoMatrix.h>
#include "MatrixDisplayUi.h"
#include <TJpg_Decoder.h>
#include "icons.h"
#include "Globals.h"
#include <ArduinoJson.h>
#include "PeripheryManager.h"
#include "MQTTManager.h"
#include "GifPlayer.h"
#include <Ticker.h>
#include "Functions.h"
#include "ServerManager.h"
#include "MenuManager.h"
#include "Apps.h"
#include "Dictionary.h"
#include <set>
#include "GifPlayer.h"

Ticker AlarmTicker;
Ticker TimerTicker;

#ifdef ULANZI
#define MATRIX_PIN 32
#else
#define MATRIX_PIN D2
#endif

#define MATRIX_WIDTH 32
#define MATRIX_HEIGHT 8
fs::File gifFile;
GifPlayer gif;

uint16_t gifX, gifY;
CRGB leds[MATRIX_WIDTH * MATRIX_HEIGHT];

FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(leds, 8, 8, 4, 1, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE);
MatrixDisplayUi *ui = new MatrixDisplayUi(matrix);

uint8_t lastBrightness;

DisplayManager_ &DisplayManager_::getInstance()
{
    static DisplayManager_ instance;
    return instance;
}

DisplayManager_ &DisplayManager = DisplayManager.getInstance();

void DisplayManager_::setBrightness(int bri)
{
    if (MATRIX_OFF && !ALARM_ACTIVE)
    {
        matrix->setBrightness(0);
    }
    else
    {
        matrix->setBrightness(bri);
    }
}

void DisplayManager_::setFPS(uint8_t fps)
{
    ui->setTargetFPS(fps);
}

void DisplayManager_::setTextColor(uint16_t color)
{
    matrix->setTextColor(color);
}

bool DisplayManager_::setAutoTransition(bool active)
{
    if (ui->AppCount < 2)
    {
        ui->disablesetAutoTransition();
        return false;
    }
    if (active && AUTO_TRANSITION)
    {
        ui->enablesetAutoTransition();
        return true;
    }
    else
    {
        ui->disablesetAutoTransition();
        return false;
    }
    showGif = false;
}

void DisplayManager_::drawJPG(uint16_t x, uint16_t y, fs::File jpgFile)
{
    TJpgDec.drawFsJpg(x, y, jpgFile);
}

void DisplayManager_::drawBMP(int16_t x, int16_t y, const uint16_t bitmap[], int16_t w, int16_t h)
{
    matrix->drawRGBBitmap(y, x, bitmap, w, h);
}

void DisplayManager_::applyAllSettings()
{
    ui->setTargetFPS(MATRIX_FPS);
    ui->setTimePerApp(TIME_PER_APP);
    ui->setTimePerTransition(TIME_PER_TRANSITION);

    setBrightness(BRIGHTNESS);
    setTextColor(TEXTCOLOR_565);
    setAutoTransition(AUTO_TRANSITION);
}

void DisplayManager_::resetTextColor()
{
    matrix->setTextColor(TEXTCOLOR_565);
}

void DisplayManager_::clearMatrix()
{
    matrix->clear();
    matrix->show();
}

bool jpg_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
    uint16_t bitmapIndex = 0;
    for (uint16_t row = 0; row < h; row++)
    {
        for (uint16_t col = 0; col < w; col++)
        {
            matrix->drawPixel(x + col, y + row, bitmap[bitmapIndex++]);
        }
    }
    return 0;
}

void DisplayManager_::printText(int16_t x, int16_t y, const char *text, bool centered, byte textCase)
{

    if (centered)
    {
        uint16_t textWidth = getTextWidth(text, textCase);
        int16_t textX = ((32 - textWidth) / 2);
        matrix->setCursor(textX, y);
    }
    else
    {
        matrix->setCursor(x, y);
    }

    if ((UPPERCASE_LETTERS && textCase == 0) || textCase == 1)
    {
        size_t length = strlen(text);
        char upperText[length + 1]; // +1 for the null terminator

        for (size_t i = 0; i < length; ++i)
        {
            upperText[i] = toupper(text[i]);
        }

        upperText[length] = '\0'; // Null terminator
        matrix->print(upperText);
    }
    else
    {
        matrix->print(text);
    }
}

void DisplayManager_::HSVtext(int16_t x, int16_t y, const char *text, bool clear, byte textCase)
{
    if (clear)
        matrix->clear();
    static uint8_t hueOffset = 0;
    uint16_t xpos = 0;
    for (uint16_t i = 0; i < strlen(text); i++)
    {
        uint8_t hue = map(i, 0, strlen(text), 0, 255) + hueOffset;
        uint32_t textColor = hsvToRgb(hue, 255, 255);
        matrix->setTextColor(textColor);
        const char *myChar = &text[i];

        matrix->setCursor(xpos + x, y);
        if ((UPPERCASE_LETTERS && textCase == 0) || textCase == 1)
        {
            matrix->print((char)toupper(text[i]));
        }
        else
        {
            matrix->print(&text[i]);
        }
        char temp_str[2] = {'\0', '\0'};
        temp_str[0] = text[i];
        xpos += getTextWidth(temp_str, textCase);
    }
    hueOffset++;
    if (clear)
        matrix->show();
}

void pushCustomApp(String name, int position)
{
    if (customApps.count(name) == 0)
    {
        ++customPagesCount;
        void (*customApps[20])(FastLED_NeoMatrix *, MatrixDisplayUiState *, int16_t, int16_t, bool, bool, GifPlayer *) = {CApp1, CApp2, CApp3, CApp4, CApp5, CApp6, CApp7, CApp8, CApp9, CApp10, CApp11, CApp12, CApp13, CApp14, CApp15, CApp16, CApp17, CApp18, CApp19, CApp20};

        if (position < 0) // Insert at the end of the vector
        {
            Apps.push_back(std::make_pair(name, customApps[customPagesCount]));
        }
        else if (position < Apps.size()) // Insert at a specific position
        {
            Apps.insert(Apps.begin() + position, std::make_pair(name, customApps[customPagesCount]));
        }
        else // Invalid position, Insert at the end of the vector
        {
            Apps.push_back(std::make_pair(name, customApps[customPagesCount]));
        }

        ui->setApps(Apps); // Add Apps
        DisplayManager.getInstance().setAutoTransition(true);
    }
}

void removeCustomAppFromApps(const String &name, bool setApps)
{
    auto it = std::find_if(Apps.begin(), Apps.end(), [&name](const std::pair<String, AppCallback> &appPair)
                           { return appPair.first == name; });

    Apps.erase(it);
    customApps.erase(customApps.find(name));
    if (setApps)
        ui->setApps(Apps);
}

bool parseFragmentsText(const String &jsonText, std::vector<uint16_t> &colors, std::vector<String> &fragments, uint16_t standardColor)
{
    colors.clear();
    fragments.clear();

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, jsonText);

    if (error)
    {
        return false;
    }

    JsonArray fragmentArray = doc.as<JsonArray>();

    for (JsonObject fragmentObj : fragmentArray)
    {
        String textFragment = fragmentObj["t"].as<String>();
        uint16_t color;
        if (fragmentObj.containsKey("c"))
        {
            color = hexToRgb565(fragmentObj["c"].as<String>());
        }
        else
        {
            color = standardColor;
        }

        fragments.push_back(textFragment);
        colors.push_back(color);
    }
    return true;
}

void DisplayManager_::generateCustomPage(const String &name, const char *json)
{
    if (strcmp(json, "") == 0 && customApps.count(name))
    {
        removeCustomAppFromApps(name, true);
        showGif = false;
        return;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, json);
    if (error)
    {
        showGif = false;
        doc.clear();
        return;
    }

    CustomApp customApp;

    if (customApps.find(name) != customApps.end())
    {
        customApp = customApps[name];
    }

    if (doc.containsKey("sound"))
    {
#ifdef ULANZI
        customApp.sound = ("/" + doc["sound"].as<String>() + ".txt");
#else
        customApp.sound = doc["sound"].as<String>();
#endif
    }
    else
    {
        customApp.sound = "";
    }

    if (doc.containsKey("bar"))
    {
        JsonArray barData = doc["bar"];
        int i = 0;
        for (JsonVariant v : barData)
        {
            if (i >= 16)
            {
                break;
            }
            customApp.barData[i] = v.as<int>();
            i++;
        }
        customApp.barSize = i;
    }
    else
    {
        customApp.barSize = 0;
    }

    customApp.duration = doc.containsKey("duration") ? doc["duration"].as<int>() * 1000 : 0;
    int pos = doc.containsKey("pos") ? doc["pos"].as<uint8_t>() : -1;
    customApp.rainbow = doc.containsKey("rainbow") ? doc["rainbow"] : false;
    customApp.pushIcon = doc.containsKey("pushIcon") ? doc["pushIcon"] : 0;
    customApp.textCase = doc.containsKey("textCase") ? doc["textCase"] : 0;
    customApp.lifetime = doc.containsKey("lifetime") ? doc["lifetime"] : 0;
    customApp.textOffset = doc.containsKey("textOffset") ? doc["textOffset"] : 0;
    customApp.name = name;

    customApp.lastUpdate = millis();

    if (doc.containsKey("color"))
    {
        auto color = doc["color"];
        if (color.is<String>())
        {
            customApp.color = hexToRgb565(color.as<String>());
        }
        else if (color.is<JsonArray>() && color.size() == 3)
        {
            uint8_t r = color[0];
            uint8_t g = color[1];
            uint8_t b = color[2];
            customApp.color = (r << 11) | (g << 5) | b;
        }
        else
        {
            customApp.color = TEXTCOLOR_565;
        }
    }
    else
    {
        customApp.color = TEXTCOLOR_565;
    }

    if (doc.containsKey("text"))
    {
        String text = utf8ascii(doc["text"].as<String>());
        parseFragmentsText(text, customApp.colors, customApp.fragments, customApp.color);
        customApp.text = text;
    }
    else
    {
        customApp.text = "";
    }

    Serial.println(customApp.text);

    if (currentCustomApp != name)
    {
        customApp.scrollposition = 9 + customApp.textOffset;
    }

    customApp.repeat = doc.containsKey("repeat") ? doc["repeat"].as<uint8_t>() : -1;

    if (doc.containsKey("icon"))
    {
        String iconFileName = String(doc["icon"].as<String>());
        if (customApp.icon && String(customApp.icon.name()).startsWith(iconFileName))
        {
        }
        else
        {
            if (customApp.icon)
            {
                customApp.icon.close();
            }
            if (LittleFS.exists("/ICONS/" + iconFileName + ".jpg"))
            {
                customApp.isGif = false;
                customApp.icon = LittleFS.open("/ICONS/" + iconFileName + ".jpg");
            }
            else if (LittleFS.exists("/ICONS/" + iconFileName + ".gif"))
            {
                customApp.isGif = true;
                customApp.icon = LittleFS.open("/ICONS/" + iconFileName + ".gif");
            }
            else
            {
                fs::File nullPointer;
                customApp.icon = nullPointer;
            }
        }
    }
    else
    {
        fs::File nullPointer;
        customApp.icon = nullPointer;
    }

    pushCustomApp(name, pos - 1);
    customApps[name] = customApp;
    doc.clear();
}

void DisplayManager_::generateNotification(const char *json)
{
    StaticJsonDocument<1024> doc;
    deserializeJson(doc, json);
    notify.duration = doc.containsKey("duration") ? doc["duration"].as<int>() * 1000 : TIME_PER_APP;
    notify.repeat = doc.containsKey("repeat") ? doc["repeat"].as<uint16_t>() : -1;
    notify.rainbow = doc.containsKey("rainbow") ? doc["rainbow"].as<bool>() : false;
    notify.hold = doc.containsKey("hold") ? doc["hold"].as<bool>() : false;
    notify.pushIcon = doc.containsKey("pushIcon") ? doc["pushIcon"] : 0;
    notify.textCase = doc.containsKey("textCase") ? doc["textCase"] : 0;
    notify.textOffset = doc.containsKey("textOffset") ? doc["textOffset"] : 0;
    notify.startime = millis();
    notify.scrollposition = 9 + notify.textOffset;
    notify.iconWasPushed = false;
    notify.iconPosition = 0;
    notify.scrollDelay = 0;
    if (doc.containsKey("sound"))
    {
#ifdef ULANZI
        PeripheryManager.playFromFile("/MELODIES/" + doc["sound"].as<String>() + ".txt");
#else
        PeripheryManager.playFromFile(doc["sound"].as<String>());
#endif
    }

    if (doc.containsKey("bar"))
    {
        JsonArray barData = doc["bar"];
        int i = 0;
        for (JsonVariant v : barData)
        {
            if (i >= 16)
            {
                break;
            }
            notify.barData[i] = v.as<int>();
            i++;
        }
        notify.barSize = i;
    }
    else
    {
        notify.barSize = 0;
    }

    if (doc.containsKey("color"))
    {
        auto color = doc["color"];
        if (color.is<String>())
        {
            notify.color = hexToRgb565(color.as<String>());
        }
        else if (color.is<JsonArray>() && color.size() == 3)
        {
            uint8_t r = color[0];
            uint8_t g = color[1];
            uint8_t b = color[2];
            notify.color = (r << 11) | (g << 5) | b;
        }
        else
        {
            notify.color = TEXTCOLOR_565;
        }
    }
    else
    {
        notify.color = TEXTCOLOR_565;
    }

    if (doc.containsKey("text"))
    {
        String text = utf8ascii(doc["text"].as<String>());
        parseFragmentsText(text, notify.colors, notify.fragments, notify.color);
        notify.text = text;
    }
    else
    {
        notify.text = "";
    }

    notify.flag = true;
    showGif = false;

    if (doc.containsKey("icon"))
    {
        String iconFileName = doc["icon"].as<String>();
        if (LittleFS.exists("/ICONS/" + iconFileName + ".jpg"))
        {
            notify.isGif = false;
            notify.icon = LittleFS.open("/ICONS/" + iconFileName + ".jpg");
            return;
        }
        else if (LittleFS.exists("/ICONS/" + iconFileName + ".gif"))
        {
            notify.isGif = true;
            notify.icon = LittleFS.open("/ICONS/" + iconFileName + ".gif");
            return;
        }
        else
        {
            fs::File nullPointer;
            notify.icon = nullPointer;
        }
    }
    else
    {
        fs::File nullPointer;
        notify.icon = nullPointer;
    }
}

void DisplayManager_::loadNativeApps()
{
    // Define a helper function to check and update an app
    auto updateApp = [&](const String &name, AppCallback callback, bool show, size_t position)
    {
        auto it = std::find_if(Apps.begin(), Apps.end(), [&](const std::pair<String, AppCallback> &app)
                               { return app.first == name; });
        if (it != Apps.end())
        {
            if (!show)
            {
                Apps.erase(it);
            }
        }
        else
        {
            if (show)
            {
                if (position >= Apps.size())
                {
                    Apps.push_back(std::make_pair(name, callback));
                }
                else
                {
                    Apps.insert(Apps.begin() + position, std::make_pair(name, callback));
                }
            }
        }
    };

    // Update the "time" app at position 0
    updateApp("time", TimeApp, SHOW_TIME, 0);

    // Update the "date" app at position 1
    updateApp("date", DateApp, SHOW_DATE, 1);

    // Update the "temp" app at position 2
    updateApp("temp", TempApp, SHOW_TEMP, 2);

    // Update the "hum" app at position 3
    updateApp("hum", HumApp, SHOW_HUM, 3);

#ifdef ULANZI
    // Update the "bat" app at position 4
    updateApp("bat", BatApp, SHOW_BAT, 4);
#endif

    ui->setApps(Apps);

    setAutoTransition(true);
}

void DisplayManager_::setup()
{
    TJpgDec.setCallback(jpg_output);
    TJpgDec.setJpgScale(1);

    FastLED.addLeds<NEOPIXEL, MATRIX_PIN>(leds, MATRIX_WIDTH * MATRIX_HEIGHT);
    setMatrixLayout(MATRIX_LAYOUT);
    if (COLOR_CORRECTION)
    {
        FastLED.setCorrection(COLOR_CORRECTION);
    }
    if (COLOR_TEMPERATURE)
    {
        FastLED.setTemperature(COLOR_TEMPERATURE);
    }
    gif.setMatrix(matrix);
    ui->setAppAnimation(SLIDE_DOWN);
    ui->setTimePerApp(TIME_PER_APP);
    ui->setTimePerTransition(TIME_PER_TRANSITION);
    ui->setTargetFPS(MATRIX_FPS);
    ui->setOverlays(overlays, 4);
    setAutoTransition(AUTO_TRANSITION);
    ui->init();
}

void checkLifetime()
{
    // Sammeln Sie die Namen der zu entfernenden Apps
    std::vector<String> appsToRemove;

    for (auto it = customApps.begin(); it != customApps.end(); ++it)
    {
        CustomApp &app = it->second;
        if (app.lifetime > 0 && (millis() - app.lastUpdate) / 1000 >= app.lifetime)
        {
            String appName = it->first;
            appsToRemove.push_back(appName);
        }
    }

    // Entfernen Sie die gesammelten Apps nach der Schleife
    for (const String &appName : appsToRemove)
    {
        removeCustomAppFromApps(appName, false);
    }
    if (appsToRemove.size() > 0)
        ui->setApps(Apps);
}

void DisplayManager_::tick()
{
    if (AP_MODE)
    {
        HSVtext(2, 6, "AP MODE", true, 1);
    }
    else
    {
        ui->update();
        if (ui->getUiState()->appState == IN_TRANSITION && !appIsSwitching)
        {
            appIsSwitching = true;
            checkLifetime();
        }
        else if (ui->getUiState()->appState == FIXED && appIsSwitching)
        {
            appIsSwitching = false;
            MQTTManager.setCurrentApp(CURRENT_APP);
            setAppTime(TIME_PER_APP);
        }
    }
}

void DisplayManager_::clear()
{
    matrix->clear();
}

void DisplayManager_::show()
{
    matrix->show();
}

void DisplayManager_::leftButton()
{
    if (!MenuManager.inMenu)
        ui->previousApp();
}

void DisplayManager_::rightButton()
{
    if (!MenuManager.inMenu)
        ui->nextApp();
}

void DisplayManager_::nextApp()
{
    if (!MenuManager.inMenu)
        ui->nextApp();
}

void DisplayManager_::previousApp()
{
    if (!MenuManager.inMenu)
        ui->previousApp();
}

void snozzeTimerCallback()
{
    ALARM_ACTIVE = true;
    AlarmTicker.detach();
}

void DisplayManager_::selectButton()
{
    if (!MenuManager.inMenu)
    {
        if (notify.flag && notify.hold)
        {
            DisplayManager.getInstance().dismissNotify();
        }
        if (ALARM_ACTIVE && SNOOZE_TIME > 0)
        {
            PeripheryManager.stopSound();
            ALARM_ACTIVE = false;
            AlarmTicker.once(SNOOZE_TIME * 60, snozzeTimerCallback);
        }

        if (TIMER_ACTIVE)
        {
            PeripheryManager.stopSound();
            TIMER_ACTIVE = false;
        }
    }
}

void DisplayManager_::selectButtonLong()
{
    if (ALARM_ACTIVE)
    {
        PeripheryManager.stopSound();
        ALARM_ACTIVE = false;
    }
}

void DisplayManager_::dismissNotify()
{
    notify.hold = false;
    notify.flag = false;
    showGif = false;
}

void timerCallback()
{
    TIMER_ACTIVE = true;
    TimerTicker.detach();
}

void DisplayManager_::gererateTimer(String Payload)
{
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, Payload);
    if (error)
        return;
    int hours = doc["hours"] | 0;
    int minutes = doc["minutes"] | 0;
    int seconds = doc["seconds"] | 0;
    TIMER_SOUND = doc.containsKey("sound") ? doc["sound"].as<String>() : "";
    time_t now = time(nullptr);
    struct tm futureTimeinfo = *localtime(&now);
    futureTimeinfo.tm_hour += hours;
    futureTimeinfo.tm_min += minutes;
    futureTimeinfo.tm_sec += seconds;
    time_t futureTime = mktime(&futureTimeinfo);
    int interval = difftime(futureTime, now) * 1000;
    TimerTicker.once_ms(interval, timerCallback);
}

void DisplayManager_::switchToApp(const char *json)
{
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, json);
    if (error)
        return;
    String name = doc["name"].as<String>();
    bool fast = doc["fast"] | false;
    int index = findAppIndexByName(name);
    if (index > -1)
        ui->transitionToApp(index);
}

void DisplayManager_::setNewSettings(const char *json)
{
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, json);
    if (error)
        return;

    if (doc.containsKey("textcolor"))
    {
        auto color = doc["textcolor"];
        if (color.is<String>())
        {
            TEXTCOLOR_565 = hexToRgb565(color.as<String>());
        }
        else if (color.is<JsonArray>() && color.size() == 3)
        {
            uint8_t r = color[0];
            uint8_t g = color[1];
            uint8_t b = color[2];
            TEXTCOLOR_565 = (r << 11) | (g << 5) | b;
        }
    }

    TIME_PER_APP = doc.containsKey("apptime") ? doc["apptime"] : TIME_PER_APP;
    TIME_PER_TRANSITION = doc.containsKey("transition") ? doc["transition"] : TIME_PER_TRANSITION;
    MATRIX_FPS = doc.containsKey("fps") ? doc["fps"] : MATRIX_FPS;
    BRIGHTNESS = doc.containsKey("brightness") ? doc["brightness"] : BRIGHTNESS;
    TIME_FORMAT = doc.containsKey("timeformat") ? doc["timeformat"].as<String>() : TIME_FORMAT;
    GAMMA = doc.containsKey("gamma") ? doc["gamma"].as<float>() : GAMMA;
    DATE_FORMAT = doc.containsKey("dateformat") ? doc["dateformat"].as<String>() : DATE_FORMAT;
    AUTO_BRIGHTNESS = doc.containsKey("auto_brightness") ? doc["autobrightness"] : AUTO_BRIGHTNESS;
    AUTO_TRANSITION = doc.containsKey("auto_transition") ? doc["autotransition"] : AUTO_TRANSITION;
    UPPERCASE_LETTERS = doc.containsKey("uppercase") ? doc["uppercase"] : UPPERCASE_LETTERS;
    if (doc.containsKey("color_correction"))
    {
        auto color = doc["color_correction"];
        if (color.is<String>())
        {
            COLOR_CORRECTION = hexToRgb565(color.as<String>());
        }
        else if (color.is<JsonArray>() && color.size() == 3)
        {
            uint8_t r = color[0];
            uint8_t g = color[1];
            uint8_t b = color[2];
            COLOR_CORRECTION = (r << 11) | (g << 5) | b;
        }
        if (COLOR_CORRECTION)
        {
            FastLED.setCorrection(COLOR_CORRECTION);
        }
    }
    if (doc.containsKey("color_temperature"))
    {
        auto temperature = doc["color_temperature"];
        if (temperature.is<String>())
        {
            COLOR_CORRECTION = hexToRgb565(temperature.as<String>());
        }
        else if (temperature.is<JsonArray>() && temperature.size() == 3)
        {
            uint8_t r = temperature[0];
            uint8_t g = temperature[1];
            uint8_t b = temperature[2];
            COLOR_TEMPERATURE = (r << 11) | (g << 5) | b;
        }
        if (COLOR_TEMPERATURE)
        {
            FastLED.setTemperature(COLOR_TEMPERATURE);
        }
    }
    applyAllSettings();
    saveSettings();
}

void DisplayManager_::drawProgressBar(int cur, int total)
{
    matrix->clear();
    int progress = (cur * 100) / total;
    char progressStr[5];
    snprintf(progressStr, 5, "%d%%", progress);
    printText(0, 6, progressStr, true, false);
    int leds_for_progress = (progress * MATRIX_WIDTH * MATRIX_HEIGHT) / 100;
    matrix->drawFastHLine(0, 7, MATRIX_WIDTH, matrix->Color(100, 100, 100));
    matrix->drawFastHLine(0, 7, leds_for_progress / MATRIX_HEIGHT, matrix->Color(0, 255, 0));
    matrix->show();
}

void DisplayManager_::drawMenuIndicator(int cur, int total, uint16_t color)
{
    int menuItemWidth = 1;
    int totalWidth = total * menuItemWidth + (total - 1);
    int leftMargin = (MATRIX_WIDTH - totalWidth) / 2;
    int pixelSpacing = 1;
    for (int i = 0; i < total; i++)
    {
        int x = leftMargin + i * (menuItemWidth + pixelSpacing);
        if (i == cur)
        {
            matrix->drawLine(x, 7, x + menuItemWidth - 1, 7, color);
        }
        else
        {
            matrix->drawLine(x, 7, x + menuItemWidth - 1, 7, matrix->Color(100, 100, 100));
        }
    }
}

void DisplayManager_::drawBarChart(int16_t x, int16_t y, const int data[], byte dataSize, bool withIcon, uint16_t color)
{
    int maximum = 0;
    int newData[dataSize];

    for (int i = 0; i < dataSize; i++)
    {
        int d = data[i];
        if (d > maximum)
        {
            maximum = d;
        }
    }

    for (int i = 0; i < dataSize; i++)
    {
        int d = data[i];
        newData[i] = map(d, 0, maximum, 0, 8);
    }

    int barWidth;
    if (withIcon)
    {
        barWidth = ((32 - 9) / (dataSize)-1);
    }
    else
    {
        barWidth = (32 / (dataSize)-1);
    }

    int startX = withIcon ? 9 : 0;

    for (int i = 0; i < dataSize; i++)
    {
        int x1 = x + startX + (barWidth + 1) * i;
        int barHeight = newData[i];
        int y1 = min(8 - barHeight, 7);
        matrix->fillRect(x1, y1 + y, barWidth, barHeight, color);
    }
}

std::pair<String, AppCallback> getNativeAppByName(const String &appName)
{
    if (appName == "time")
    {
        return std::make_pair("time", TimeApp);
    }
    else if (appName == "date")
    {
        return std::make_pair("date", DateApp);
    }
    else if (appName == "temp")
    {
        return std::make_pair("temp", TempApp);
    }
    else if (appName == "hum")
    {
        return std::make_pair("hum", HumApp);
    }
#ifdef ULANZI
    else if (appName == "bat")
    {
        return std::make_pair("bat", BatApp);
    }
#endif
    return std::make_pair("", nullptr);
}

void DisplayManager_::updateAppVector(const char *json)
{
    StaticJsonDocument<512> doc; // Erhöhen Sie die Größe des Dokuments bei Bedarf
    deserializeJson(doc, json);

    JsonArray appArray;
    if (doc.is<JsonObject>())
    {
        JsonArray tempArray = doc.to<JsonArray>();
        tempArray.add(doc.as<JsonObject>());
        appArray = tempArray;
    }
    else if (doc.is<JsonArray>())
    {
        appArray = doc.as<JsonArray>();
    }

    for (JsonObject appObj : appArray)
    {
        String appName = appObj["name"].as<String>();
        bool show = appObj["show"].as<bool>();
        int position = appObj.containsKey("pos") ? appObj["pos"].as<int>() : Apps.size();

        auto appIt = std::find_if(Apps.begin(), Apps.end(), [&appName](const std::pair<String, AppCallback> &app)
                                  { return app.first == appName; });

        std::pair<String, AppCallback> nativeApp = getNativeAppByName(appName);
        if (!show)
        {
            if (appIt != Apps.end())
            {
                Apps.erase(appIt);
            }
        }
        else
        {
            if (nativeApp.second != nullptr)
            {
                if (appIt != Apps.end())
                {
                    Apps.erase(appIt);
                }
                position = position < 0 ? 0 : position >= Apps.size() ? Apps.size()
                                                                      : position;
                Apps.insert(Apps.begin() + position, nativeApp);
            }
            else
            {
                if (appIt != Apps.end() && appObj.containsKey("pos"))
                {
                    std::pair<String, AppCallback> app = *appIt;
                    Apps.erase(appIt);
                    position = position < 0 ? 0 : position >= Apps.size() ? Apps.size()
                                                                          : position;
                    Apps.insert(Apps.begin() + position, app);
                }
            }
        }
    }

    // Set the updated apps vector in the UI and save settings
    ui->setApps(Apps);
    saveSettings();
}

String DisplayManager_::getStat()
{
    StaticJsonDocument<256> doc;
    char buffer[5];
#ifdef ULANZI
    doc[BatKey] = BATTERY_PERCENT;
    doc[BatRawKey] = BATTERY_RAW;
#endif
    snprintf(buffer, 5, "%.0f", CURRENT_LUX);
    doc[LuxKey] = buffer;
    doc[LDRRawKey] = LDR_RAW;
    doc["ram"] = ESP.getFreeHeap();
    doc[BrightnessKey] = BRIGHTNESS;
    snprintf(buffer, 5, "%.0f", CURRENT_TEMP);
    doc[TempKey] = buffer;
    snprintf(buffer, 5, "%.0f", CURRENT_HUM);
    doc[HumKey] = buffer;
    doc[UpTimeKey] = PeripheryManager.readUptime();
    doc[SignalStrengthKey] = WiFi.RSSI();
    doc[UpdateKey] = UPDATE_AVAILABLE;
    doc["messages"] = RECEIVED_MESSAGES;
    String jsonString;
    return serializeJson(doc, jsonString), jsonString;
}

void DisplayManager_::setAppTime(uint16_t duration)
{
    ui->setTimePerApp(duration);
}

void DisplayManager_::setMatrixLayout(int layout)
{
    delete matrix; // Free memory from the current matrix object
    switch (layout)
    {
    case 0:
        matrix = new FastLED_NeoMatrix(leds, 32, 8, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG);
        break;
    case 1:
        matrix = new FastLED_NeoMatrix(leds, 8, 8, 4, 1, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE);
        break;
    case 2:
        matrix = new FastLED_NeoMatrix(leds, 32, 8, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG);
        break;
    default:
        break;
    }

    delete ui;                        // Free memory from the current ui object
    ui = new MatrixDisplayUi(matrix); // Create a new ui object with the new matrix
}

String DisplayManager_::getAppsAsJson()
{
    // Create a JSON object to hold the app positions and names
    DynamicJsonDocument doc(1024);
    JsonObject appsObject = doc.to<JsonObject>();

    // Add each app position and name to the object
    for (size_t i = 0; i < Apps.size(); i++)
    {
        appsObject[Apps[i].first] = i;
    }

    // Serialize the JSON object to a string and return it
    String json;
    serializeJson(appsObject, json);
    return json;
}

void DisplayManager_::powerStateParse(const char *json)
{
    DynamicJsonDocument doc(128);
    DeserializationError error = deserializeJson(doc, json);
    if (error)
        return;
    if (doc.containsKey("state"))
    {
        bool state = doc["state"].as<bool>();
        setPower(state);
    }
}

void DisplayManager_::showSleepAnimation()
{
    matrix->setTextColor(0xFFFF);
    int steps[][2] = {{12, 8}, {13, 7}, {14, 6}, {15, 5}, {14, 4}, {13, 3}, {12, 2}, {13, 1}, {14, 0}, {15, -1}, {14, -2}, {13, -3}, {12, -4}, {13, -5}};
    int numSteps = sizeof(steps) / sizeof(steps[0]);
    for (int i = 0; i < numSteps; i++)
    {
        clear();
        printText(steps[i][0], steps[i][1], "Z", false, 1);
        show();
        delay(80);
    }
}

void DisplayManager_::showCurtainEffect()
{
    int width = 32;
    int height = 8;

    for (int i = 0; i <= width / 2; i++)
    {
        // ui->update();
        matrix->fillRect(0, 0, i, 8, 0xFFFF); // Linker Vorhang
        show();
        delay(80);
    }
}

void DisplayManager_::setPower(bool state)
{
    if (state)
    {
        MATRIX_OFF = false;
        setBrightness(BRIGHTNESS);
    }
    else
    {
        MATRIX_OFF = true;
        showSleepAnimation();
        setBrightness(0);
    }
}

void DisplayManager_::setIndicator1Color(uint16_t color)
{
    ui->setIndicator1Color(color);
}

void DisplayManager_::setIndicator1State(bool state)
{
    ui->setIndicator1State(state);
}

void DisplayManager_::setIndicator2Color(uint16_t color)
{
    ui->setIndicator2Color(color);
}

void DisplayManager_::setIndicator2State(bool state)
{
    ui->setIndicator2State(state);
}

void DisplayManager_::indicatorParser(uint8_t indicator, const char *json)
{

    if (strcmp(json, "") == 0)
    {
        if (indicator == 1)
        {
            ui->setIndicator1State(false);
            MQTTManager.setIndicatorState(1, ui->indicator1State, ui->indicator1Color);
        }
        else
        {
            ui->setIndicator2State(false);
            MQTTManager.setIndicatorState(2, ui->indicator2State, ui->indicator2Color);
        }
        return;
    }

    DynamicJsonDocument doc(128);
    DeserializationError error = deserializeJson(doc, json);
    if (error)
        return;

    if (doc.containsKey("color"))
    {
        auto color = doc["color"];
        if (color.is<String>())
        {
            uint16_t col = hexToRgb565(color.as<String>());

            if (col > 0)
            {
                if (indicator == 1)
                {
                    ui->setIndicator1State(true);
                    ui->setIndicator1Color(col);
                }
                else
                {
                    ui->setIndicator2State(true);
                    ui->setIndicator2Color(col);
                }
            }
            else
            {
                if (indicator == 1)
                {
                    ui->setIndicator1State(false);
                }
                else
                {
                    ui->setIndicator2State(false);
                }
            }
        }
        else if (color.is<JsonArray>() && color.size() == 3)
        {
            uint8_t r = color[0];
            uint8_t g = color[1];
            uint8_t b = color[2];

            if (r == 0 && g == 0 && b == 0)
            {
                if (indicator == 1)
                {
                    ui->setIndicator1State(false);
                }
                else
                {
                    ui->setIndicator2State(false);
                }
            }
            else
            {
                if (indicator == 1)
                {
                    ui->setIndicator1State(true);
                    ui->setIndicator1Color((r << 11) | (g << 5) | b);
                }
                else
                {
                    ui->setIndicator2State(true);
                    ui->setIndicator2Color((r << 11) | (g << 5) | b);
                }
            }
        }
    }

    if (doc.containsKey("blink"))
    {
        if (indicator == 1)
        {
            ui->setIndicator1Blink(doc["blink"].as<bool>());
        }
        else
        {
            ui->setIndicator2Blink(doc["blink"].as<bool>());
        }
    }
    else
    {
        if (indicator == 1)
        {
            ui->setIndicator1Blink(false);
        }
        else
        {
            ui->setIndicator2Blink(false);
        }
    }
    MQTTManager.setIndicatorState(1, ui->indicator1State, ui->indicator1Color);
    // MQTTManager.setIndicatorState(2, ui->indicator2State, ui->indicator2Color);
}

void DisplayManager_::gammaCorrection()
{
    if (GAMMA > 0)
    {
        for (int i = 0; i < 256; i++)
        {
            leds[i] = applyGamma_video(leds[i], GAMMA);
        }
    }
}

void DisplayManager_::sendAppLoop()
{
    MQTTManager.publish("stats/loop", getAppsAsJson().c_str());
}