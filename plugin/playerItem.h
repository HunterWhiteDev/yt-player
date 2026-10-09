#include <QMediaDevices>
#include <QMediaPlayer>
#include <QObject>
#include <QVariant>
#include <qaudiooutput.h>
#include <qcontainerfwd.h>
#include <qlist.h>
#include <qmediaplayer.h>
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
  explicit SearchThread(QString input, QObject *parent = nullptr)
      : QThread(parent), input(input) {}

private:
  void run() override;
Q_SIGNALS:
  void finished(QVariantList searchResults);
};

class LoadVideoThread : public QThread {
  Q_OBJECT

  QVariantMap m_videoData;
  QProcess *m_ytdlpProcess;

public:
  explicit LoadVideoThread(QVariantMap videoData, QObject *parent = nullptr)
      : QThread(parent), m_videoData(videoData) {}

  void quitProcess();

Q_SIGNALS:
  void finished(bool state);
  void updateLoadingStatus(QString status);

private:
  void run() override;
};

class PlayerItem : public QObject {
  Q_OBJECT

public:
  explicit PlayerItem(QObject *parent = nullptr);
  ~PlayerItem() override;

  // Thread members
  SearchThread *m_searchThread;
  LoadVideoThread *m_loadVideoThread;

  QVariantList lastSearchResults;
  QVariantMap getSongFromAPI(QString id);
  QVector<QVariantMap> history;
  int historyIdx = 0;
  QVariantMap nowPlaying;

  QVariantMap searchRelated(QString videoTitle);
  void playNext();

  void handleSongStart();

  Q_INVOKABLE
  void search(QString input);
  Q_INVOKABLE
  void loadVideo(QVariantMap videoData, bool addToHistory);
  Q_INVOKABLE
  void previous();
  Q_INVOKABLE
  void next();
  Q_INVOKABLE
  void setHistoryIdx(int idx);

Q_SIGNALS:
  void searchUpdate(QVariantList searchResults);
  void nowPlayingUpdate(QVariantMap data);
  void historyUpdate(QVector<QVariantMap> history, int idx);
  void songStarted(bool mpvState);
  void timeUpdate(QJsonValue data);
  void quitMpvProcess();
  void updateLoadingStatus(QString status);
};

void loadVideoWork(QVariantMap videoData);
