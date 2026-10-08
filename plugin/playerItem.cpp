#include "playerItem.h"
#include <QAudioDevice>
#include <QAudioOutput>
#include <QChar>
#include <QDebug>
#include <QDir>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QProcess>
#include <QString>
#include <QStringLiteral>
#include <QThread>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <qaudiodevice.h>
#include <qaudiooutput.h>
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qdir.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qlist.h>
#include <qlogging.h>
#include <qmap.h>
#include <qmediadevices.h>
#include <qmediaplayer.h>
#include <qobject.h>
#include <qprocess.h>
#include <qthread.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qurl.h>
#include <qvariant.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

namespace playerItem {
std::string exec(const char *cmd) {
  char buffer[128];
  std::string result = "";
  FILE *pipe = popen(cmd, "r");
  while (fgets(buffer, sizeof buffer, pipe) != NULL) {
    result += buffer;
  }
  pclose(pipe);
  return result;
}
} // namespace playerItem

void LoadVideoThread::run() {

  // Removes song if one already exists
  remove("/tmp/video.mp3");

  m_ytdlpProcess = new QProcess();
  m_ytdlpProcess->setProcessChannelMode(QProcess::MergedChannels);

  QStringList args;
  args << QStringLiteral("-c")
       << QStringLiteral("yt-dlp -x %1 -t mp3 -o /tmp/video")
              .arg(m_videoData.value(QStringLiteral("url")).toString());

  m_ytdlpProcess->start(QStringLiteral("bash"), args);
  m_ytdlpProcess->waitForFinished();

  QUrl url = QUrl::fromLocalFile(QStringLiteral("/tmp/video.mp3"));

  m_ytdlpProcess->terminate();
  finished(true);
}

void LoadVideoThread::quitProcess() {
  m_ytdlpProcess->terminate();
  m_ytdlpProcess->waitForFinished();
}

void SearchThread::run() {
  QStringList args;
  args << QStringLiteral("ytsearch7:%1").arg(input)
       << QStringLiteral("--flat-playlist") << QStringLiteral("--print")
       << QStringLiteral(
              "%(title)s*%(duration)s*%(ie_key)s*%(id)s*%(url)s*%(channel)s");
  QProcess *process = new QProcess();
  process->setProcessChannelMode(QProcess::MergedChannels);
  process->start(QStringLiteral("yt-dlp"), args);

  // make sure it starts
  if (!process->waitForStarted()) {
    qDebug() << "Failed to start! Error code: " << process->error();
    return;
  }

  // wait for finish
  if (!process->waitForFinished()) {
    qDebug() << "Failed to finish! Error code: " << process->error();
    return;
  }

  // read back and return
  QByteArray stdOut = process->readAllStandardOutput();
  QString stdOutQString = QString::fromUtf8(stdOut);
  qDebug() << process->readAllStandardError();

  QStringList outputStringList = stdOutQString.split(QChar::fromLatin1('\n'));

  QVariantList searchResults;

  for (int i = 0; i < outputStringList.count(); i++) {

    QStringList lineStringList = outputStringList[i].split(QStringLiteral("*"));

    // If it's a malformed string just skip it
    if (lineStringList.count() < 5)
      continue;

    QVariantMap map;

    QString title = lineStringList[0];
    QString duration = lineStringList[1];
    QString type = lineStringList[2];
    QString id = lineStringList[3];
    QString url = lineStringList[4];
    QString channel = lineStringList[5];

    // //"YouTube" should be the correct type (ie key). Channels for example are
    // "YouTubeTab"
    if (type.compare(QStringLiteral("Youtube")) != 0)
      continue;

    map.insert(QStringLiteral("title"), title);
    map.insert(QStringLiteral("id"), id);
    map.insert(QStringLiteral("duration"), duration);
    map.insert(QStringLiteral("type"), type);
    map.insert(QStringLiteral("url"), url);
    map.insert(QStringLiteral("channel"), channel);

    searchResults.append(map);
  }

  // Save this so we can get meta from the last search when the user clicks play
  // lastSearchResults = searchResults;
  process->terminate();
  process->waitForFinished();
  process->deleteLater();
  finished(searchResults);
};

PlayerItem::PlayerItem(QObject *parent) : QObject(parent), historyIdx(-1) {

  // Create cache dir if it does not exist
  char *p_username = getlogin();
  QString filePath = QStringLiteral("/home/%1/.cache/ytplayer").arg(p_username);

  QVariantMap nowPlaying;

  bool dirExists = QDir(filePath).exists();
  if (!dirExists) {
    QDir(filePath).mkdir(QStringLiteral("."));
  }

  QVector<QString> history;
}

void PlayerItem::search(QString input) {

  m_searchThread = new SearchThread(input);
  m_searchThread->start();

  connect(m_searchThread, &SearchThread::finished, this,
          &PlayerItem::searchUpdate);
}

// If updateIndex is passed, we move the history index to the last  position.
// Other wise we handle it in the next() or previous() functions

void PlayerItem::loadVideo(QVariantMap videoData, bool updateIndex) {

  Q_EMIT nowPlayingUpdate(videoData);

  m_loadVideoThread = new LoadVideoThread(videoData);
  connect(m_loadVideoThread, &LoadVideoThread::finished, this,
          &PlayerItem::songStarted);

  m_loadVideoThread->start();
}

void PlayerItem::handleSongStart() { songStarted(true); }

void PlayerItem::previous() {
  if (historyIdx == 0)
    return;

  historyIdx--;

  Q_EMIT historyUpdate(history.count(), historyIdx);
  loadVideo(history[historyIdx], false);
}

void PlayerItem::playNext() {
  // if (nowPlayingId.length() == 0) {
  //   qDebug() << "No id for now playing";
  //   return;
  // }

  int searchCount = history.count() * 2;
  QStringList args;
  args << QStringLiteral("ytsearch%1:%2")
              .arg(searchCount)
              .arg(nowPlaying.value(QStringLiteral("title")).toString())
       << QStringLiteral("--flat-playlist") << QStringLiteral("--print")
       << QStringLiteral("%(id)s");

  QProcess process;
  process.startDetached(QStringLiteral("yt-dlp"), args);

  QString cmdString =
      QStringLiteral(
          "yt-dlp ytsearch%1:'%2' --flat-playlist --print "
          "'%(title)s||%(duration)s||%(ie_key)s||%(id)s||%(url)s||%(channel)s'")
          .arg(searchCount)
          .arg(nowPlaying.value(QStringLiteral("title")).toString());
  std::string output = playerItem::exec(&cmdString.toStdString()[0]);
  QString outputQString = QString::fromStdString(output);

  // Get the output, iterate over every id that the output has and the history.
  // If any string in the history is equal to that output, just skip for the
  // rests of each loop
  QStringList stringList = outputQString.split(QChar::fromLatin1('\n'));
  for (QString lineString : stringList) {

    QStringList lineStringList = lineString.split(QStringLiteral("||"));

    QString title = lineStringList[0];
    QString duration = lineStringList[1];
    QString type = lineStringList[2];
    QString id = lineStringList[3];
    QString url = lineStringList[4];
    QString channel = lineStringList[5];

    QVariantMap map;
    map.insert(QStringLiteral("title"), title);
    map.insert(QStringLiteral("id"), id);
    map.insert(QStringLiteral("duration"), duration);
    map.insert(QStringLiteral("type"), type);
    map.insert(QStringLiteral("url"), url);
    map.insert(QStringLiteral("channel"), channel);

    bool skip = false;
    for (QVariantMap video : history) {

      QString historyId = video.value(QStringLiteral("id")).toString();
      if (id.compare(historyId) == 0) {
        skip = true;
      }
      if (skip)
        continue;
    }
    if (skip)
      continue;

    loadVideo(map, true);
    return;
  }
}

void PlayerItem::next() {
  if (historyIdx == history.length() - 1) {
    playNext();
    return;
  }

  historyIdx++;

  Q_EMIT historyUpdate(history.count(), historyIdx);
  loadVideo(history[historyIdx], false);
}

PlayerItem::~PlayerItem() = default;
