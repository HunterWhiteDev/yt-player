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

class WaitThread : public QThread {
  Q_OBJECT
public:
  explicit WaitThread(QObject *parent = nullptr) : QThread(parent) {}

  bool m_isPolling = false;

private:
  void run() override;
Q_SIGNALS:
  void mpvChange(bool isPlaying);
  void timePos(QJsonValue data);
  void pollingStateChanged(bool isPolling);
  void playNext();
};

class MpvThread : public QThread {
  Q_OBJECT

  QVariantMap m_videoData;
  QProcess *m_mpvProcess;

public:
  explicit MpvThread(QVariantMap videoData, QObject *parent = nullptr)
      : QThread(parent), m_videoData(videoData) {}

  void quitProcess();

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

  MpvThread *nowPlayingThread;
  WaitThread *waitForFinishThread;

  QVariantMap searchRelated(QString videoTitle);
  void playNext();

  void mpvChange(bool mpvState);
  void timePos(QJsonValue data);

  Q_INVOKABLE
  void search(QString input);
  Q_INVOKABLE
  void loadVideo(QVariantMap videoData, bool updateIndex);
  Q_INVOKABLE
  void play();
  Q_INVOKABLE
  void pause();
  Q_INVOKABLE
  void previous();
  Q_INVOKABLE
  void next();

Q_SIGNALS:
  void searchUpdate(QVariantList searchResults);
  void nowPlayingUpdate(QVariantMap data);
  void playingStateChange(bool state);
  void historyUpdate(int length, int idx);
  void mpvStarted(bool mpvState);
  void timeUpdate(QJsonValue data);
  void quitMpvProcess();
};

void loadVideoWork(QVariantMap videoData);

void waitForFinish();
