#ifndef SPEAK_H_
#define SPEAK_H_

#include <QStringList>
#include <QObject>
#include <QList>
#include <QElapsedTimer>

#include <libspeechd.h>

class LCDServer;
class LCDTextItem;
class LCDMenuItem;
class QEvent;
class QTimer;


class Speak : public QObject
{
    Q_OBJECT

  public:

    Speak(LCDServer *lparent);

   ~Speak();

    bool init();

    void stopAll(void);
    void reset(void);

    void switchToTime();
    void switchToMusic(const QString &artist, const QString &album, const QString &track);
    void setMusicProgress(QString time, float generic_progress);
    void setMusicRepeat(int repeat);
    void setMusicShuffle(int shuffle);
    void switchToChannel(QString channum = "", QString title = "", 
                         QString subtitle = "");
    void setChannelProgress(const QString &time, float percentViewed);
    void switchToMenu(QList<LCDMenuItem> *menuItems, QString menu = "",
                      bool popup = true);
    void switchToGeneric(QList<LCDTextItem> *textItems);
    void setGenericProgress(bool busy, float generic_progress);

    void switchToVolume(QString app_name);
    void setVolumeLevel(float volume_level);

    void switchToNothing();

private:
    SPDConnection* m_spdconn;
    QElapsedTimer m_timer;
    QString m_prevMenu;
    QString m_prevSelected;
};

#endif
