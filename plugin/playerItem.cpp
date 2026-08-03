#include "playerItem.h"
#include <QChar>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QString>
#include <QStringLiteral>
#include <QThread>
#include <cstring>
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qdir.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qlist.h>
#include <qlogging.h>
#include <qmap.h>
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
#include <thread>
#include <unistd.h>

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

PlayerItem::PlayerItem(QObject *parent) : QObject(parent), historyIdx(-1) {

  // Create cache dir if it does not exist
  char *p_username = getlogin();
  QString filePath = QStringLiteral("/home/%1/.cache/ytplayer").arg(p_username);

  QProcess *mpvProcess = new QProcess;

  QVariantMap nowPlaying;
  // nowPlayingThread = new QThread();
  // waitForFinishThread = new QThread();

  bool dirExists = QDir(filePath).exists();
  if (!dirExists) {
    QDir(filePath).mkdir(QStringLiteral("."));
  }

  QVector<QString> history;
}

void PlayerItem::search(QString input) {

  QStringList args;
  args << QStringLiteral("ytsearch7:%1").arg(input)
       << QStringLiteral("--flat-playlist") << QStringLiteral("--print")
       << QStringLiteral(
              "%(title)s_%(duration)s_%(ie_key)s_%(id)s_%(url)s_%(channel)s");
  QProcess process;
  process.setProcessChannelMode(QProcess::MergedChannels);
  process.start(QStringLiteral("yt-dlp"), args);

  // make sure it starts
  if (!process.waitForStarted()) {
    qDebug() << "Failed to start! Error code: " << process.error();
    return;
  }

  // wait for finish
  if (!process.waitForFinished()) {
    qDebug() << "Failed to finish! Error code: " << process.error();
    return;
  }

  // read back and return
  QByteArray stdOut = process.readAllStandardOutput();
  QString stdOutQString = QString::fromUtf8(stdOut);

  QStringList outputStringList = stdOutQString.split(QChar::fromLatin1('\n'));

  QVariantList searchResults;

  for (int i = 0; i < outputStringList.count(); i++) {

    QStringList lineStringList = outputStringList[i].split(QStringLiteral("_"));

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
  lastSearchResults = searchResults;
  Q_EMIT searchUpdate(searchResults);
}

// If updateIndex is passed, we move the history index to the last  position.
// Other wise we handle it in the next() or previous() functions
void loadVideoWork(QVariantMap videoData, QProcess *mpvProcess) {

  mpvProcess = new QProcess();

  // QVariantMap videoData = getSongFromAPI(id);

  // Video is downloaded and stdout is piped to mpv in real time
  mpvProcess->setProcessChannelMode(QProcess::MergedChannels);

  // We want to run a command like this: yt-dlp https://youtu.be/zq_VYh1SvuMM
  // -o
  // - | mpv --no-video -
  // QProcess can not run command line commands. Only a
  // single process. So we just load bash with the QProcess
  QStringList args;

  args << QStringLiteral("-c")
       << QStringLiteral(
              "yt-dlp %1 -o - | mpv  "
              "--title='%2' --input-ipc-server=/tmp/mpvsocket --no-video -")
              .arg(videoData.value(QStringLiteral("id")).toString(),
                   videoData.value(QStringLiteral("title")).toString());

  // nowPlayingId = id;
  mpvProcess->start(QStringLiteral("bash"), args);
  mpvProcess->waitForFinished();
}

void waitForFinish() {

  while (true) {

    std::this_thread::sleep_for(1s);
    std::string outputString = exec(
        "echo '{ \"command\": [\"get_property\", \"time-pos\"] }' | socat - "
        "/tmp/mpvsocket");
    QString outputQString = QString::fromStdString(outputString);

    qDebug() << outputQString;

    if (outputQString.contains(QStringLiteral("Connection refused"))) {
      // No connection so socket must have ended, end this loop so the thread
      // can finish and we can play a new song
      return;
    }
  }
}

void PlayerItem::loadVideo(QVariantMap videoData, bool updateIndex) {

  qDebug() << "PROCESS STATE -> " << mpvProcess->state();

  if (updateIndex) {
    historyIdx++;
    history.push_back(videoData);
    Q_EMIT historyUpdate(history.count(), historyIdx);
  }

  nowPlaying = videoData;
  Q_EMIT nowPlayingUpdate(videoData);

  Q_EMIT playingStateChange(true);

  // Run mpv on this thread
  nowPlayingThread = QThread::create(loadVideoWork, videoData, &mpvProcess);
  nowPlayingThread->start();

  // Watch the stdOut to see when above thread
  waitForFinishThread = QThread::create(waitForFinish);
  waitForFinishThread->start();

  // connect(waitForFinishThread, &QThread::finished, this, [this]() {
  //   waitForFinishThread->quit();
  //   nowPlayingThread->quit();
  //   nowPlayingThread->deleteLater();
  //   waitForFinishThread->deleteLater();
  // });
}

void PlayerItem::pause() {
  // MPV can be controlled via sockets to /tmp
  // https://stackoverflow.com/questions/35013075/pause-programmatically-video-player-mpv

  QStringList args;
  args << QStringLiteral("-c")
       << QStringLiteral("echo '{ \"command\": [\"set_property\", \"pause\", "
                         "true] }' | socat - /tmp/mpvsocket");
  QProcess process;

  process.setProcessChannelMode(QProcess::MergedChannels);
  process.startDetached(QStringLiteral("bash"), args);
  Q_EMIT playingStateChange(false);
}

void PlayerItem::play() {
  // MPV can be controlled via sockets to /tmp
  // https://stackoverflow.com/questions/35013075/pause-programmatically-video-player-mpv

  QStringList args;
  args << QStringLiteral("-c")
       << QStringLiteral("echo '{ \"command\": [\"set_property\", \"pause\", "
                         "false] }' | socat - /tmp/mpvsocket");
  QProcess process;

  process.setProcessChannelMode(QProcess::MergedChannels);
  process.startDetached(QStringLiteral("bash"), args);

  Q_EMIT playingStateChange(true);
}

// void PlayerItem::previous() {
//   if (historyIdx == 0)
//     return;
//
//   historyIdx--;
//
//   Q_EMIT historyUpdate(history.count(), historyIdx);
//   loadVideo(history[historyIdx], false);
// }

/// Get a random "recomended video";
void PlayerItem::playNext() {
  // if (nowPlayingId.length() == 0) {
  //   qDebug() << "No id for now playing";
  //   return;
  // }

  // QVariantMap songMeta = getSongFromAPI(nowPlayingId);

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
          "'%(title)s_%(duration)s_%(ie_key)s_%(id)s_%(url)s_%(channel)s'")
          .arg(searchCount)
          .arg(nowPlaying.value(QStringLiteral("title")).toString());
  std::string output = exec(&cmdString.toStdString()[0]);
  QString outputQString = QString::fromStdString(output);
  qDebug() << outputQString;

  // Get the output, iterate over every id that the output has and the history.
  // If any string in the history is equal to that output, just skip for the
  // rests of each loop
  QStringList stringList = outputQString.split(QChar::fromLatin1('\n'));
  for (QString lineStringList : stringList) {

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

// void PlayerItem::next() {
//   if (historyIdx == history.length() - 1) {
//     playNext();
//     return;
//   }
//
//   historyIdx++;
//
//   Q_EMIT historyUpdate(history.count(), historyIdx);
//   loadVideo(history[historyIdx], false);
// }

PlayerItem::~PlayerItem() = default;
