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
#include "MainWindow.h"
#include "dynamic_libs/os_functions.h"
#include "dynamic_libs/socket_functions.h"
#include "Application.h"
#include "utils/StringTools.h"
#include "utils/logger.h"


MainWindow::MainWindow(int w, int h)
    : GuiFrame(w, h)
    , width(w)
    , height(h)
    , bgImageColor(w, h, (GX2Color){ 0, 0, 0, 0 })
    , playfieldData(Resources::GetImageData("playfield.png"))
    , playfield(playfieldData)    
    , frameWindow(w, h)
{
    bgImageColor.setImageColor((GX2Color){  79, 153, 239, 255 }, 0);
    bgImageColor.setImageColor((GX2Color){  79, 153, 239, 255 }, 1);
    bgImageColor.setImageColor((GX2Color){  59, 159, 223, 255 }, 2);
    bgImageColor.setImageColor((GX2Color){  59, 159, 223, 255 }, 3);
    append(&bgImageColor);

    for(int i = 0; i < 4; i++)
    {
        std::string filename = strfmt("player%i_point.png", i+1);
        pointerImgData[i] = Resources::GetImageData(filename.c_str());
        pointerImg[i] = new GuiImage(pointerImgData[i]);
        pointerImg[i]->setScale(1.5f);
        pointerValid[i] = false;
    }
    
    float scalefactor = 4.0f;
    playfield.setAlignment(ALIGN_CENTER | ALIGN_MIDDLE);
    playfield.setPosition(0, 40);
    playfield.setScale(scalefactor);
    append(&playfield);

    append(&frameWindow);
    
    controlUsed = 0;
}

MainWindow::~MainWindow()
{
    remove(&frameWindow);
    remove(&bgImageColor);
    remove(&playfield);
    Resources::RemoveImageData(playfieldData);

    while(!tvElements.empty())
    {
        delete tvElements[0];
        remove(tvElements[0]);
    }
    while(!drcElements.empty())
    {
        delete drcElements[0];
        remove(drcElements[0]);
    }
    for(int i = 0; i < 4; i++)
    {
        delete pointerImg[i];
        Resources::RemoveImageData(pointerImgData[i]);
    }
}

void MainWindow::updateEffects()
{
    //! dont read behind the initial elements in case one was added
    u32 tvSize = tvElements.size();
    u32 drcSize = drcElements.size();

    for(u32 i = 0; (i < drcSize) && (i < drcElements.size()); ++i)
    {
        drcElements[i]->updateEffects();
    }

    //! only update TV elements that are not updated yet because they are on DRC
    for(u32 i = 0; (i < tvSize) && (i < tvElements.size()); ++i)
    {
        u32 n;
        for(n = 0; (n < drcSize) && (n < drcElements.size()); n++)
        {
            if(tvElements[i] == drcElements[n])
                break;
        }
        if(n == drcElements.size())
        {
            tvElements[i]->updateEffects();
        }
    }
}

void MainWindow::update(GuiController *controller)
{
    //! dont read behind the initial elements in case one was added
    //u32 tvSize = tvElements.size();

    if(controller->chan & GuiTrigger::CHANNEL_1)
    {
        u32 drcSize = drcElements.size();

        for(u32 i = 0; (i < drcSize) && (i < drcElements.size()); ++i)
        {
            drcElements[i]->update(controller);
        }
    }
    else
    {
        u32 tvSize = tvElements.size();

        for(u32 i = 0; (i < tvSize) && (i < tvElements.size()); ++i)
        {
            tvElements[i]->update(controller);
        }
    }

//    //! only update TV elements that are not updated yet because they are on DRC
//    for(u32 i = 0; (i < tvSize) && (i < tvElements.size()); ++i)
//    {
//        u32 n;
//        for(n = 0; (n < drcSize) && (n < drcElements.size()); n++)
//        {
//            if(tvElements[i] == drcElements[n])
//                break;
//        }
//        if(n == drcElements.size())
//        {
//            tvElements[i]->update(controller);
//        }
//    }

    if(controller->chanIdx >= 1 && controller->chanIdx <= 4 && controller->data.validPointer)
    {
        int wpadIdx = controller->chanIdx - 1;
        f32 posX = controller->data.x;
        f32 posY = controller->data.y;
        pointerImg[wpadIdx]->setPosition(posX, posY);
        pointerImg[wpadIdx]->setAngle(controller->data.pointerAngle);
        pointerValid[wpadIdx] = true;
    }    
}

void MainWindow::drawDrc(CVideo *video)
{
    for(u32 i = 0; i < drcElements.size(); ++i)
    {
        drcElements[i]->draw(video);
    }

    for(int i = 0; i < 4; i++)
    {
        if(pointerValid[i])
        {
            pointerImg[i]->setAlpha(0.5f);
            pointerImg[i]->draw(video);
            pointerImg[i]->setAlpha(1.0f);
        }
    }
}

void MainWindow::drawTv(CVideo *video)
{
    for(u32 i = 0; i < tvElements.size(); ++i)
    {
        tvElements[i]->draw(video);
    }

    for(int i = 0; i < 4; i++)
    {
        if(pointerValid[i])
        {
            pointerImg[i]->draw(video);
            pointerValid[i] = false;
        }
    }
}

bool MainWindow::RunEmu(GuiController *controller)  // emulation function ran each cycle that ROM is ready to be played or isn't paused
{
//    bool refreshImage = false;  // only update the texture if it has changed
    
    int repeat = frameWindow.frameSkip;  // set the frameskip to speed up or slow down emulation and how often the playfield needs to be updated
    
//    if (repeat < 0)
//    {
//        int pause = 10 - frameWindow.frameSkip;
//        usleep(1000 * pause);
//        repeat = 0;
//    }
    
    repeat = (repeat / 2) + 1;

    bool sound = false;
    
    for (int i = 0; i < repeat; i++)
    {
        int opcodes = 2;
        if ((frameWindow.frameSkip / 1))
        {
            opcodes = 5;
        }
        
        if (mode)
            opcodes = opcodes * 3;
        
        while(opcodes)
        {
            CHIP8_setKeys(controller->data.buttons_h); // Store key press state (Press and Release)
            CHIP8_emulateCycle(); // Emulate one cycle
            for(int cy = 0; cy < 128; cy++) {  // currently using a u32* color buffer for the playfield
                for(int cx = 0; cx < 256; cx++) {
                    if(!(gfx[(cx/2)][(cy/2)] == 0x00)) {                        
                        renderTexture[cy * 256 + cx] = 0xFFFFFFFF;
                    }
                    else {
                        renderTexture[(cy * 256) + cx] = 0x000000FF;
                    }
                }
            }
            opcodes--;
        }
        
        if (i == (repeat - 1))
        {
            CHIP8_decreaseTimers();
            if (CHIP8_soundFlag)
                sound = true;
        }

//        playfieldData->loadGDImage(virtualDisp); // if using a gd image for the playfield, uncomment this line and comment the one below
        playfieldData->loadBufImage(); // currently using a u32* color buffer for the playfield

        playfield.setImageData(playfieldData);
    }
    
    return sound;
}
