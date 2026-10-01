#include <QObject>
#include <QVariant>
#include <qcontainerfwd.h>
#include <qlist.h>
#include <qobject.h>
#include <qprocess.h>
#include <qthread.h>
#include <qtmetamacros.h>
#include <qtypes.h>
using namespace std;


class SearchThread : public QThread {
    Q_OBJECT

    QString input;

public:
    explicit SearchThread(QString input, QObject *parent = nullptr) : QThread(parent), input(input) {}


private:
    void run() override;
Q_SIGNALS:
    void finished(QVariantList searchResults);
};

class LoadVideoThread : public QThread {
    Q_OBJECT

    QVariantMap m_videoData;
    QProcess *m_mpvProcess;
    QProcess *m_ytdlpProcess;

public:
    explicit LoadVideoThread(QVariantMap videoData, QObject *parent = nullptr)
        : QThread(parent), m_videoData(videoData) {}

    void quitProcess();

Q_SIGNALS:
    void finished(bool state);

private:
    void run() override;
};


class MpvWatchThread : public QThread {
    Q_OBJECT


public:
    explicit MpvWatchThread(QObject *parent = nullptr)
        : QThread(parent) {}

    void quitProcess();

Q_SIGNALS:
    void timeUpdate(int time);

private:
    void run() override;
};


class PlayerItem : public QObject {
    Q_OBJECT

public:
    explicit PlayerItem(QObject *parent = nullptr);
    ~PlayerItem() override;

    QVariantMap nowPlaying;

    QVariantList lastSearchResults;
    QVariantMap getSongFromAPI(QString id);
    QVector<QVariantMap> history;
    qint16 historyIdx;


    QVariantMap searchRelated(QString videoTitle);
    void playNext();

    void mpvChange(bool mpvState);
    void timePos(QJsonValue data);

    Q_INVOKABLE
    void search(QString input);
    Q_INVOKABLE
    void loadVideo(QVariantMap videoData, bool updateIndex);
    Q_INVOKABLE
    void previous();
    Q_INVOKABLE
    void next();
    Q_INVOKABLE
    void play();
    Q_INVOKABLE
    void pause();
    Q_INVOKABLE
    void setTime(int time);



Q_SIGNALS:
    void searchUpdate(QVariantList searchResults);
    void nowPlayingUpdate(QVariantMap data);
    void historyUpdate(int length, int idx);
    void songStarted(bool mpvState);
    void timeUpdate(QJsonValue data);
    void quitMpvProcess();
};

void loadVideoWork(QVariantMap videoData);

void waitForFinish();
