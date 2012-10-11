/*
    speak.cpp

    a MythTV project object to speak to user

*/

// c/c++
#include <unistd.h>
#include <stdlib.h>
#include <cmath>

//qt

// mythtv
#include "mythlogging.h"
#include "lcddevice.h"

//mythlcdserver
#include "speak.h"
#include "lcdserver.h"

Speak::Speak(LCDServer *lparent) : QObject(NULL)
{
    // Constructor for Speak

    if (debug_level > 0)
        LOG(VB_GENERAL, LOG_INFO,
            "Speak: constructor");
    m_spdconn = NULL;
    m_timer.invalidate();
}

Speak::~Speak()
{
    if (debug_level > 0)
        LOG(VB_GENERAL, LOG_INFO,
            "Speak: destructor");
    if (m_spdconn)
    {
        spd_close(m_spdconn);
        m_spdconn = NULL;
    }

}

bool Speak::init()
{
    m_spdconn = spd_open("mythlcdserver", NULL, NULL, SPD_MODE_THREADED);
    if (m_spdconn == NULL)
    {
        LOG(VB_GENERAL, LOG_ERR, "Speak: failed to connect to speech-dispatcher");
        return false;
    }
    return true;
}

void Speak::stopAll()
{
    if (debug_level > 0)
        LOG(VB_GENERAL, LOG_INFO, "Speak: stopAll");
}

void Speak::reset()
{
    if (debug_level > 0)
        LOG(VB_GENERAL, LOG_INFO, "Speak: reset");
}

void Speak::switchToTime()
{
    if (debug_level > 0)
        LOG(VB_GENERAL, LOG_INFO, "Speak: switchToTime");
}

void Speak::switchToMusic(const QString &artist, const QString &album, const QString &track)
{
    QString speakText;
    if (!artist.isEmpty())
        speakText = artist + ". ";
    if (!album.isEmpty())
        speakText += album + ". ";
    speakText += track;

    if (debug_level > 0)
        LOG(VB_GENERAL, LOG_INFO, "Speak: switchToMusic artist=" + artist + " album=" + album + " track=" + track + " speakText=" + speakText);

    spd_say(m_spdconn, SPD_TEXT, speakText.toUtf8());
}

void Speak::setMusicProgress(QString time, float value)
{
    if (debug_level > 0)
        LOG(VB_GENERAL, LOG_INFO, QString("Speak: setMusicProgress time=%1 value=%2").arg(time).arg(value));
}

void Speak::setMusicRepeat (int repeat)
{
    if (debug_level > 0)
        LOG(VB_GENERAL, LOG_INFO, QString("Speak: setMusicRepeat repeat=%1").arg(repeat));
}

void Speak::setMusicShuffle (int shuffle)
{
    if (debug_level > 0)
        LOG(VB_GENERAL, LOG_INFO, QString("Speak: setMusicShuffle shuffle=%1").arg(shuffle));
}

void Speak::switchToChannel(QString channum, QString title, QString subtitle)
{
    QString speakText;
    if (!channum.isEmpty())
        speakText = "Channel " + channum + ". ";
    speakText += title + ". ";
    speakText += subtitle;

    if (debug_level > 0)
        LOG(VB_GENERAL, LOG_INFO, "Speak: switchToChannel channum=" + channum + " title=" + title + " subtitle=" + subtitle + " speakText=" + speakText);

    spd_say(m_spdconn, SPD_TEXT, speakText.toUtf8());
}

void Speak::setChannelProgress(const QString &time, float value)
{
    if (debug_level > 0)
        LOG(VB_GENERAL, LOG_INFO, QString("Speak: setChannelProgress time=%1 value=%2").arg(time).arg(value));
}

void Speak::switchToMenu(QList<LCDMenuItem> *menuItems, QString menu,
                                 bool popup)
{
    if (menuItems->isEmpty())
        return;

    QList<LCDMenuItem>::iterator it = menuItems->begin();
    const LCDMenuItem *curItem;
    const LCDMenuItem *selectedItem = NULL;

    while (it != menuItems->end())
    {
        curItem = &(*it);
        ++it;
        if (curItem->isSelected())
        {
            selectedItem = curItem;
        }

        if (menu.isEmpty())
        {
            // Here we try to provide a menu name based on an item in the menu
            // Of course, this won't work with another language.
            if (curItem->ItemName() == "Yes, Exit now")
                menu = "MYTH EXIT";
            else if (curItem->ItemName() == "Yes, delete it")
                menu = "MYTH DELETE";
            else if (curItem->ItemName() == "Enter standby mode")
                menu = "MYTH SYSTEM";
            else if (curItem->ItemName() == "Change Group Filter")
                menu = "MYTH Group List";
            else if (curItem->ItemName() == "Storage Options")
                menu = "MYTH Recording Options";
            else if (curItem->ItemName() == "Subtitles")
                menu = "MYTH Playback";
        }
        if (!menu.isEmpty() && selectedItem != NULL)
            break;
    }

    QString selectedText;
    if (selectedItem != NULL)
    {
        selectedText = selectedItem->ItemName();
    }
    else if (menu.isEmpty())
    {
        LOG(VB_GENERAL, LOG_ERR, "Speak: switchToMenu no menu or selected item");
        return;
    }

    if (menu == m_prevMenu && selectedText == m_prevSelected)
    {
        // Mythfrontend sends the same message twice when a menu is first entered
        // this code ignores the second message
        if (m_timer.isValid() && m_timer.elapsed() < 200)
        {
            if (debug_level > 0)
            {
                LOG(VB_GENERAL, LOG_INFO, "Speak: switchToMenu duplicate menu=" + menu + " selected=" + selectedText);
                return;
            }
        }
    }

    QString speakText;
    if (!menu.isEmpty() && menu != m_prevMenu)
    {
        // Adding a period seems to add a pause beween the menu and the item.
        speakText = menu + " Menu. ";
        speakText += selectedText;
    }
    else
        speakText = selectedText;

    m_prevMenu = menu;
    m_prevSelected = selectedText;
    m_timer.start();

    // Filter out undesired characters
    // Two tildes appear in recording items, separating title - subtitle from time
    // E.g. The Colbert Report - Reihan Salam ~  ~ 11:31 PM
    // Sadly, the date isn't provided, although I think that's why there are two tildes
    speakText.remove(QChar('~'));

    if (debug_level > 0)
    {
        LOG(VB_GENERAL, LOG_INFO, "Speak: switchToMenu menu=" + menu + " selected=" + selectedText + " speakText=" + speakText);
    }

    spd_say(m_spdconn, SPD_TEXT, speakText.toUtf8());
}

void Speak::switchToGeneric(QList<LCDTextItem> *textItems)
{
    if (debug_level > 0)
        LOG(VB_GENERAL, LOG_INFO, "Speak: switchToGeneric");
}

void Speak::setGenericProgress(bool b, float value)
{
    if (debug_level > 0)
        LOG(VB_GENERAL, LOG_INFO, QString("Speak: setGenericProgress b=%1 value=%2").arg(b).arg(value));
}

void Speak::switchToVolume(QString app_name)
{
    if (debug_level > 0)
        LOG(VB_GENERAL, LOG_INFO, "Speak: switchToVolume app_name=" + app_name);
}

void Speak::setVolumeLevel(float value)
{
    if (debug_level > 0)
        LOG(VB_GENERAL, LOG_INFO, QString("Speak: setVolumeLevel value=%1").arg(value));
}

void Speak::switchToNothing()
{
    if (debug_level > 0)
        LOG(VB_GENERAL, LOG_INFO, "Speak: switchToNothing");
    stopAll();
}

/* vim: set expandtab tabstop=4 shiftwidth=4: */
