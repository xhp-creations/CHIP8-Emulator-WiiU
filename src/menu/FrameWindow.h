/****************************************************************************
 * Copyright (C) 2015 Dimok
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
#ifndef _FRAME_WINDOW_H_
#define _FRAME_WINDOW_H_

#include "gui/Gui.h"
#include "gui/GuiFrame.h"

class FrameWindow : public GuiFrame, public sigslot::has_slots<>
{
public:
    FrameWindow(int w, int h);
    virtual ~FrameWindow();

    void draw(CVideo *pVideo);
    
    int frameSkip;
private:
    void OnRomButtonClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger);
    void OnLoadButtonClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger);
    void OnConfigButtonDeselected(GuiElement *element);
    void OnConfigButtonClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger);
    void OnCfgCloseButtonDeselected(GuiElement *element);
    void OnCfgCloseButtonClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger);
    void OnCfgSaveButtonClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger);
    void OnKeyButtonDeselected(GuiElement *element);
    void OnKeyButtonClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger);
    void OnContButtonDeselected(GuiElement *element);
    void OnContButtonClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger);
    void OnPauseButtonClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger);
    void OnLeftArrowClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger);
    void OnRightArrowClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger);
    void OnFSkipLeftArrowClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger);
    void OnFSkipRightArrowClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger);

    GuiSound *buttonClickSound;
    GuiImageData * buttonImgData;
    GuiImageData * romButtonImgData;
    GuiImageData * romSelectImgData;

    GuiImageData* arrowRightImageData;
    GuiImageData* arrowLeftImageData;
    GuiImageData* fskipRightImageData;
    GuiImageData* fskipLeftImageData;
    
    GuiImage arrowRightImage;
    GuiImage arrowLeftImage;
    GuiImage fskipRightImage;
    GuiImage fskipLeftImage;
    GuiImage pauseButtonImage;
    GuiImage loadButtonImage;
    GuiImage configButtonImage;
    GuiImage cfgCloseButtonImage;
    GuiImage cfgSaveButtonImage;
    GuiImage toastImage;
    
    GuiButton arrowRightButton;
    GuiButton arrowLeftButton;
    GuiButton fskipRightButton;
    GuiButton fskipLeftButton;
    GuiButton pauseButton;
    GuiButton loadButton;
    GuiButton configButton;
    GuiButton cfgCloseButton;
    GuiButton cfgSaveButton;
    
    GuiText hblVersionText;
    GuiText selectedRomText;
    GuiText frameSkipText;
    GuiText frameSkipNumberText;
    GuiText pauseButtonText;
    GuiText loadButtonText;
    GuiText configButtonText;
    GuiText cfgCloseButtonText;
    GuiText cfgSaveButtonText;
    GuiText toastText;
    
    GuiImage romSelectImg;

    typedef struct
    {
        GuiImageData *imgData;
        GuiImage *image;
        GuiButton *button;
    } keyButton;
    
    std::vector<keyButton> keyButtons;

    typedef struct
    {
        GuiImageData *imgData;
        GuiImage *image;
        GuiButton *button;
        GuiImage *iconImg;
        int setKey;
    } contButton;
    
    std::vector<contButton> contButtons;

    typedef struct
    {
        std::string execPath;
        std::string configPath;
        std::string romName;
        GuiImage *image;
        GuiButton *button;
        GuiText *nameLabel;
        GuiImageData *iconImgData;
        GuiImage *iconImg;
        u8 config[16];
    } frameButton;

    std::vector<frameButton> frameButtons;
    GuiTrigger touchTrigger;
    GuiTrigger wpadTouchTrigger;
    GuiTrigger buttonLTrigger;
    GuiTrigger buttonRTrigger;
    int listOffset;
    int currentLeftPosition;
    int targetLeftPosition;
    int selectedROM;
    int selectedContButton;
    int selectedKey;
    float toastTimeout;
};

#endif //_HOMEBREW_WINDOW_H_
