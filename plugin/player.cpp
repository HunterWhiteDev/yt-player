#include "player.h"
#include "playerItem.h"
#include <QtEnvironmentVariables>
#include <qglobal.h>
#include <qtenvironmentvariables.h>
#include <qurl.h>

void Pager::registerTypes(const char *uri) {
  qputenv("QT_AUDIO_BACKEND", "ALSA");
  Q_ASSERT(QLatin1String(uri) == QLatin1String("dev.hunterwhite.player"));
  qmlRegisterType<PlayerItem>(uri, 1, 0, "Player");
}
