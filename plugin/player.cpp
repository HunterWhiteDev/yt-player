#include "player.h"
#include "playerItem.h"
#include <qglobal.h>

void Pager::registerTypes(const char *uri) {
  Q_ASSERT(QLatin1String(uri) == QLatin1String("dev.hunterwhite.player"));
  qmlRegisterType<PlayerItem>(uri, 1, 0, "Player");
}
