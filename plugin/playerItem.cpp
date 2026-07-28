#include "playerItem.h"
#include <QChar>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QString>
#include <QStringLiteral>
#include <cstring>
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qdir.h>
#include <qlist.h>
#include <qlogging.h>
#include <qmap.h>
#include <qobject.h>
#include <qprocess.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qurl.h>
#include <qvariant.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
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

PlayerItem::PlayerItem(QObject *parent) : QObject(parent), historyIdx(0) {

  // Create cache dir if it does not exist
  char *p_username = getlogin();
  QString filePath = QStringLiteral("/home/%1/.cache/ytplayer").arg(p_username);

  bool dirExists = QDir(filePath).exists();
  if (!dirExists) {
    QDir(filePath).mkdir(QStringLiteral("."));
  }
}

QVariantMap PlayerItem::getSongFromAPI(QString videoId) {

  QVariantMap map;
  QStringList args;
  // Im using a double slash here so its unlikely the title will id will have it
  // when I call .split();
  args << QStringLiteral("%1").arg(videoId) << QStringLiteral("--print")
       << QStringLiteral("%(title)s//%(duration)s//%(ie_key)s//%(id)s//%(url)s/"
                         "/%(channel)s");

  QProcess fetchProcess;

  fetchProcess.setProcessChannelMode(QProcess::MergedChannels);
  fetchProcess.start(QStringLiteral("yt-dlp"), args);

  if (!fetchProcess.waitForStarted()) {
    qDebug() << "Failed to start! Error code: " << fetchProcess.error();
    return map;
  }

  // wait for finish
  if (!fetchProcess.waitForFinished()) {
    qDebug() << "Failed to finish! Error code: " << fetchProcess.error();
    return map;
  }

  QByteArray stdOut = fetchProcess.readAllStandardOutput();

  QString stdOutQString = QString::fromUtf8(stdOut);
  QStringList outputString = stdOutQString.split(QStringLiteral("//"));

  qDebug() << "ID: " << videoId;
  qDebug() << "SO: " << stdOutQString;
  qDebug() << "OS " << outputString;

  QString title = outputString[0];
  QString duration = outputString[1];
  QString type = outputString[2];

  QString id = outputString[3];
  QString url = outputString[4];
  QString channel = outputString[5];

  qDebug() << "T: " << title << "ID: " << id << "CHANNEL: " << channel;

  map.insert(QStringLiteral("title"), title);
  map.insert(QStringLiteral("id"), id);
  map.insert(QStringLiteral("duration"), duration);
  map.insert(QStringLiteral("type"), type);
  map.insert(QStringLiteral("url"), url);
  map.insert(QStringLiteral("channel"), channel);

  return map;
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

void PlayerItem::loadVideo(QString id) {

  mpvProcess.close();
  mpvProcess.kill();

  QVariantMap videoData = getSongFromAPI(id);

  // Video is downloaded and stdout is piped to mpv in real time
  mpvProcess.setProcessChannelMode(QProcess::MergedChannels);

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

  // history.push_back(videoData.value(QStringLiteral("id")).toString());
  // historyIdx = history.count() - 1;

  Q_EMIT nowPlayingUpdate(videoData);
  Q_EMIT playingStateChange(true);

  mpvProcess.start(QStringLiteral("bash"), args);
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

void PlayerItem::previous() {
  if (historyIdx == 0)
    return;

  loadVideo(history.at(historyIdx - 1));
  historyIdx--;
}

// This call was giving me a hard time with QProcess so I just went with a more
// stand libary approach
void PlayerItem::next(QString videoTitle) {

  std::string execString = "yt-dlp ytsearch1:'Videos related to ";
  execString.append(videoTitle.toStdString());
  execString.append("' --flat-playlist --print '%(id)s'");

  std::string stdOut = exec(&execString[0]);
  QString qStringStdOut = QString::fromStdString(stdOut);
  QStringList qStringList = qStringStdOut.split(QChar::fromLatin1('\n'));
  QString id = qStringList[0];

  // TODO: Make this look for 5 or so and then choose the one that is not the
  // currently playing song.

  loadVideo(id);
}

PlayerItem::~PlayerItem() = default;
