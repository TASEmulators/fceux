// RetroAchievementsImages.cpp
//
#ifdef FCEUX_HAS_RETRO_ACHIEVEMENTS

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>

#include "Qt/RetroAchievements/RetroAchievementsImages.h"

//----------------------------------------------------------------------------
RaImageCache::RaImageCache(QObject *parent)
	: QObject(parent)
	, netMgr(new QNetworkAccessManager(this))
{
	cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/retroachievements";
}
//----------------------------------------------------------------------------
QString RaImageCache::cachePath(const QString &url) const
{
	const QByteArray hash = QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Sha1).toHex();

	return cacheDir + "/" + QString::fromLatin1(hash) + ".img";
}
//----------------------------------------------------------------------------
QPixmap RaImageCache::get(const QString &url)
{
	if (url.isEmpty())
	{
		return QPixmap();
	}
	auto it = memory.constFind(url);

	if (it != memory.constEnd())
	{
		return it.value();
	}
	QPixmap pixmap;

	if (pixmap.load(cachePath(url)))
	{
		memory.insert(url, pixmap);
		return pixmap;
	}
	if (pending.contains(url) || failed.contains(url))
	{
		return QPixmap();
	}
	pending.insert(url);

	QNetworkRequest req{ QUrl(url) };

	req.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
	req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
	req.setTransferTimeout(30000);
#endif

	QNetworkReply *reply = netMgr->get(req);

	connect(reply, &QNetworkReply::finished, this, [this, reply, url]()
	{
		const QByteArray data = reply->readAll();
		QPixmap image;

		pending.remove(url);
		reply->deleteLater();

		if ((reply->error() != QNetworkReply::NoError) || !image.loadFromData(data))
		{
			// Not retried until the next start, so a broken link is not requested over and over.
			failed.insert(url);
			return;
		}
		QDir().mkpath(cacheDir);

		QFile file(cachePath(url));

		if (file.open(QIODevice::WriteOnly))
		{
			file.write(data);
		}
		memory.insert(url, image);

		emit imageReady(url);
	});

	return QPixmap();
}
//----------------------------------------------------------------------------

#endif // FCEUX_HAS_RETRO_ACHIEVEMENTS
