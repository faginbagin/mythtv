#include "mythscreenstack.h"
#include "mythmainwindow.h"
#include "mythscreentype.h"
#include "mythpainter.h"
#include "mythevent.h"

#include <cassert>

#include <QCoreApplication>
#include <QTimer>
#include <QString>

const int kFadeVal = 20;

MythScreenStack::MythScreenStack(MythMainWindow *parent, const QString &name,
                                 bool mainstack)
               : QObject(parent)
{
    setObjectName(name);

    if (parent)
        parent->AddScreenStack(this, mainstack);

    EnableEffects();
}

MythScreenStack::~MythScreenStack()
{
    CheckDeletes(true);

    while (!m_Children.isEmpty())
    {
        MythScreenType *child = m_Children.back();
        MythScreenStack::PopScreen(child, false, true); // Don't fade, do delete
    }
}

void MythScreenStack::EnableEffects(void)
{
    m_DoTransitions = (GetPainter()->SupportsAlpha() &&
                       GetPainter()->SupportsAnimation());
}

int MythScreenStack::TotalScreens(void) const
{
    return m_Children.count();
}

void MythScreenStack::AddScreen(MythScreenType *screen, bool allowFade)
{
    if (!screen)
        return;

    m_DoInit = false;

    MythScreenType *old = m_topScreen;
    if (old && screen->IsFullscreen())
        old->aboutToHide();

    m_Children.push_back(screen);

    if (allowFade && m_DoTransitions)
    {
        m_newTop = screen;
        DoNewFadeTransition();
    }
    else
    {
        if (parent())
            reinterpret_cast<MythMainWindow *>(parent())->update();
        RecalculateDrawOrder();
        if (!screen->IsInitialized())
            m_DoInit = true;
    }

    screen->aboutToShow();

    m_topScreen = screen;

    emit topScreenChanged(m_topScreen);
}

void MythScreenStack::PopScreen(MythScreenType *screen, bool allowFade,
                                bool deleteScreen)
{
    if (!screen)
    {
        screen = m_topScreen;
    }
    if (!screen || screen->IsDeleting())
        return;

    bool poppedFullscreen = screen->IsFullscreen();

    screen->aboutToHide();

    if (m_Children.isEmpty())
        return;

    MythMainWindow *mainwindow = GetMythMainWindow();

    screen->setParent(nullptr);
    if ((screen == m_topScreen) && allowFade && m_DoTransitions
        && !mainwindow->IsExitingToMain())
    {
        screen->SetFullscreen(false);
        if (deleteScreen)
        {
            screen->SetDeleting(true);
            m_ToDelete.push_back(screen);
        }
        screen->AdjustAlpha(1, -kFadeVal);
    }
    else
    {
        for (int i = 0; i < m_Children.size(); ++i)
        {
            if (m_Children.at(i) == screen)
                m_Children.remove(i);
        }
        if (deleteScreen)
            screen->deleteLater();

        screen = nullptr;

        mainwindow->update();
        if (mainwindow->IsExitingToMain())
        {
            QCoreApplication::postEvent(
                mainwindow, new QEvent(MythEvent::kExitToMainMenuEventType));
        }
    }

    m_topScreen = nullptr;

    MythScreenStack::RecalculateDrawOrder();

    // If we're fading it, we still want to draw it.
    if (screen && !m_DrawOrder.contains(screen))
        m_DrawOrder.push_back(screen);

    if (!m_Children.isEmpty())
    {
        foreach (auto & draw, m_DrawOrder)
        {
            if (draw != screen && !draw->IsDeleting())
            {
                m_topScreen = draw;
                draw->SetAlpha(255);
                if (poppedFullscreen)
                    draw->aboutToShow();
            }
        }
    }

    if (m_topScreen)
    {
        m_topScreen->SetRedraw();

        if (!allowFade || !m_DoTransitions)
            emit topScreenChanged(m_topScreen);
    }
    else
    {
        // Screen still needs to be redrawn if we have popped the last screen
        // off the popup stack, or similar
        if (mainwindow->GetMainStack())
        {
            MythScreenType *mainscreen = mainwindow->GetMainStack()->GetTopScreen();
            if (mainscreen)
                mainscreen->SetRedraw();
        }

        if (!allowFade || !m_DoTransitions)
            emit topScreenChanged(nullptr);
    }
}

MythScreenType *MythScreenStack::GetTopScreen(void) const
{
    if (m_topScreen)
        return m_topScreen;
    if (!m_DrawOrder.isEmpty())
        return m_DrawOrder.back();
    return nullptr;
}

void MythScreenStack::GetDrawOrder(QVector<MythScreenType *> &screens)
{
    if (m_InNewTransition)
        CheckNewFadeTransition();
    CheckDeletes();

    screens = m_DrawOrder;
}

void MythScreenStack::GetScreenList(QVector<MythScreenType *> &screens)
{
    if (m_InNewTransition)
        CheckNewFadeTransition();
    CheckDeletes();

    screens = m_Children;
}

void MythScreenStack::ScheduleInitIfNeeded(void)
{
    // make sure Init() is called outside the paintEvent
    if (m_DoInit && m_topScreen && !m_InitTimerStarted &&
        !m_topScreen->IsLoading())
    {
        m_InitTimerStarted = true;
        QTimer::singleShot(100, this, SLOT(doInit()));
    }
}

void MythScreenStack::doInit(void)
{
    if (m_DoInit && m_topScreen)
    {
        m_DoInit = false;

        if (!m_topScreen->IsLoaded())
            m_topScreen->LoadInForeground();

        if (!m_topScreen->IsInitialized())
            m_topScreen->doInit();
    }
    m_InitTimerStarted = false;
}

void MythScreenStack::RecalculateDrawOrder(void)
{
    m_DrawOrder.clear();

    if (m_Children.isEmpty())
        return;

    foreach (auto screen, m_Children)
    {
        if (screen->IsFullscreen())
            m_DrawOrder.clear();

        m_DrawOrder.push_back(screen);
    }

    if (m_DrawOrder.isEmpty())
    {
        MythScreenType *screen = GetTopScreen();
        if (screen)
            m_DrawOrder.push_back(screen);
    }
}

void MythScreenStack::DoNewFadeTransition(void)
{
    m_InNewTransition = true;
    m_newTop->SetAlpha(0);
    m_newTop->AdjustAlpha(1, kFadeVal);

    if (m_newTop->IsFullscreen())
    {
        foreach (auto & draw, m_DrawOrder)
        {
            if (!draw->IsDeleting())
                draw->AdjustAlpha(1, -kFadeVal);
        }

        m_DrawOrder.push_back(m_newTop);
    }
    else
        RecalculateDrawOrder();
}

void MythScreenStack::CheckNewFadeTransition(void)
{
    if (!m_newTop)
    {
        m_InNewTransition = false;
        return;
    }

    if (m_newTop->GetAlpha() >= 255)
    {
        m_InNewTransition = false;
        if (!m_newTop->IsInitialized())
            m_DoInit = true;
        m_newTop = nullptr;

        RecalculateDrawOrder();
    }
}

void MythScreenStack::CheckDeletes(bool force)
{
    if (m_ToDelete.isEmpty())
        return;

    bool changed = false;

    QVector<MythScreenType *>::Iterator it = m_ToDelete.begin();
    while (it != m_ToDelete.end() && !m_ToDelete.isEmpty())
    {
        bool deleteit = false;

        if (force || (*it)->GetAlpha() <= 0)
        {
            deleteit = true;
        }

        if (!deleteit)
        {
            bool found = false;

            foreach (auto & test, m_DrawOrder)
            {
                if (*it == test)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
                deleteit = true;
        }

        if (deleteit)
        {
            for (auto *test = m_Children.begin();
                 test != m_Children.end();
                 ++test)
            {
                if (*test == *it)
                {
                    m_Children.erase(test);
                    break;
                }
            }

            if (*it == m_newTop)
                m_newTop = nullptr;
            delete (*it);
            m_ToDelete.erase(it);
            it = m_ToDelete.begin();
            changed = true;
            continue;
        }

        ++it;
    }

    if (changed)
    {
        RecalculateDrawOrder();
        emit topScreenChanged(GetTopScreen());
    }
}

QString MythScreenStack::GetLocation(bool fullPath) const
{
    if (fullPath)
    {
        QString path;
        foreach (auto child, m_Children)
        {
            if (!child->IsDeleting())
            {
                if (path.isEmpty())
                    path = child->objectName();
                else
                    path += '/' + child->objectName();
            }
        }
        return path;
    }

    if (m_topScreen)
        return m_topScreen->objectName();

    return QString();
}

MythPainter* MythScreenStack::GetPainter(void)
{
    return GetMythPainter();
}
