#pragma once

#include <QObject>
#include <QVariant>
#include <qcontainerfwd.h>
#include <qlist.h>
#include <qobject.h>
#include <qprocess.h>
#include <qtmetamacros.h>
#include <qtypes.h>
using namespace std;

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

  QProcess *mpvProcess;
  QThread *nowPlayingThread;
  QThread *waitForFinishThread;

  QVariantMap searchRelated(QString videoTitle);
  void playNext();

  Q_INVOKABLE
  void search(QString input);
  Q_INVOKABLE
  void loadVideo(QVariantMap videoData, bool updateIndex);
  Q_INVOKABLE
  void play();
  Q_INVOKABLE
  void pause();
  // Q_INVOKABLE
  // void previous();
  // Q_INVOKABLE
  // void next();

Q_SIGNALS:
  void searchUpdate(QVariantList searchResults);
  void nowPlayingUpdate(QVariantMap data);
  void playingStateChange(bool state);
  void historyUpdate(int length, int idx);
};

void loadVideoWork(QVariantMap videoData, QProcess *mpvProcess);

void waitForFinish();
