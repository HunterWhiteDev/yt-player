#pragma once

#include <QObject>
#include <qcontainerfwd.h>
#include <qlist.h>
#include <qobject.h>
#include <qprocess.h>
#include <qtmetamacros.h>
#include <qtypes.h>
using namespace std;

class PlayerItem : public QObject {
  Q_OBJECT

  typedef struct {
    QString id;
    QString title;
    QString url;
    QString duration;
  } SearchResult;

public:
  explicit PlayerItem(QObject *parent = nullptr);
  ~PlayerItem() override;

  QVariantList lastSearchResults;
  QVariantMap getSongFromAPI(QString id);
  QVector<QString> history;
  qint16 historyIdx;

  QProcess mpvProcess;
  QString nowPlayingId;
  QVariantMap searchRelated(QString videoTitle);
  void playNext();

  Q_INVOKABLE
  void search(QString input);
  Q_INVOKABLE
  void loadVideo(QString url, bool updateIndex);
  Q_INVOKABLE
  void play();
  Q_INVOKABLE
  void pause();
  Q_INVOKABLE
  void previous();
  Q_INVOKABLE
  void next();

  Q_SLOT
  void loadingFinished(QVariantMap vidoeData, int updateIndex);

Q_SIGNALS:
  void searchUpdate(QVariantList searchResults);
  void nowPlayingUpdate(QVariantMap data);
  void playingStateChange(bool state);
  void historyUpdate(int length, int idx);
};

void loadVideoWork(QVariantMap videoData, bool updateIndex,
                   QProcess *mpvProcess);
