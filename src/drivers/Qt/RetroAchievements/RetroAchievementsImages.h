// RetroAchievementsImages.h
//
// Downloads and caches the badge and avatar images that RetroAchievements links to.
// Images are kept in memory and in a cache directory so each one is downloaded once.
#pragma once

#ifdef FCEUX_HAS_RETRO_ACHIEVEMENTS

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QPixmap>
#include <QSet>
#include <QString>

class QNetworkAccessManager;

class RaImageCache : public QObject
{
	Q_OBJECT

	public:
		explicit RaImageCache(QObject *parent = nullptr);

		void setUserAgent(const QByteArray &agent) { userAgent = agent; }

		// GUI thread only. Returns a null pixmap while the image is not available yet,
		// and emits imageReady once it has been downloaded.
		QPixmap get(const QString &url);

		// GUI thread only. Downloads the image again even when a copy is cached, because the picture
		// behind the URL can change. The cached copy stays in use until the new one has arrived.
		void refresh(const QString &url);

	signals:
		void imageReady(const QString &url);

	private:
		QString cachePath(const QString &url) const;
		void download(const QString &url);

		QNetworkAccessManager *netMgr;
		QByteArray userAgent;
		QString cacheDir;
		QHash<QString, QPixmap> memory;
		QSet<QString> pending;
		QSet<QString> failed;
};

#endif // FCEUX_HAS_RETRO_ACHIEVEMENTS
