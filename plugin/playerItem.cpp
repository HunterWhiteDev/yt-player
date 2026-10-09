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
  //
  remove("/tmp/video");
  remove("/tmp/video.mp3");
  remove("/tmp/video.webm");

  m_ytdlpProcess = new QProcess();
  m_ytdlpProcess->setProcessChannelMode(QProcess::MergedChannels);

  QStringList args;
  args << QStringLiteral("-c")
       << QStringLiteral("yt-dlp -x %1 -t mp3 -o /tmp/video.mp3")
              .arg(m_videoData.value(QStringLiteral("url")).toString());

  // Connect to update loading status
  connect(m_ytdlpProcess, &QProcess::readyReadStandardOutput, [this]() {
    QString output = QString::fromUtf8(m_ytdlpProcess->readAllStandardOutput());

    // qDebug() << output;
    // File has been downloaded and converted to mp3 file on disk
    if (output.contains(QStringLiteral("\r[download]"))) {
      Q_EMIT updateLoadingStatus(output.slice(11, 4));
    }
  });

  m_ytdlpProcess->start(QStringLiteral("bash"), args);
  m_ytdlpProcess->waitForFinished();

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

PlayerItem::PlayerItem(QObject *parent) : QObject(parent), historyIdx(0) {

  // Create cache dir if it does not exist
  char *p_username = getlogin();
  QString filePath = QStringLiteral("/home/%1/.cache/ytplayer").arg(p_username);

  QVariantMap nowPlaying;

  bool dirExists = QDir(filePath).exists();
  if (!dirExists) {
    QDir(filePath).mkdir(QStringLiteral("."));
  }
}

void PlayerItem::search(QString input) {

  m_searchThread = new SearchThread(input);
  m_searchThread->start();

  connect(m_searchThread, &SearchThread::finished, this,
          &PlayerItem::searchUpdate);
}

// If updateIndex is passed, we move the history index to the last  position.
// Other wise we handle it in the next() or previous() functions

void PlayerItem::loadVideo(QVariantMap videoData, bool addToHistory) {

  Q_EMIT nowPlayingUpdate(videoData);

  m_loadVideoThread = new LoadVideoThread(videoData);
  connect(m_loadVideoThread, &LoadVideoThread::finished, this,
          &PlayerItem::songStarted);

  connect(m_loadVideoThread, &LoadVideoThread::updateLoadingStatus, this,
          &PlayerItem::updateLoadingStatus);

  m_loadVideoThread->start();

  if (addToHistory) {
    history.push_back(videoData);
    historyIdx = history.size() - 1;
    historyUpdate(history, historyIdx);
  }
  nowPlaying = videoData;
}

void PlayerItem::handleSongStart() { songStarted(true); }

void PlayerItem::previous() {
  if (historyIdx == 0)
    return;

  historyIdx--;

  Q_EMIT historyUpdate(history, historyIdx);
  loadVideo(history[historyIdx], false);
}

void PlayerItem::playNext() {

  int searchCount = history.count() + 1;
  QStringList args;
  args << QStringLiteral("ytsearch%1:%2")
              .arg(searchCount)
              .arg(nowPlaying.value(QStringLiteral("title")).toString())
       << QStringLiteral("--flat-playlist") << QStringLiteral("--print")
       << QStringLiteral(
              "%(title)s*%(duration)s*%(ie_key)s*%(id)s*%(url)s*%(channel)s");

  qDebug() << args;

  QProcess *process = new QProcess;

  process->setProcessChannelMode(QProcess::MergedChannels);
  connect(process, &QProcess::readyReadStandardOutput, [process, this]() {
    QString output = QString::fromUtf8(process->readAllStandardOutput());
    QStringList lineStringList =
        output.slice(0, output.size() - 1).split(QStringLiteral("*"));

    // if (id != nowPlaying.value(QStringLiteral("id")).toString()) {

    QString id = lineStringList[3];

    // Look for entry again in the history
    bool found = false;
    for (QVariantMap &historyElement : history) {
      if (historyElement.value(QStringLiteral("id")) == id) {
        found = true;
        break;
      }
    }

    // If the current search entry was not found in the history, it's a new
    // video. Play it
    if (!found) {
      QString title = lineStringList[0];
      QString duration = lineStringList[1];
      QString type = lineStringList[2];
      QString url = lineStringList[4];
      QString channel = lineStringList[5];

      QVariantMap newData;
      newData.insert(QStringLiteral("title"), title);
      newData.insert(QStringLiteral("duration"), duration);
      newData.insert(QStringLiteral("type"), type);
      newData.insert(QStringLiteral("url"), url);
      newData.insert(QStringLiteral("channel"), channel);
      newData.insert(QStringLiteral("id"), id);

      qDebug() << newData;
      loadVideo(newData, true);

      process->close();
      process->terminate();
    }
  });

  process->start(QStringLiteral("yt-dlp"), args);
  process->waitForFinished();
}

void PlayerItem::next() {
  qDebug() << historyIdx;
  qDebug() << history.length() - 1;
  if (historyIdx == history.length() - 1) {
    qDebug() << "Running Play Next";
    playNext();
  } else {
    historyIdx++;

    Q_EMIT historyUpdate(history, historyIdx);
    loadVideo(history[historyIdx], false);
  }
}

void PlayerItem::setHistoryIdx(int idx) {
  qDebug() << "Setting Index to : " << idx;
  historyIdx = idx;
  historyUpdate(history, idx);
}

PlayerItem::~PlayerItem() = default;
