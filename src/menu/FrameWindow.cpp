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
#include "FrameWindow.h"
#include "common/common.h"
#include "Application.h"
#include "fs/CFile.hpp"
#include "fs/DirList.h"
#include "fs/fs_utils.h"
#include "utils/romXML.h"
#include "system/AsyncDeleter.h"

#define MAX_BUTTONS_ON_PAGE     4

FrameWindow::FrameWindow(int w, int h)
    : GuiFrame(w, h)
    , buttonClickSound(Resources::GetSound("button_click.mp3"))
    , buttonImgData(Resources::GetImageData("button.png"))
    , romButtonImgData(Resources::GetImageData("romButton.png"))
    , romSelectImgData(Resources::GetImageData("romSelect.png"))
    , arrowRightImageData(Resources::GetImageData("rightArrow.png"))
    , arrowLeftImageData(Resources::GetImageData("leftArrow.png"))
    , fskipRightImageData(Resources::GetImageData("rightArrow.png"))
    , fskipLeftImageData(Resources::GetImageData("leftArrow.png"))
    , arrowRightImage(arrowRightImageData)
    , arrowLeftImage(arrowLeftImageData)
    , fskipRightImage(fskipRightImageData)
    , fskipLeftImage(fskipLeftImageData)
    , pauseButtonImage(buttonImgData)
    , loadButtonImage(buttonImgData)
    , configButtonImage(buttonImgData)
    , cfgCloseButtonImage(buttonImgData)
    , cfgSaveButtonImage(buttonImgData)
    , toastImage(500, 80, (GX2Color){ 0, 0, 0, 0 })
    , arrowRightButton(arrowRightImage.getWidth(), arrowRightImage.getHeight())
    , arrowLeftButton(arrowLeftImage.getWidth(), arrowLeftImage.getHeight())
    , fskipRightButton(fskipRightImage.getWidth(), fskipRightImage.getHeight())
    , fskipLeftButton(arrowLeftImage.getWidth(), arrowLeftImage.getHeight())
    , pauseButton(pauseButtonImage.getWidth(), pauseButtonImage.getHeight())
    , loadButton(loadButtonImage.getWidth(), loadButtonImage.getHeight())
    , configButton(configButtonImage.getWidth(), configButtonImage.getHeight())
    , cfgCloseButton(cfgCloseButtonImage.getWidth(), cfgCloseButtonImage.getHeight())
    , cfgSaveButton(cfgSaveButtonImage.getWidth(), cfgSaveButtonImage.getHeight())
    , hblVersionText("CHIP-8 Emulator by rw-r-r_0644", 32, glm::vec4(1.0f))
    , selectedRomText("No ROM Loaded ...", 32, glm::vec4(1.0f))
    , frameSkipText("SPEED", 28, glm::vec4(1.0f))
    , frameSkipNumberText("NORM", 42, glm::vec4(1.0f))
    , pauseButtonText("PLAY", 36, glm::vec4(1.0f))
    , loadButtonText("LOAD ROM", 36, glm::vec4(1.0f))
    , configButtonText("CONFIGURE", 36, glm::vec4(1.0f))
    , cfgCloseButtonText("CLOSE MENU", 36, glm::vec4(1.0f))
    , cfgSaveButtonText("SAVE CONFIG", 36, glm::vec4(1.0f))
    , toastText("TOAST", 36, glm::vec4(1.0f))
    , romSelectImg(romSelectImgData)
    , touchTrigger(GuiTrigger::CHANNEL_1, GuiTrigger::VPAD_TOUCH)
    , wpadTouchTrigger(GuiTrigger::CHANNEL_2 | GuiTrigger::CHANNEL_3 | GuiTrigger::CHANNEL_4 | GuiTrigger::CHANNEL_5, GuiTrigger::BUTTON_A)
    , buttonLTrigger(GuiTrigger::CHANNEL_ALL, GuiTrigger::BUTTON_L | GuiTrigger::BUTTON_LEFT, true)
    , buttonRTrigger(GuiTrigger::CHANNEL_ALL, GuiTrigger::BUTTON_R | GuiTrigger::BUTTON_RIGHT, true)
{
    targetLeftPosition = 0;
    currentLeftPosition = 0;
    listOffset = 0;
    selectedKey = 16;
    frameSkip = 2;
    selectedROM = -1;
    selectedContButton = 0;
    controlUsed = 0;
    toastTimeout = 0.0f;
    
    float scalefactor = 2.0f;
    romSelectImg.setAlignment(ALIGN_CENTER | ALIGN_MIDDLE);
    romSelectImg.setPosition(0, 0);
    romSelectImg.setScale(scalefactor);
    append(&romSelectImg);

    DirList dirList("sd:/roms/CHIP8/", ".ch8", DirList::Files | DirList::CheckSubfolders);

    dirList.SortList();
    
    u8 defaultKeys[16] = {0x5,0x10,0x10,0x10,0x4,0x6,0x2,0x8,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10};

    for(int i = 0; i < dirList.GetFilecount(); i++)
    {
        //! skip hidden linux and mac files
        if(dirList.GetFilename(i)[0] == '.' || dirList.GetFilename(i)[0] == '_')
            continue;

        int idx = frameButtons.size();
        frameButtons.resize(frameButtons.size() + 1);

        frameButtons[idx].execPath = dirList.GetFilepath(i);
        frameButtons[idx].image = new GuiImage(romButtonImgData);
        frameButtons[idx].image->setScale(0.9f);
        frameButtons[idx].iconImgData = NULL;

        std::string romPath = frameButtons[idx].execPath;
        size_t slashPos = romPath.rfind('.');
        if(slashPos != std::string::npos)
            romPath.erase(slashPos);

        u8 * ckeys = NULL;
        u32 keyDataSize = 0;
        
        memcpy(frameButtons[idx].config, defaultKeys, 16);
        
        frameButtons[idx].configPath = romPath + ".config";

        LoadFileToMem(frameButtons[idx].configPath.c_str(), &ckeys, &keyDataSize);
        
        if (ckeys != NULL)
        {
            memcpy(frameButtons[idx].config, ckeys, 16);
            free(ckeys);
            ckeys = NULL;
        }

        u8 * iconData = NULL;
        u32 iconDataSize = 0;

        LoadFileToMem((romPath + ".png").c_str(), &iconData, &iconDataSize);

        if(iconData != NULL)
        {
            frameButtons[idx].iconImgData = new GuiImageData(iconData, iconDataSize);
            free(iconData);
            iconData = NULL;
        }

        const float cfImageScale = 1.0f;

        frameButtons[idx].iconImg = new GuiImage(frameButtons[idx].iconImgData);
        frameButtons[idx].iconImg->setAlignment(ALIGN_LEFT | ALIGN_MIDDLE);
        frameButtons[idx].iconImg->setPosition(-280, 0);
        frameButtons[idx].iconImg->setScale(cfImageScale);

        RomXML metaXml;

        bool xmlReadSuccess = metaXml.LoadRomXMLData((romPath + ".xml").c_str());

//        romPath = frameButtons[idx].execPath;
        slashPos = romPath.rfind('/');
        if(slashPos != std::string::npos)
            romPath.erase(0, slashPos + 1);
        slashPos = romPath.rfind('.');
        if(slashPos != std::string::npos)
            romPath.erase(slashPos);
            
        frameButtons[idx].romName = romPath;

        const char *cpName = xmlReadSuccess ? metaXml.GetName() : romPath.c_str();

        if(strncmp(cpName, "sd:/roms/CHIP8/", strlen("sd:/roms/CHIP8/")) == 0)
           cpName += strlen("sd:/roms/CHIP8/");

        frameButtons[idx].nameLabel = new GuiText(cpName, 42, glm::vec4(1.0f));
        frameButtons[idx].nameLabel->setAlignment(ALIGN_CENTER | ALIGN_MIDDLE);
        frameButtons[idx].nameLabel->setMaxWidth(500, GuiText::SCROLL_HORIZONTAL);
        frameButtons[idx].nameLabel->setPosition(0, 0);

        frameButtons[idx].button = new GuiButton(romButtonImgData->getWidth(), romButtonImgData->getHeight());

        frameButtons[idx].button->setImage(frameButtons[idx].image);
        frameButtons[idx].button->setLabel(frameButtons[idx].nameLabel, 0);
        frameButtons[idx].button->setIcon(frameButtons[idx].iconImg);
        float fXOffset = (i / MAX_BUTTONS_ON_PAGE) * width;
        float fYOffset = (frameButtons[idx].image->getHeight() + 20.0f) * 1.5f - (frameButtons[idx].image->getHeight() + 20) * (i % MAX_BUTTONS_ON_PAGE);
        frameButtons[idx].button->setPosition(currentLeftPosition + fXOffset, fYOffset);
        frameButtons[idx].button->setTrigger(&touchTrigger);
        frameButtons[idx].button->setTrigger(&wpadTouchTrigger);
        frameButtons[idx].button->setEffectGrow();
        frameButtons[idx].button->setSoundClick(buttonClickSound);
        frameButtons[idx].button->clicked.connect(this, &FrameWindow::OnRomButtonClick);

        append(frameButtons[idx].button);
    }

    if((MAX_BUTTONS_ON_PAGE) < frameButtons.size())
    {
        arrowLeftButton.setImage(&arrowLeftImage);
        arrowLeftButton.setEffectGrow();
        arrowLeftButton.setPosition(40, 0);
        arrowLeftButton.setAlignment(ALIGN_LEFT | ALIGN_MIDDLE);
        arrowLeftButton.setTrigger(&touchTrigger);
        arrowLeftButton.setTrigger(&wpadTouchTrigger);
        arrowLeftButton.setTrigger(&buttonLTrigger);
        arrowLeftButton.setSoundClick(buttonClickSound);
        arrowLeftButton.clicked.connect(this, &FrameWindow::OnLeftArrowClick);

        arrowRightButton.setImage(&arrowRightImage);
        arrowRightButton.setEffectGrow();
        arrowRightButton.setPosition(-40, 0);
        arrowRightButton.setAlignment(ALIGN_RIGHT | ALIGN_MIDDLE);
        arrowRightButton.setTrigger(&touchTrigger);
        arrowRightButton.setTrigger(&wpadTouchTrigger);
        arrowRightButton.setTrigger(&buttonRTrigger);
        arrowRightButton.setSoundClick(buttonClickSound);
        arrowRightButton.clicked.connect(this, &FrameWindow::OnRightArrowClick);
        append(&arrowRightButton);
    }

    float arrowScale = 0.45f;
    fskipLeftButton.setImage(&fskipLeftImage);
    fskipLeftButton.setEffectGrow();
    fskipLeftButton.setPosition(-270, 30);
    fskipLeftButton.setAlignment(ALIGN_RIGHT | ALIGN_BOTTOM);
    fskipLeftButton.setTrigger(&touchTrigger);
    fskipLeftButton.setTrigger(&wpadTouchTrigger);
    fskipLeftButton.setSoundClick(buttonClickSound);
    fskipLeftButton.setScale(arrowScale);
    fskipLeftButton.clicked.connect(this, &FrameWindow::OnFSkipLeftArrowClick);
    append(&fskipLeftButton);

    fskipRightButton.setImage(&fskipRightImage);
    fskipRightButton.setEffectGrow();
    fskipRightButton.setPosition(-120, 30);
    fskipRightButton.setAlignment(ALIGN_RIGHT | ALIGN_BOTTOM);
    fskipRightButton.setTrigger(&touchTrigger);
    fskipRightButton.setTrigger(&wpadTouchTrigger);
    fskipRightButton.setSoundClick(buttonClickSound);
    fskipRightButton.setScale(arrowScale);
    fskipRightButton.clicked.connect(this, &FrameWindow::OnFSkipRightArrowClick);
    append(&fskipRightButton);
    
    pauseButtonText.setAlignment(ALIGN_CENTER | ALIGN_MIDDLE);
    pauseButtonText.setPosition(0, 0);

    pauseButton.setImage(&pauseButtonImage);
    pauseButton.setLabel(&pauseButtonText, 0);
    pauseButton.setEffectGrow();
    pauseButton.setPosition(160, 40);
    pauseButton.setAlignment(ALIGN_CENTER | ALIGN_BOTTOM);
    pauseButton.setTrigger(&touchTrigger);
    pauseButton.setTrigger(&wpadTouchTrigger);
    pauseButton.setSoundClick(buttonClickSound);
    pauseButton.setScale(1.0f);
    pauseButton.clicked.connect(this, &FrameWindow::OnPauseButtonClick);
    append(&pauseButton);

    loadButtonText.setAlignment(ALIGN_CENTER | ALIGN_MIDDLE);
    loadButtonText.setPosition(0, 0);
    
    loadButton.setImage(&loadButtonImage);
    loadButton.setLabel(&loadButtonText, 0);
    loadButton.setEffectGrow();
    loadButton.setPosition(-400, 40);
    loadButton.setAlignment(ALIGN_CENTER | ALIGN_BOTTOM);
    loadButton.setTrigger(&touchTrigger);
    loadButton.setTrigger(&wpadTouchTrigger);
    loadButton.setSoundClick(buttonClickSound);
    loadButton.setScale(1.0f);
    loadButton.clicked.connect(this, &FrameWindow::OnLoadButtonClick);
    append(&loadButton);

    configButtonText.setAlignment(ALIGN_CENTER | ALIGN_MIDDLE);
    configButtonText.setPosition(0, 0);
    
    configButton.setImage(&configButtonImage);
    configButton.setLabel(&configButtonText, 0);
    configButton.setEffectGrow();
    configButton.setPosition(-120, 40);
    configButton.setAlignment(ALIGN_CENTER | ALIGN_BOTTOM);
    configButton.setTrigger(&touchTrigger);
    configButton.setTrigger(&wpadTouchTrigger);
    configButton.setSoundClick(buttonClickSound);
    configButton.setScale(1.0f);
    configButton.clicked.connect(this, &FrameWindow::OnConfigButtonClick);
    append(&configButton);
    
    frameSkipText.setAlignment(ALIGN_RIGHT | ALIGN_BOTTOM);
    frameSkipText.setPosition(-184, 105);
    append((&frameSkipText));
    
    frameSkipNumberText.setAlignment(ALIGN_CENTER | ALIGN_BOTTOM);
    frameSkipNumberText.setPosition(423, 80);
    append((&frameSkipNumberText));
    
    hblVersionText.setAlignment(ALIGN_TOP | ALIGN_RIGHT);
    hblVersionText.setPosition(-30, -45);
    append(&hblVersionText);
    
    selectedRomText.setAlignment(ALIGN_TOP | ALIGN_LEFT);
    selectedRomText.setPosition(30, -45);
    append(&selectedRomText);
    
    for (u32 i = 0; i < 17; i++)
    {
        char num[32];
        sprintf(num, "key%d.png", i);
        std::string numStr = num;
        
        keyButtons.resize(keyButtons.size() + 1);
        
        keyButtons[i].imgData = new GuiImageData(Resources::GetFile(numStr.c_str()), Resources::GetFileSize(numStr.c_str()));
        
        int xPos = ((i - 1) % 3);
        int yPos = ((i - 1) / 3);
        
        if (i == 0)
        {
            xPos = 1;
            yPos = 3;
        }
        if (i == 10)
        {
            xPos = 0;
            yPos = 3;
        }
        if (i == 11)
        {
            xPos = 2;
            yPos = 3;
        }
        if (i > 11)
        {
            xPos = 3;
            yPos = (i % 4);
        }
        if (i == 16)
        {
            xPos = 4;
            yPos = 3;
        }
        
        keyButtons[i].image = new GuiImage(keyButtons[i].imgData);
        keyButtons[i].image->setAlignment(ALIGN_CENTER | ALIGN_MIDDLE);
        keyButtons[i].image->setPosition(0, 0);
        keyButtons[i].image->setScale(1.0f);
        
        keyButtons[i].button = new GuiButton(keyButtons[i].image->getWidth(), keyButtons[i].image->getHeight());
        
        keyButtons[i].button->setImage(keyButtons[i].image);
        keyButtons[i].button->setPosition(-225 + (150 * xPos), -60 + (-150 * yPos));
        keyButtons[i].button->setAlignment(ALIGN_CENTER | ALIGN_TOP);
        keyButtons[i].button->setTrigger(&touchTrigger);
        keyButtons[i].button->setTrigger(&wpadTouchTrigger);
        keyButtons[i].button->setEffectGrow();
        keyButtons[i].button->setSoundClick(buttonClickSound);
        keyButtons[i].button->clicked.connect(this, &FrameWindow::OnKeyButtonClick);
        
        append(keyButtons[i].button);
    }
    
    for (u32 i = 0; i < 16; i++)
    {
        char num[32];
        sprintf(num, "button%d.png", i);
        std::string numStr = num;
        
        int idx = contButtons.size();
        contButtons.resize(contButtons.size() + 1);
        
        contButtons[i].imgData = new GuiImageData(Resources::GetFile(numStr.c_str()), Resources::GetFileSize(numStr.c_str()));
        
        int xPos = (i % 4);
        int yPos = (i / 4);
        
        contButtons[i].image = new GuiImage(contButtons[i].imgData);
        contButtons[i].image->setAlignment(ALIGN_CENTER | ALIGN_MIDDLE);
        contButtons[i].image->setPosition(0, 0);
        contButtons[i].image->setScale(1.0f);
        
        idx = defaultKeys[i];
        contButtons[i].iconImg = new GuiImage(keyButtons[idx].imgData);
        contButtons[i].iconImg->setAlignment(ALIGN_CENTER | ALIGN_MIDDLE);
        contButtons[i].iconImg->setPosition(50, -50);
        contButtons[i].iconImg->setScale(0.25f);
        
        contButtons[i].button = new GuiButton(contButtons[i].image->getWidth(), contButtons[i].image->getHeight());
        
        contButtons[i].button->setImage(contButtons[i].image);
        contButtons[i].button->setPosition(-225 + (150 * xPos), -60 + (-150 * yPos));
        contButtons[i].button->setAlignment(ALIGN_CENTER | ALIGN_TOP);
        contButtons[i].button->setIcon(contButtons[i].iconImg);
        contButtons[i].button->setTrigger(&touchTrigger);
        contButtons[i].button->setTrigger(&wpadTouchTrigger);
        contButtons[i].button->setEffectGrow();
        contButtons[i].button->setSoundClick(buttonClickSound);
        contButtons[i].button->clicked.connect(this, &FrameWindow::OnContButtonClick);
        
        append(contButtons[i].button);
    }
    
    cfgCloseButton.setImage(&cfgCloseButtonImage);
    cfgCloseButton.setLabel(&cfgCloseButtonText, 0);
    cfgCloseButton.setEffectGrow();
    cfgCloseButton.setPosition(50, 80);
    cfgCloseButton.setAlignment(ALIGN_LEFT | ALIGN_BOTTOM);
    cfgCloseButton.setTrigger(&touchTrigger);
    cfgCloseButton.setTrigger(&wpadTouchTrigger);
    cfgCloseButton.setSoundClick(buttonClickSound);
    cfgCloseButton.setScale(1.0f);
    cfgCloseButton.clicked.connect(this, &FrameWindow::OnCfgCloseButtonClick);
    append(&cfgCloseButton);

    cfgSaveButton.setImage(&cfgSaveButtonImage);
    cfgSaveButton.setLabel(&cfgSaveButtonText, 0);
    cfgSaveButton.setEffectGrow();
    cfgSaveButton.setPosition(50, 160);
    cfgSaveButton.setAlignment(ALIGN_LEFT | ALIGN_BOTTOM);
    cfgSaveButton.setTrigger(&touchTrigger);
    cfgSaveButton.setTrigger(&wpadTouchTrigger);
    cfgSaveButton.setSoundClick(buttonClickSound);
    cfgSaveButton.setScale(1.0f);
    cfgSaveButton.clicked.connect(this, &FrameWindow::OnCfgSaveButtonClick);
    append(&cfgSaveButton);

    toastImage.setImageColor((GX2Color){  20, 20, 20, 255 }, 0);
    toastImage.setImageColor((GX2Color){  20, 20, 20, 255 }, 1);
    toastImage.setImageColor((GX2Color){  0, 0, 0, 255 }, 2);
    toastImage.setImageColor((GX2Color){  0, 0, 0, 255 }, 3);
    toastImage.setAlignment(ALIGN_CENTER | ALIGN_BOTTOM);
    toastImage.setPosition(0, 50);
    toastImage.setScale(1.0f);
    append(&toastImage);

    toastText.setAlignment(ALIGN_CENTER | ALIGN_BOTTOM);
    toastText.setPosition(0, 100);
    toastText.setScale(1.0f);
    append(&toastText);

    for(u32 i = 0; i < frameButtons.size(); i++)
    {
        frameButtons[i].button->setVisible(false);
        frameButtons[i].button->setState(GuiElement::STATE_DISABLED);
    }
    
    for(u32 i = 0; i < 16; i++)
    {
        contButtons[i].button->setVisible(false);
        contButtons[i].button->setState(GuiElement::STATE_DISABLED);
    }

    for(u32 i = 0; i < 17; i++)
    {
        keyButtons[i].button->setVisible(false);
        keyButtons[i].button->setState(GuiElement::STATE_DISABLED);
    }

    romSelectImg.setVisible(false);
    arrowLeftButton.setVisible(false);
    arrowLeftButton.setState(GuiElement::STATE_DISABLED);
    arrowRightButton.setVisible(false);
    arrowRightButton.setState(GuiElement::STATE_DISABLED);
    cfgCloseButton.setVisible(false);
    cfgCloseButton.setState(GuiElement::STATE_DISABLED);
    cfgSaveButton.setVisible(false);
    cfgSaveButton.setState(GuiElement::STATE_DISABLED);
}

FrameWindow::~FrameWindow()
{
    for(u32 i = 0; i < frameButtons.size(); ++i)
    {
        delete frameButtons[i].image;
        delete frameButtons[i].nameLabel;
        delete frameButtons[i].button;
        delete frameButtons[i].iconImgData;
        delete frameButtons[i].iconImg;
    }

    for(u32 i = 0; i < 17; ++i)
    {
        delete keyButtons[i].imgData;
        delete keyButtons[i].image;
        delete keyButtons[i].button;
    }

    for(u32 i = 0; i < 16; ++i)
    {
        delete contButtons[i].imgData;
        delete contButtons[i].image;
        delete contButtons[i].button;
        delete contButtons[i].iconImg;
    }

    Resources::RemoveSound(buttonClickSound);
    Resources::RemoveImageData(buttonImgData);
    Resources::RemoveImageData(romButtonImgData);
    Resources::RemoveImageData(romSelectImgData);
    Resources::RemoveImageData(arrowRightImageData);
    Resources::RemoveImageData(arrowLeftImageData);
    Resources::RemoveImageData(fskipRightImageData);
    Resources::RemoveImageData(fskipLeftImageData);
}

void FrameWindow::OnRomButtonClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger)
{
    bool disableButtons = false;

    for(u32 i = 0; i < frameButtons.size(); i++)
    {
        if(button == frameButtons[i].button)
        {
            u8 * newROM = NULL;
            u32 fileSize = 0;
            
            LoadFileToMem((frameButtons[i].execPath).c_str(), &newROM, &fileSize);
            
            if (newROM != NULL)
            {
                CHIP8_initialize(newROM, frameButtons[i].config);
                free(newROM);
                newROM = NULL;
                disableButtons = true;
                selectedROM = i;
                selectedRomText.setText((frameButtons[i].romName).c_str());
            }
            
            break;
        }
    }

    if(disableButtons)
    {
        for(u32 i = 0; i < frameButtons.size(); i++)
        {
            frameButtons[i].button->setVisible(false);
            frameButtons[i].button->setState(GuiElement::STATE_DISABLED);
        }
        
        romSelectImg.setVisible(false);
        arrowLeftButton.setVisible(false);
        arrowLeftButton.setState(GuiElement::STATE_DISABLED);
        arrowRightButton.setVisible(false);
        arrowRightButton.setState(GuiElement::STATE_DISABLED);

        loadButton.setVisible(true);
        loadButton.clearState(GuiElement::STATE_DISABLED);
        configButton.setVisible(true);
        configButton.clearState(GuiElement::STATE_DISABLED);
        pauseButton.setVisible(true);
        pauseButton.clearState(GuiElement::STATE_DISABLED);

        fskipLeftButton.setVisible(true);
        fskipLeftButton.clearState(GuiElement::STATE_DISABLED);
        fskipRightButton.setVisible(true);
        fskipRightButton.clearState(GuiElement::STATE_DISABLED);
        frameSkipText.setVisible(true);
        frameSkipNumberText.setVisible(true);
    }
}

void FrameWindow::OnConfigButtonDeselected(GuiElement *element)
{
    configButton.effectFinished.disconnect_all();

    configButton.setVisible(false);
    configButton.setState(GuiElement::STATE_DISABLED);

    for(u32 i = 0; i < 16; i++)
    {
        contButtons[i].button->setVisible(true);
        contButtons[i].button->clearState(GuiElement::STATE_DISABLED);
        int idx = frameButtons[selectedROM].config[i];
        contButtons[i].iconImg = new GuiImage(keyButtons[idx].imgData);
        contButtons[i].iconImg->setAlignment(ALIGN_CENTER | ALIGN_MIDDLE);
        contButtons[i].iconImg->setPosition(50, -50);
        contButtons[i].iconImg->setScale(0.25f);
        contButtons[i].button->setIcon(contButtons[i].iconImg);
    }    
}

void FrameWindow::OnConfigButtonClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger)
{
    if (selectedROM >= 0)
    {
        romSelectImg.setVisible(true);
        cfgCloseButton.setVisible(true);
        cfgCloseButton.clearState(GuiElement::STATE_DISABLED);
        cfgSaveButton.setVisible(true);
        cfgSaveButton.clearState(GuiElement::STATE_DISABLED);
        loadButton.setVisible(false);
        loadButton.setState(GuiElement::STATE_DISABLED);
        pauseButton.setVisible(false);
        pauseButton.setState(GuiElement::STATE_DISABLED);
        
        fskipLeftButton.setVisible(false);
        fskipLeftButton.setState(GuiElement::STATE_DISABLED);
        fskipRightButton.setVisible(false);
        fskipRightButton.setState(GuiElement::STATE_DISABLED);
        frameSkipText.setVisible(false);
        frameSkipNumberText.setVisible(false);

        configButton.effectFinished.connect(this, &FrameWindow::OnConfigButtonDeselected);
    }
}

void FrameWindow::OnCfgCloseButtonDeselected(GuiElement *element)
{
    cfgCloseButton.effectFinished.disconnect_all();
    
    cfgCloseButton.setVisible(false);
    cfgCloseButton.setState(GuiElement::STATE_DISABLED);
    cfgSaveButton.setVisible(false);
    cfgSaveButton.setState(GuiElement::STATE_DISABLED);

    for(u32 i = 0; i < 17; i++)
    {
        if (i < 16)
        {
            contButtons[i].button->effectFinished.disconnect_all();
            contButtons[i].button->setVisible(false);
            contButtons[i].button->setState(GuiElement::STATE_DISABLED);
        }
        keyButtons[i].button->effectFinished.disconnect_all();
        keyButtons[i].button->setVisible(false);
        keyButtons[i].button->setState(GuiElement::STATE_DISABLED);
    }

    romSelectImg.setVisible(false);
    
    loadButton.setVisible(true);
    loadButton.clearState(GuiElement::STATE_DISABLED);
    configButton.setVisible(true);
    configButton.clearState(GuiElement::STATE_DISABLED);
    pauseButton.setVisible(true);
    pauseButton.clearState(GuiElement::STATE_DISABLED);
    
    fskipLeftButton.setVisible(true);
    fskipLeftButton.clearState(GuiElement::STATE_DISABLED);
    fskipRightButton.setVisible(true);
    fskipRightButton.clearState(GuiElement::STATE_DISABLED);
    frameSkipText.setVisible(true);
    frameSkipNumberText.setVisible(true);
}

void FrameWindow::OnCfgCloseButtonClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger)
{
    cfgCloseButton.effectFinished.connect(this, &FrameWindow::OnCfgCloseButtonDeselected);
}

void FrameWindow::OnCfgSaveButtonClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger)
{
   CFile * save_file = new CFile(frameButtons[selectedROM].configPath, CFile::WriteOnly);
   if (save_file->isOpen())
   {
       save_file->write(frameButtons[selectedROM].config, 16);
       save_file->close();
       toastText.setText("SAVED CONFIG FILE");
       toastTimeout = 3.0f;
   }
}

void FrameWindow::OnKeyButtonDeselected(GuiElement *element)
{
    for(u32 i = 0; i < 17; i++)
    {
        keyButtons[i].button->setVisible(false);
        keyButtons[i].button->setState(GuiElement::STATE_DISABLED);
    }
    
    for(u32 i = 0; i < 16; i++)
    {
        contButtons[i].button->setVisible(true);
        contButtons[i].button->clearState(GuiElement::STATE_DISABLED);
        int idx = frameButtons[selectedROM].config[i];
        contButtons[i].iconImg = new GuiImage(keyButtons[idx].imgData);
        contButtons[i].iconImg->setAlignment(ALIGN_CENTER | ALIGN_MIDDLE);
        contButtons[i].iconImg->setPosition(50, -50);
        contButtons[i].iconImg->setScale(0.25f);
        contButtons[i].button->setIcon(contButtons[i].iconImg);
    }

    cfgCloseButton.clearState(GuiElement::STATE_DISABLED);
}

void FrameWindow::OnKeyButtonClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger)
{
    for(u32 i = 0; i < 17; i++)
    {
        if (button == keyButtons[i].button)
        {
            selectedKey = i;
            break;
        }
    }
    
    for (int j = 0; j < 16; j++)
    {
        if (frameButtons[selectedROM].config[j] == selectedKey)
            frameButtons[selectedROM].config[j] = 0x10;
    }
    
    frameButtons[selectedROM].config[selectedContButton] = selectedKey;
    
    memcpy(&kconf, frameButtons[selectedROM].config, 16);
    
    button->effectFinished.connect(this, &FrameWindow::OnKeyButtonDeselected);
}

void FrameWindow::OnContButtonDeselected(GuiElement *element)
{
    for(u32 i = 0; i < 16; i++)
    {
        contButtons[i].button->setVisible(false);
        contButtons[i].button->setState(GuiElement::STATE_DISABLED);
    }
    
    for(u32 i = 0; i < 17; i++)
    {
        keyButtons[i].button->setVisible(true);
        keyButtons[i].button->clearState(GuiElement::STATE_DISABLED);
    }
    
    cfgCloseButton.setState(GuiElement::STATE_DISABLED);
}

void FrameWindow::OnContButtonClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger)
{
    for(u32 i = 0; i < 16; i++)
    {
        if (button == contButtons[i].button)
        {
            selectedContButton = i;
            break;
        }
    }

    button->effectFinished.connect(this, &FrameWindow::OnContButtonDeselected);
}

void FrameWindow::OnLoadButtonClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger)
{
    romSelectImg.setVisible(true);
    arrowLeftButton.setVisible(true);
    arrowLeftButton.clearState(GuiElement::STATE_DISABLED);
    arrowRightButton.setVisible(true);
    arrowRightButton.clearState(GuiElement::STATE_DISABLED);

    for(u32 i = 0; i < frameButtons.size(); i++)
    {
        frameButtons[i].button->setVisible(true);
        frameButtons[i].button->clearState(GuiElement::STATE_DISABLED);
    }
    
    loadButton.setVisible(false);
    loadButton.setState(GuiElement::STATE_DISABLED);
    configButton.setVisible(false);
    configButton.setState(GuiElement::STATE_DISABLED);
    pauseButton.setVisible(false);
    pauseButton.setState(GuiElement::STATE_DISABLED);
    
    fskipLeftButton.setVisible(false);
    fskipLeftButton.setState(GuiElement::STATE_DISABLED);
    fskipRightButton.setVisible(false);
    fskipRightButton.setState(GuiElement::STATE_DISABLED);
    frameSkipText.setVisible(false);
    frameSkipNumberText.setVisible(false);
}

void FrameWindow::OnPauseButtonClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger)
{
    if (emulateROM)
    {
        pauseButtonText.setText("PLAY");
        emulateROM = false;
    }
    else
    {
        controlUsed = controller->chanIdx;
        pauseButtonText.setText("PAUSE");
        emulateROM = true;
    }
}

void FrameWindow::OnLeftArrowClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger)
{
    if(listOffset > 0)
    {
        listOffset--;
        targetLeftPosition = -listOffset * getWidth();

        if(listOffset == 0)
            remove(&arrowLeftButton);
        append(&arrowRightButton);
    }
}

void FrameWindow::OnRightArrowClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger)
{
    if((listOffset * MAX_BUTTONS_ON_PAGE) < (int)frameButtons.size())
    {
        listOffset++;
        targetLeftPosition = -listOffset * getWidth();

        if(((listOffset + 1) * MAX_BUTTONS_ON_PAGE) >= (int)frameButtons.size())
            remove(&arrowRightButton);

        append(&arrowLeftButton);
    }
}

void FrameWindow::OnFSkipLeftArrowClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger)
{
    if (frameSkip > 0)
        frameSkip--;
    
//    char skip[8];
//    sprintf(skip, "%d", (frameSkip - 10));
    
    if (frameSkip == 0)
    {
        frameSkipNumberText.setText("VSLOW");
    }
    if (frameSkip == 1)
    {
        frameSkipNumberText.setText("SLOW");
    }
    if (frameSkip == 2)
    {
        frameSkipNumberText.setText("NORM");
    }
}

void FrameWindow::OnFSkipRightArrowClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger)
{
    if (frameSkip < 2)
        frameSkip++;
    
//    char skip[8];
//    sprintf(skip, "%d", (frameSkip - 10));
    
    if (frameSkip == 0)
    {
        frameSkipNumberText.setText("VSLOW");
    }
    if (frameSkip == 1)
    {
        frameSkipNumberText.setText("SLOW");
    }
    if (frameSkip == 2)
    {
        frameSkipNumberText.setText("NORM");
    }
}

void FrameWindow::draw(CVideo *pVideo)
{
    if (toastTimeout > 1.0f)
    {
        toastImage.setAlpha(1.0f);
        toastText.setAlpha(1.0f);
    }
    else
    {
        toastImage.setAlpha(toastTimeout);
        toastText.setAlpha(toastTimeout);
    }
    
    if (toastTimeout > 0.0f)
    {
        toastTimeout = toastTimeout - 0.01f;
    }
    
    if (toastTimeout < 0.0f)
    {
        toastTimeout = 0.0f;
    }
    
    bool bUpdatePositions = false;

    if(currentLeftPosition < targetLeftPosition)
    {
        currentLeftPosition += 35;

        if(currentLeftPosition > targetLeftPosition)
            currentLeftPosition = targetLeftPosition;

        bUpdatePositions = true;
    }
    else if(currentLeftPosition > targetLeftPosition)
    {
        currentLeftPosition -= 35;

        if(currentLeftPosition < targetLeftPosition)
            currentLeftPosition = targetLeftPosition;

        bUpdatePositions = true;
    }

    if(bUpdatePositions)
    {
        bUpdatePositions = false;

        for(u32 i = 0; i < frameButtons.size(); i++)
        {
            float fXOffset = (i / MAX_BUTTONS_ON_PAGE) * getWidth();
            float fYOffset = (frameButtons[i].image->getHeight() + 20.0f) * 1.5f - (frameButtons[i].image->getHeight() + 20) * (i % MAX_BUTTONS_ON_PAGE);
            frameButtons[i].button->setPosition(currentLeftPosition + fXOffset, fYOffset);
        }
    }

    GuiFrame::draw(pVideo);
}
