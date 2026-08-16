#include "playerItem.h"
#include <QChar>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QString>
#include <QStringLiteral>
#include <QThread>
#include <cstdlib>
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

void MpvThread::run() {

  qDebug() << "Creating new process";
  m_mpvProcess = new QProcess();

  // Video is downloaded and stdout is piped to mpv in real time

  qDebug() << "setting process mode";
  m_mpvProcess->setProcessChannelMode(QProcess::MergedChannels);

  // We want to run a command like this: yt-dlp https://youtu.be/zq_VYh1SvuMM
  // -o
  // - | mpv --no-video -
  // QProcess can not run command line commands. Only a
  // single process. So we just load bash with the QProcess

  qDebug() << "Setting args";
  QStringList args;

  args << QStringLiteral("-c")
       << QStringLiteral(
              "mpv %1"
              "--title='%2' --input-ipc-server=/tmp/mpvsocket --no-video -")
              .arg(m_videoData.value(QStringLiteral("url")).toString(),
                   m_videoData.value(QStringLiteral("title")).toString());

  qDebug() << "Starting Process";
  m_mpvProcess->start(QStringLiteral("bash"), args);
  qDebug() << m_mpvProcess->processId();
  qDebug() << "Process should have started";
  qDebug() << m_mpvProcess->readAllStandardOutput();
  qDebug() << m_mpvProcess->readAllStandardError();
  m_mpvProcess->waitForFinished();
}

void MpvThread::quitProcess() {
  m_mpvProcess->terminate();
  m_mpvProcess->waitForFinished();
}

void WaitThread::run() {

  bool hasStarted = false;

  // Just set the string to something so it has a length and starts the loop
  // QString outputQString = QStringLiteral("NULL");
  while (true) {
    std::this_thread::sleep_for(1s);
    std::string outputString = playerItem::exec(
        "echo '{ \"command\": [\"get_property\", \"time-pos\"] }' | socat - "
        "/tmp/mpvsocket");
    QString outputQString = QString::fromStdString(outputString);

    if (outputQString.contains(QStringLiteral("data"))) {
      qDebug() << "Contains Data";
      if (!hasStarted) {
        qDebug() << "emitting Mpv Change";
        hasStarted = true;
        Q_EMIT mpvChange(true);
      }
      QJsonObject json =
          QJsonDocument::fromJson(outputQString.toUtf8()).object();
      QJsonValue time = json.value(QStringLiteral("data"));
      Q_EMIT timePos(time);
    } else if (hasStarted) {

      qDebug() << "Play Next Here";
      hasStarted = false;
    }
  }
};

PlayerItem::PlayerItem(QObject *parent) : QObject(parent), historyIdx(-1) {

  // Create cache dir if it does not exist
  char *p_username = getlogin();
  QString filePath = QStringLiteral("/home/%1/.cache/ytplayer").arg(p_username);

  QVariantMap nowPlaying;

  nowPlayingThread = nullptr;
  waitForFinishThread = nullptr;

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

void PlayerItem::loadVideo(QVariantMap videoData, bool updateIndex) {

  qDebug() << nowPlayingThread;

  if (nowPlayingThread != nullptr) {
    // if (nowPlayingThread->isRunning()) {
    qDebug() << "Exiting Thread";
    nowPlayingThread->exit();
    nowPlayingThread->terminate();
    nowPlayingThread->quitProcess();

    qDebug() << "Waiting";

    nowPlayingThread->wait();
    qDebug() << "Done";
  }

  if (updateIndex) {
    historyIdx++;
    history.push_back(videoData);
    Q_EMIT historyUpdate(history.count(), historyIdx);
  }

  nowPlaying = videoData;
  Q_EMIT nowPlayingUpdate(videoData);

  Q_EMIT playingStateChange(true);

  // Run mpv on this thread
  nowPlayingThread = new MpvThread(videoData);

  nowPlayingThread->start();
  qDebug() << "Thread should start";

  if (!waitForFinishThread) {
    qDebug() << "Creating new finish thread";
    // Watch the stdOut to see when above thread
    waitForFinishThread = new WaitThread();
    waitForFinishThread->start();

    // connect(waitForFinishThread, &WaitThread::finished, this,
    //         &PlayerItem::playNext);
    connect(waitForFinishThread, &WaitThread::mpvChange, this,
            &PlayerItem::mpvChange);
    connect(waitForFinishThread, &WaitThread::timePos, this,
            &PlayerItem::timePos);
  }
}

// void PlayerItem::quitMpvProcess() {}

void PlayerItem::mpvChange(bool mpvState) { Q_EMIT mpvStarted(mpvState); }
void PlayerItem::timePos(QJsonValue data) { Q_EMIT timeUpdate(data); };

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

void PlayerItem::previous() {
  if (historyIdx == 0)
    return;

  historyIdx--;

  Q_EMIT historyUpdate(history.count(), historyIdx);
  loadVideo(history[historyIdx], false);
}

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
          "'%(title)s||%(duration)s||%(ie_key)s||%(id)s||%(url)s||%(channel)s'")
          .arg(searchCount)
          .arg(nowPlaying.value(QStringLiteral("title")).toString());
  std::string output = playerItem::exec(&cmdString.toStdString()[0]);
  QString outputQString = QString::fromStdString(output);
  qDebug() << outputQString;

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

    qDebug() << "MAP : " << map;
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
