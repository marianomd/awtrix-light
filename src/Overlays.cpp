#include "Overlays.h"

#include "MatrixDisplayUi.h"
#include "Functions.h"
#include "MenuManager.h"
#include "PeripheryManager.h"
#include <WiFi.h>
#include "effects.h"
#include "MQTTManager.h"

std::vector<Notification> notifications;
bool notifyFlag = false;


uint32_t fadeColor(uint32_t color, uint32_t interval)
{
    float phase = (sin(2 * PI * millis() / float(interval)) + 1) * 0.5;
    uint8_t r = ((color >> 16) & 0xFF) * phase;
    uint8_t g = ((color >> 8) & 0xFF) * phase;
    uint8_t b = (color & 0xFF) * phase;
    return (r << 16) | (g << 8) | b;
}

uint32_t TextEffect(uint32_t color, uint32_t fade, uint32_t blink)
{
    if (fade > 0)
    {
        float phase = (sin(2 * PI * millis() / float(fade)) + 1) * 0.5;
        uint8_t r = ((color >> 16) & 0xFF) * phase;
        uint8_t g = ((color >> 8) & 0xFF) * phase;
        uint8_t b = (color & 0xFF) * phase;
        return (r << 16) | (g << 8) | b;
    }
    else if (blink > 0)
    {
        if (millis() % blink > blink / 2)
        {
            return color;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return color;
    }
}

void StatusOverlay(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, GifPlayer *gifPlayer)
{
    if (!WiFi.isConnected())
    {
        matrix->drawPixel(0, 0, fadeColor(0xFF0000, 2000));
    }
    if (!MQTTManager.isConnected())
    {
        matrix->drawPixel(0, 7, fadeColor(0xFFFF00, 2000));
    }
}

void MenuOverlay(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, GifPlayer *gifPlayer)
{
    if (!MenuManager.inMenu)
        return;
    matrix->fillScreenRGB888(0, 0, 0);
    DisplayManager.setTextColor(0xFFFFFF);
    DisplayManager.printText(0, 6, utf8ascii(MenuManager.menutext()).c_str(), true, 2);
}

void NotifyOverlay(MatrixPanel_I2S_DMA *matrix, MatrixDisplayUiState *state, GifPlayer *gifPlayer)
{
    // Check if notification flag is set
    if (notifications.empty())
    {
        notifyFlag = false;
        return; // Exit function if flag is not set
    }
    else
    {
        notifyFlag = true;
    }

    if (notifications[0].wakeup && MATRIX_OFF)
    {
        DisplayManager.setBrightness(BRIGHTNESS);
    }

    // Check if notification duration has expired or if repeat count is 0 and hold is not enabled
    if ((((millis() - notifications[0].startime >= notifications[0].duration) && notifications[0].repeat == -1) || notifications[0].repeat == 0) && !notifications[0].hold)
    {
        // Reset notification flags and exit function
        DEBUG_PRINTLN(F("Notification deleted"));
        PeripheryManager.stopSound();
        if (notifications.size() >= 2)
        {
            notifications[1].startime = millis();
        }
        notifications[0].icon.close();
        notifications.erase(notifications.begin());

        if (notifications[0].wakeup && MATRIX_OFF)
        {
            DisplayManager.setBrightness(0);
        }

        if (notifications.empty())
        {
            notifyFlag = false;
            if (AUTO_TRANSITION)
                DisplayManager.forceNextApp();
        }

        return;
    }

    // Set current app name
    CURRENT_APP = F("Notification");

    // Check if notification has an icon
    bool hasIcon = notifications[0].icon;

    // Clear the matrix display
    DisplayManager.drawFilledRect(0, 0, 32, 8, notifications[0].background);

    if (notifications[0].effect > -1)
    {
        callEffect(matrix, 0, 0, notifications[0].effect);
    }

    // Calculate text and available width
    uint16_t textWidth = 0;
    if (!notifications[0].fragments.empty())
    {
        for (const auto &fragment : notifications[0].fragments)
        {
            textWidth += getTextWidth(fragment.c_str(), notifications[0].textCase);
        }
    }
    else
    {
        textWidth = getTextWidth(notifications[0].text.c_str(), notifications[0].textCase);
    }

    uint16_t availableWidth = hasIcon ? 24 : 32;

    // Check if text is scrolling
    bool noScrolling = (textWidth <= availableWidth);
    int iconWidth;
    auto renderFirst = [&]()
    {
        if (hasIcon)
        {
            if (notifications[0].pushIcon > 0 && !noScrolling)
            {
                if (notifications[0].iconPosition < 0 && notifications[0].iconWasPushed == false && notifications[0].scrollposition > 8)
                {
                    notifications[0].iconPosition += movementFactor;
                }

                if (notifications[0].scrollposition < (9 - notifications[0].textOffset) && !notifications[0].iconWasPushed)
                {
                    notifications[0].iconPosition = notifications[0].scrollposition - 9 + notifications[0].textOffset;

                    if (notifications[0].iconPosition <= -9 - notifications[0].textOffset)
                    {
                        notifications[0].iconWasPushed = true;
                    }
                }
            }

            if (notifications[0].isGif)
            {
                iconWidth = gifPlayer->playGif(notifications[0].iconPosition + notifications[0].iconOffset, 0, &notifications[0].icon);
            }
            else
            {
                DisplayManager.drawJPG(notifications[0].iconPosition + notifications[0].iconOffset, 0, notifications[0].icon);
                iconWidth = 8;
            }
            if (!noScrolling)
            {
                if (notifications[0].progress > -1)
                {
                    DisplayManager.drawLine(iconWidth + notifications[0].iconPosition + notifications[0].iconOffset, 0, iconWidth + notifications[0].iconPosition, 6, notifications[0].background);
                }
                else
                {
                    DisplayManager.drawLine(iconWidth + notifications[0].iconPosition + notifications[0].iconOffset, 0, iconWidth + notifications[0].iconPosition, 7, notifications[0].background);
                }
            }
        }

        if (notifications[0].progress > -1)
        {
            DisplayManager.drawProgressBar((hasIcon ? 8 : 0), 7, notifications[0].progress, notifications[0].pColor, notifications[0].pbColor);
        }

        if (notifications[0].drawInstructions.length() > 0)
        {
            DisplayManager.processDrawInstructions(0, 0, notifications[0].drawInstructions);
        }

        if (notifications[0].barSize > 0)
        {
            DisplayManager.drawBarChart(0, 0, notifications[0].barData, notifications[0].barSize, hasIcon, notifications[0].color);
        }

        if (notifications[0].lineSize > 0)
        {
            DisplayManager.drawLineChart(0, 0, notifications[0].lineData, notifications[0].lineSize, hasIcon, notifications[0].color);
        }
    };

    if (notifications[0].topText)
    {
        renderFirst();
    }

    // Check if text needs to be scrolled
    if (textWidth > availableWidth && notifications[0].scrollposition + notifications[0].textOffset <= (-textWidth))
    {
        // Reset scroll position and icon position if needed
        notifications[0].scrollDelay = 0;
        notifications[0].scrollposition = 9 + notifications[0].textOffset;

        if (notifications[0].pushIcon == 2)
        {
            notifications[0].iconWasPushed = false;
        }

        if (notifications[0].repeat > 0)
        {
            --notifications[0].repeat;
            if (notifications[0].repeat == 0)
                return;
        }
    }

    if (!noScrolling)
    {
        if ((notifications[0].scrollDelay > MATRIX_FPS * 1.2) || ((hasIcon ? notifications[0].textOffset + 9 : notifications[0].textOffset) > 31))
        {
            if (!notifications[0].noScrolling)
            {
                if (notifications[0].scrollSpeed == -1)
                {
                    notifications[0].scrollposition -= movementFactor * ((float)SCROLL_SPEED / 100);
                }
                else
                {
                    notifications[0].scrollposition -= movementFactor * (notifications[0].scrollSpeed / 100);
                }
            }
        }
        else
        {
            ++notifications[0].scrollDelay;
            if (hasIcon)
            {
                if (notifications[0].iconWasPushed && notifications[0].pushIcon == 1)
                {
                    notifications[0].scrollposition = 0 + notifications[0].textOffset;
                }
                else
                {
                    notifications[0].scrollposition = 9 + notifications[0].textOffset;
                }
            }
            else
            {
                notifications[0].scrollposition = 0 + notifications[0].textOffset;
            }
        }
    }

    int16_t textX;
    if (notifications[0].center)
    {
        textX = hasIcon ? ((24 - textWidth) / 2) + 9 : ((32 - textWidth) / 2);
    }
    else
    {
        textX = hasIcon ? 9 : 0;
    }

    if (noScrolling)
    {
        // Disable repeat if text is not scrolling
        notifications[0].repeat = -1;

        if (!notifications[0].fragments.empty())
        {
            int16_t fragmentX = textX + notifications[0].textOffset;
            for (size_t i = 0; i < notifications[0].fragments.size(); ++i)
            {
                if (notifications[0].colors[i] == 0)
                {
                    DisplayManager.HSVtext(fragmentX, 6, notifications[0].fragments[i].c_str(), false, notifications[0].textCase);
                }
                else
                {
                    DisplayManager.setTextColor(TextEffect(notifications[0].colors[i], notifications[0].fade, notifications[0].blink));
                    DisplayManager.printText(fragmentX, 6, notifications[0].fragments[i].c_str(), false, notifications[0].textCase);
                }

                fragmentX += getTextWidth(notifications[0].fragments[i].c_str(), notifications[0].textCase);
            }
        }
        else
        {
            if (notifications[0].rainbow)
            {
                DisplayManager.HSVtext(textX + notifications[0].textOffset, 6, notifications[0].text.c_str(), false, notifications[0].textCase);
            }
            else if (notifications[0].gradient[0] > -1 && notifications[0].gradient[1] > -1)
            {
                DisplayManager.GradientText(textX + notifications[0].textOffset, 6, notifications[0].text.c_str(), notifications[0].gradient[0], notifications[0].gradient[1], false, notifications[0].textCase);
            }
            else
            {
                DisplayManager.setTextColor(TextEffect(notifications[0].color, notifications[0].fade, notifications[0].blink));

                DisplayManager.printText(textX + notifications[0].textOffset, 6, notifications[0].text.c_str(), false, notifications[0].textCase);
            }
        }
    }
    else
    {
        if (!notifications[0].fragments.empty())
        {
            int16_t fragmentX = notifications[0].scrollposition;
            for (size_t i = 0; i < notifications[0].fragments.size(); ++i)
            {
                if (notifications[0].colors[i] == 0)
                {
                    DisplayManager.HSVtext(fragmentX, 6, notifications[0].fragments[i].c_str(), false, notifications[0].textCase);
                }
                else
                {
                    DisplayManager.setTextColor(TextEffect(notifications[0].colors[i], notifications[0].fade, notifications[0].blink));
                    DisplayManager.printText(fragmentX, 6, notifications[0].fragments[i].c_str(), false, notifications[0].textCase);
                }
                fragmentX += getTextWidth(notifications[0].fragments[i].c_str(), notifications[0].textCase);
            }
        }
        else
        {
            if (notifications[0].rainbow)
            {
                // Display scrolling text in rainbow color if enabled
                DisplayManager.HSVtext(notifications[0].scrollposition, 6, notifications[0].text.c_str(), false, notifications[0].textCase);
            }
            else if (notifications[0].gradient[0] > -1 && notifications[0].gradient[1] > -1)
            {
                DisplayManager.GradientText(notifications[0].scrollposition + notifications[0].textOffset, 6, notifications[0].text.c_str(), notifications[0].gradient[0], notifications[0].gradient[1], false, notifications[0].textCase);
            }
            else
            {
                // Set text color
                DisplayManager.setTextColor(TextEffect(notifications[0].color, notifications[0].fade, notifications[0].blink));
                DisplayManager.printText(notifications[0].scrollposition + notifications[0].textOffset, 6, notifications[0].text.c_str(), false, notifications[0].textCase);
            }
        }
    }

    // Display icon if present and not pushed
    if (!notifications[0].topText)
    {
        renderFirst();
    }

    if (!notifications[0].soundPlayed || notifications[0].loopSound)
    {
        if (!PeripheryManager.isPlaying())
        {
            if (notifications[0].sound != "" || (MATRIX_OFF && notifications[0].wakeup))
            {
                PeripheryManager.playFromFile(notifications[0].sound);
            }

            if (notifications[0].rtttl != "")
            {
                PeripheryManager.playRTTTLString(notifications[0].rtttl);
            }
        }
        notifications[0].soundPlayed = true;
    }

    // Reset text color after displaying notification
    DisplayManager.getInstance().resetTextColor();
}

OverlayCallback overlays[] = {MenuOverlay, NotifyOverlay, StatusOverlay};
