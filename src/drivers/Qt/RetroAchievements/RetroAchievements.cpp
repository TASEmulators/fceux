// RetroAchievements.cpp
//
// RetroAchievements (rcheevos rc_client) service for the Qt GUI.
//
#ifdef FCEUX_HAS_RETRO_ACHIEVEMENTS

#include <cstdio>
#include <cstring>

#include <QEventLoop>
#include <QMetaObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSysInfo>
#include <QUrl>

#include <rc_consoles.h>

#include "../../../types.h"
#include "../../../fceu.h"
#include "../../../cheat.h"
#include "../../../driver.h"
#include "../../../version.h"

#include "Qt/config.h"
#include "Qt/fceuWrapper.h"
#include "Qt/RetroAchievements/RetroAchievements.h"
#include "Qt/RetroAchievements/RetroAchievementsImages.h"
#include "Qt/RetroAchievements/RetroAchievementsOverlay.h"

static RetroAchievements *raInstance = nullptr;

// Declared in rcheevos' internal rc_version.h, whose location differs between rcheevos releases.
RC_BEGIN_C_DECLS
RC_EXPORT const char* rc_version_string(void);
RC_END_C_DECLS

static const char *cfgEnable = "SDL.RetroAchEnable";
static const char *cfgUser   = "SDL.RetroAchUser";
static const char *cfgToken  = "SDL.RetroAchToken";
static const char *cfgDebug  = "SDL.RetroAchDebug";

//----------------------------------------------------------------------------
RetroAchievements::RetroAchievements(void)
	: QObject(nullptr)
	, client(nullptr)
	, netMgr(new QNetworkAccessManager(this))
	, imageCache(new RaImageCache(this))
	, idleTimer(this)
	, loginState(LOGGED_OUT)
	, gameActive(false)
	, debugLog(false)
	, configSavePending(false)
	, generation(0)
	, inFlight(0)
	, shuttingDown(false)
{
	bool enable = false;

	qRegisterMetaType<RaLeaderboardEntry>();
	qRegisterMetaType<QList<RaLeaderboardEntry> >();

	connect(&idleTimer, &QTimer::timeout, this, &RetroAchievements::idle);

	// Last resort when a request does not finish in time.
	shutdownTimer.setSingleShot(true);
	connect(&shutdownTimer, &QTimer::timeout, this, &RetroAchievements::finishShutdown);

	idleTimer.setInterval(250);

	g_config->getOption(cfgEnable, &enable);
	g_config->getOption(cfgDebug, &debugLog);

	if (enable)
	{
		initClient();
	}
}
//----------------------------------------------------------------------------
RetroAchievements::~RetroAchievements(void)
{
	finishShutdown();
}
//----------------------------------------------------------------------------
void RetroAchievements::create(void)
{
	if (raInstance == nullptr)
	{
		raInstance = new RetroAchievements();
	}
}
//----------------------------------------------------------------------------
void RetroAchievements::destroy(void)
{
	if (raInstance != nullptr)
	{
		// Gives an unlock that was just submitted the time to reach the server.
		raInstance->drainBlocking(3000);
	}
	delete raInstance;
	raInstance = nullptr;
}
//----------------------------------------------------------------------------
RetroAchievements *RetroAchievements::instance(void)
{
	return raInstance;
}
//----------------------------------------------------------------------------
const char *RetroAchievements::libraryVersion(void)
{
	return rc_version_string();
}
//----------------------------------------------------------------------------
void RetroAchievements::initClient(void)
{
	char clause[64];
	std::string user, token;

	if (client != nullptr)
	{
		return;
	}
	client = rc_client_create(&RetroAchievements::readMemory, &RetroAchievements::serverCall);

	rc_client_set_userdata(client, this);
	rc_client_enable_logging(client, RC_CLIENT_LOG_LEVEL_INFO, &RetroAchievements::logMessage);
	rc_client_set_event_handler(client, &RetroAchievements::eventHandler);

	// Hardcore stays off until the features it forbids are blocked while it is active.
	rc_client_set_hardcore_enabled(client, 0);

	clause[0] = 0;
	rc_client_get_user_agent_clause(client, clause, sizeof(clause));

	// The server needs a numeric version here, so FCEU_VERSION_STRING with its suffixes is not used.
	userAgent = QString("fceux/%1.%2.%3 (%4 %5) %6")
		.arg(FCEU_VERSION_MAJOR).arg(FCEU_VERSION_MINOR).arg(FCEU_VERSION_PATCH)
		.arg(QSysInfo::kernelType()).arg(QSysInfo::kernelVersion())
		.arg(QString::fromLatin1(clause)).toUtf8();

	imageCache->setUserAgent(userAgent);
	indicators.reset(new RaOverlayIndicators());

	idleTimer.start();

	g_config->getOption(cfgUser, &user);
	g_config->getOption(cfgToken, &token);

	if (!user.empty() && !token.empty())
	{
		loginWithToken(user, token);
	}
	else
	{
		FCEU_printf("RetroAchievements: enabled, but not logged in.\n");
	}
}
//----------------------------------------------------------------------------
void RetroAchievements::beginShutdown(void)
{
	if ((client == nullptr) || shuttingDown)
	{
		return;
	}
	if (inFlight == 0)
	{
		finishShutdown();
		return;
	}
	FCEU_printf("RetroAchievements: waiting for %i pending request(s) to finish.\n", inFlight);

	// Frames are no longer evaluated, and nothing new is scheduled, but the requests in flight
	// (and what they trigger) are still answered.
	shuttingDown = true;
	gameActive = false;
	idleTimer.stop();

	if (indicators)
	{
		indicators->clear();
	}
	shutdownTimer.start(3000);
}
//----------------------------------------------------------------------------
void RetroAchievements::drainBlocking(int timeoutMs)
{
	if ((client == nullptr) || (inFlight == 0))
	{
		return;
	}
	QEventLoop loop;
	QTimer timeout;

	FCEU_printf("RetroAchievements: waiting for %i pending request(s) to finish.\n", inFlight);

	// Same as beginShutdown: no frames, nothing new scheduled, and no game identification after a login.
	shuttingDown = true;
	gameActive = false;
	idleTimer.stop();

	timeout.setSingleShot(true);
	connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
	connect(this, &RetroAchievements::requestsDrained, &loop, &QEventLoop::quit);

	timeout.start(timeoutMs);

	// The replies are processed here with the emulator lock already held by this thread.
	loop.exec(QEventLoop::ExcludeUserInputEvents);
}
//----------------------------------------------------------------------------
void RetroAchievements::finishShutdown(void)
{
	FCEU_WRAPPER_SCOPED_LOCK();

	if (client == nullptr)
	{
		shuttingDown = false;
		return;
	}
	shutdownTimer.stop();
	idleTimer.stop();

	// Drop requests that are still in flight so none of them calls back into the destroyed client.
	++generation;

	const QList<QNetworkReply*> replies = netMgr->findChildren<QNetworkReply*>();

	for (QNetworkReply *reply : replies)
	{
		reply->disconnect(this);
		reply->abort();
		reply->deleteLater();
	}

	rc_client_destroy(client);
	client = nullptr;

	// rc_client does not call back for these once it is destroyed.
	for (LeaderboardFetch *fetch : leaderboardFetches)
	{
		delete fetch;
	}
	leaderboardFetches.clear();

	inFlight = 0;
	shuttingDown = false;
	indicators.reset();
	gameActive = false;
	releaseRomImage();

	setLoginState(LOGGED_OUT);
}
//----------------------------------------------------------------------------
void RetroAchievements::requestAnswered(void)
{
	if (inFlight > 0)
	{
		--inFlight;
	}
	if (inFlight == 0)
	{
		emit requestsDrained();

		if (shuttingDown)
		{
			// Not from inside the reply handler.
			QMetaObject::invokeMethod(this, &RetroAchievements::finishShutdown, Qt::QueuedConnection);
		}
	}
}
//----------------------------------------------------------------------------
void RetroAchievements::setEnabled(bool enable)
{
	FCEU_WRAPPER_SCOPED_LOCK();

	g_config->setOption(cfgEnable, enable ? 1 : 0);
	saveConfigLater();

	if (enable)
	{
		if (shuttingDown)
		{
			finishShutdown();
		}
		initClient();
	}
	else
	{
		beginShutdown();
	}
}
//----------------------------------------------------------------------------
void RetroAchievements::setLoginState(LoginState state)
{
	QString userName;

	loginState = state;

	if (client != nullptr)
	{
		const rc_client_user_t *user = rc_client_get_user_info(client);

		if ((state == LOGGED_IN) && (user != nullptr))
		{
			userName = QString::fromUtf8(user->display_name);
		}
	}
	loggedInUser = userName;

	emit loginStateChanged(static_cast<int>(state), userName);
}
//----------------------------------------------------------------------------
void RetroAchievements::loginWithToken(const std::string &user, const std::string &token)
{
	setLoginState(LOGIN_IN_PROGRESS);

	rc_client_begin_login_with_token(client, user.c_str(), token.c_str(), &RetroAchievements::loginCallback, this);
}
//----------------------------------------------------------------------------
void RetroAchievements::loginWithPassword(const QString &user, const QString &password)
{
	FCEU_WRAPPER_SCOPED_LOCK();

	if (client == nullptr)
	{
		FCEU_printf("RetroAchievements: not enabled.\n");
		return;
	}
	setLoginState(LOGIN_IN_PROGRESS);

	const QByteArray userUtf8 = user.toUtf8();
	const QByteArray passUtf8 = password.toUtf8();

	rc_client_begin_login_with_password(client, userUtf8.constData(), passUtf8.constData(),
		&RetroAchievements::loginCallback, this);
}
//----------------------------------------------------------------------------
void RetroAchievements::logout(void)
{
	FCEU_WRAPPER_SCOPED_LOCK();

	g_config->setOption(cfgToken, "");
	saveConfigLater();

	if (client != nullptr)
	{
		rc_client_logout(client);
		gameActive = false;
		releaseRomImage();
		setLoginState(LOGGED_OUT);
	}
}
//----------------------------------------------------------------------------
void RC_CCONV RetroAchievements::loginCallback(int result, const char *errorMessage, rc_client_t *client, void *userdata)
{
	RetroAchievements *self = static_cast<RetroAchievements*>(userdata);

	if (result != RC_OK)
	{
		const QString message = QString::fromUtf8(errorMessage ? errorMessage : "unknown error");

		if (result == RC_INVALID_CREDENTIALS)
		{
			// The stored token was rejected; forget it so it is not retried on every start.
			g_config->setOption(cfgToken, "");
			self->saveConfigLater();
		}
		self->setLoginState(LOGGED_OUT);

		emit self->loginFailed(message);
		return;
	}
	const rc_client_user_t *user = rc_client_get_user_info(client);

	FCEU_printf("RetroAchievements: logged in as %s (%u points)\n", user->display_name, user->score);

	std::string storedUser, storedToken;

	g_config->getOption(cfgUser, &storedUser);
	g_config->getOption(cfgToken, &storedToken);

	if ((storedUser != user->username) || (storedToken != user->token))
	{
		g_config->setOption(cfgUser, user->username);
		g_config->setOption(cfgToken, user->token);
		self->saveConfigLater();
	}
	self->setLoginState(LOGGED_IN);

	// A game may have been loaded while the login was still in progress.
	self->identifyGame();
}
//----------------------------------------------------------------------------
void RetroAchievements::identifyGame(void)
{
	std::vector<uint8_t> image;

	if ((client == nullptr) || shuttingDown || (loginState != LOGGED_IN) || (GameInfo == nullptr))
	{
		return;
	}
	if (!FCEU_ReadLoadedRomImage(image))
	{
		FCEU_printf("RetroAchievements: could not read the game file again to identify it.\n");
		return;
	}
	// A load that is still running is aborted inside the call below. Keeping its buffer alive until
	// the call returns (swapping a vector does not move the data) means nothing can see it freed.
	std::vector<uint8_t> previous;

	previous.swap(romImage);
	romImage.swap(image);
	romName = GameInfo->filename ? GameInfo->filename : "";

	rc_client_begin_identify_and_load_game(client, RC_CONSOLE_UNKNOWN, romName.c_str(),
		romImage.data(), romImage.size(), &RetroAchievements::loadGameCallback, this);
}
//----------------------------------------------------------------------------
void RetroAchievements::releaseRomImage(void)
{
	romImage.clear();
	romImage.shrink_to_fit();
	romName.clear();
}
//----------------------------------------------------------------------------
void RC_CCONV RetroAchievements::loadGameCallback(int result, const char *errorMessage, rc_client_t *client, void *userdata)
{
	RetroAchievements *self = static_cast<RetroAchievements*>(userdata);

	if (result == RC_ABORTED)
	{
		return;
	}
	// Identification is over, rc_client no longer reads the ROM.
	self->releaseRomImage();

	if (result != RC_OK)
	{
		self->gameActive = false;
		FCEU_printf("RetroAchievements: game not loaded: %s\n", errorMessage ? errorMessage : "unknown error");
		return;
	}
	const rc_client_game_t *game = rc_client_get_game_info(client);
	rc_client_user_game_summary_t summary;

	rc_client_get_user_game_summary(client, &summary);

	self->gameActive = true;

	FCEU_printf("RetroAchievements: loaded '%s': %u of %u achievements unlocked, %u of %u points\n",
		game->title, summary.num_unlocked_achievements, summary.num_core_achievements,
		summary.points_unlocked, summary.points_core);

	emit self->gameIdentified(game->id, QString::fromUtf8(game->title));
}
//----------------------------------------------------------------------------
void RetroAchievements::gameLoaded(void)
{
	// Identified now when logged in, otherwise by the login callback.
	gameActive = false;

	identifyGame();
}
//----------------------------------------------------------------------------
void RetroAchievements::gameClosed(void)
{
	releaseRomImage();

	if (client == nullptr)
	{
		return;
	}
	rc_client_unload_game(client);

	indicators->clear();

	gameActive = false;

	emit gameUnloaded();
}
//----------------------------------------------------------------------------
void RetroAchievements::frame(void)
{
	if ((client != nullptr) && gameActive)
	{
		rc_client_do_frame(client);
	}
}
//----------------------------------------------------------------------------
void RetroAchievements::reset(void)
{
	if ((client != nullptr) && gameActive)
	{
		rc_client_reset(client);
	}
}
//----------------------------------------------------------------------------
bool RetroAchievements::serializeProgress(std::vector<uint8_t> &data)
{
	if ((client == nullptr) || !gameActive)
	{
		return false;
	}
	const size_t size = rc_client_progress_size(client);

	if (size == 0)
	{
		return false;
	}
	data.resize(size);

	if (rc_client_serialize_progress_sized(client, data.data(), size) != RC_OK)
	{
		data.clear();
		return false;
	}
	if (debugLog)
	{
		FCEU_printf("RetroAchievements: savestate: stored %zu bytes of progress\n", size);
	}
	return true;
}
//----------------------------------------------------------------------------
void RetroAchievements::deserializeProgress(const uint8_t *data, size_t size)
{
	if ((client != nullptr) && gameActive)
	{
		const int result = rc_client_deserialize_progress_sized(client, data, size);

		if (debugLog)
		{
			if (data == nullptr)
			{
				FCEU_printf("RetroAchievements: savestate: no progress data, tracking restarted\n");
			}
			else
			{
				FCEU_printf("RetroAchievements: savestate: restored %zu bytes of progress (%s)\n",
					size, (result == RC_OK) ? "ok" : "failed, tracking restarted");
			}
		}
	}
}
//----------------------------------------------------------------------------
void RetroAchievements::idle(void)
{
	// rc_client_do_frame already runs this work while frames are emulated, so it only matters while
	// paused. Do not make the emulator wait for it, and just try again at the next tick when it is busy.
	if (!FCEU_WRAPPER_TRYLOCK(2))
	{
		return;
	}
	if (client != nullptr)
	{
		rc_client_idle(client);
	}
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------------------------------
void RetroAchievements::saveConfigLater(void)
{
	if (configSavePending.exchange(true))
	{
		return;
	}
	QMetaObject::invokeMethod(this, [this]()
	{
		configSavePending = false;
		g_config->save();
	}, Qt::QueuedConnection);
}
//----------------------------------------------------------------------------
void RC_CCONV RetroAchievements::serverCall(const rc_api_request_t *request,
	rc_client_server_callback_t callback, void *callbackData, rc_client_t *client)
{
	RetroAchievements *self = static_cast<RetroAchievements*>(rc_client_get_userdata(client));

	// Called with the emulator lock held from either thread. The network manager
	// lives on the GUI thread, so the request is always queued over to it.
	const QByteArray url         = QByteArray(request->url);
	const QByteArray postData    = request->post_data    ? QByteArray(request->post_data)    : QByteArray();
	const QByteArray contentType = request->content_type ? QByteArray(request->content_type) : QByteArray();
	const unsigned int gen       = self->generation;

	++self->inFlight;

	QMetaObject::invokeMethod(self, [=]()
	{
		if (gen == self->generation)
		{
			self->startRequest(url, postData, contentType, callback, callbackData);
		}
	}, Qt::QueuedConnection);
}
//----------------------------------------------------------------------------
void RetroAchievements::startRequest(const QByteArray &url, const QByteArray &postData,
	const QByteArray &contentType, rc_client_server_callback_t callback, void *callbackData)
{
	QNetworkRequest req{ QUrl::fromEncoded(url) };
	QNetworkReply *reply;

	req.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
	req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
	req.setTransferTimeout(30000);
#endif

	if (postData.isNull())
	{
		reply = netMgr->get(req);
	}
	else
	{
		req.setHeader(QNetworkRequest::ContentTypeHeader,
			contentType.isEmpty() ? QByteArray("application/x-www-form-urlencoded") : contentType);
		reply = netMgr->post(req, postData);
	}

	connect(reply, &QNetworkReply::finished, this, [this, reply, callback, callbackData]()
	{
		const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		const QByteArray body = reply->readAll();
		const QByteArray error = reply->errorString().toUtf8();
		rc_api_server_response_t response;

		memset(&response, 0, sizeof(response));

		if (status == 0)
		{
			// No HTTP response at all (timeout, no connection, ...). Let rc_client retry.
			response.body = error.constData();
			response.body_length = static_cast<size_t>(error.size());
			response.http_status_code = RC_API_SERVER_RESPONSE_RETRYABLE_CLIENT_ERROR;
		}
		else
		{
			response.body = body.constData();
			response.body_length = static_cast<size_t>(body.size());
			response.http_status_code = status;
		}
		reply->deleteLater();

		FCEU_WRAPPER_SCOPED_LOCK();

		callback(&response, callbackData);

		requestAnswered();
	});
}
//----------------------------------------------------------------------------
uint32_t RC_CCONV RetroAchievements::readMemory(uint32_t address, uint8_t *buffer, uint32_t numBytes, rc_client_t *)
{
	// Always called with the emulator lock held, inside the frame hook or a load/login callback.
	if ((GameInfo == nullptr) || (RAM == nullptr))
	{
		return 0;
	}

	for (uint32_t i = 0; i < numBytes; i++)
	{
		const uint32_t addr = address + i;

		if (addr < 0x2000)
		{
			buffer[i] = RAM[addr & 0x7FF];
		}
		else if ((addr >= 0x6000) && (addr <= 0xFFFF))
		{
			// Debugger style read, safe from mapper side effects.
			buffer[i] = static_cast<uint8_t>(FCEU_CheatGetByte(addr));
		}
		else
		{
			// PPU/APU registers and mapper registers can not be read without side effects.
			return 0;
		}
	}
	return numBytes;
}
//----------------------------------------------------------------------------
void RC_CCONV RetroAchievements::logMessage(const char *message, const rc_client_t *)
{
	FCEU_printf("RetroAchievements: %s\n", message);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QString shortText(const char *text)
{
	QString s = QString::fromUtf8(text ? text : "");

	if (s.size() > 32)
	{
		s = s.left(29) + "...";
	}
	return s;
}
//----------------------------------------------------------------------------
RaGameSummary RetroAchievements::getGameSummary(void)
{
	RaGameSummary summary;

	FCEU_WRAPPER_SCOPED_LOCK();

	if ((client == nullptr) || !gameActive)
	{
		return summary;
	}
	const rc_client_game_t *game = rc_client_get_game_info(client);
	rc_client_user_game_summary_t stats;
	char presence[256];

	rc_client_get_user_game_summary(client, &stats);

	summary.loaded = true;
	summary.gameId = game->id;
	summary.title = QString::fromUtf8(game->title);
	summary.badgeUrl = QString::fromUtf8(game->badge_url ? game->badge_url : "");
	summary.numAchievements = stats.num_core_achievements;
	summary.numUnlocked = stats.num_unlocked_achievements;
	summary.pointsTotal = stats.points_core;
	summary.pointsUnlocked = stats.points_unlocked;

	if (rc_client_has_rich_presence(client))
	{
		presence[0] = 0;
		rc_client_get_rich_presence_message(client, presence, sizeof(presence));
		summary.richPresence = QString::fromUtf8(presence);
	}
	return summary;
}
//----------------------------------------------------------------------------
QList<RaAchievementBucket> RetroAchievements::getAchievements(void)
{
	QList<RaAchievementBucket> result;

	FCEU_WRAPPER_SCOPED_LOCK();

	if ((client == nullptr) || !gameActive)
	{
		return result;
	}
	rc_client_achievement_list_t *list = rc_client_create_achievement_list(client,
		RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE, RC_CLIENT_ACHIEVEMENT_LIST_GROUPING_LOCK_STATE);

	if (list == nullptr)
	{
		return result;
	}
	for (uint32_t b = 0; b < list->num_buckets; b++)
	{
		const rc_client_achievement_bucket_t &src = list->buckets[b];
		RaAchievementBucket bucket;

		if (src.num_achievements == 0)
		{
			continue;
		}
		bucket.label = QString::fromUtf8(src.label ? src.label : "");

		for (uint32_t a = 0; a < src.num_achievements; a++)
		{
			const rc_client_achievement_t *ach = src.achievements[a];
			RaAchievementInfo info;
			const bool unlocked = (ach->state == RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED);
			const char *url = unlocked ? ach->badge_url : ach->badge_locked_url;

			info.id = ach->id;
			info.title = QString::fromUtf8(ach->title ? ach->title : "");
			info.description = QString::fromUtf8(ach->description ? ach->description : "");
			info.badgeUrl = QString::fromUtf8(url ? url : "");
			info.measuredProgress = QString::fromUtf8(ach->measured_progress);
			info.points = ach->points;
			info.unlocked = unlocked;

			bucket.achievements.append(info);
		}
		result.append(bucket);
	}
	rc_client_destroy_achievement_list(list);

	return result;
}
//----------------------------------------------------------------------------
QList<RaLeaderboardInfo> RetroAchievements::getLeaderboards(void)
{
	QList<RaLeaderboardInfo> result;

	FCEU_WRAPPER_SCOPED_LOCK();

	if ((client == nullptr) || !gameActive || !rc_client_has_leaderboards(client))
	{
		return result;
	}
	rc_client_leaderboard_list_t *list = rc_client_create_leaderboard_list(client, RC_CLIENT_LEADERBOARD_LIST_GROUPING_NONE);

	if (list == nullptr)
	{
		return result;
	}
	for (uint32_t b = 0; b < list->num_buckets; b++)
	{
		const rc_client_leaderboard_bucket_t &bucket = list->buckets[b];

		for (uint32_t i = 0; i < bucket.num_leaderboards; i++)
		{
			const rc_client_leaderboard_t *src = bucket.leaderboards[i];
			RaLeaderboardInfo info;

			if (src->state == RC_CLIENT_LEADERBOARD_STATE_DISABLED)
			{
				continue;
			}
			info.id = src->id;
			info.title = QString::fromUtf8(src->title ? src->title : "");
			info.description = QString::fromUtf8(src->description ? src->description : "");
			info.lowerIsBetter = (src->lower_is_better != 0);
			info.tracking = (src->state == RC_CLIENT_LEADERBOARD_STATE_TRACKING);

			result.append(info);
		}
	}
	rc_client_destroy_leaderboard_list(list);

	return result;
}
//----------------------------------------------------------------------------
void RC_CCONV RetroAchievements::leaderboardEntriesCallback(int result, const char *, rc_client_leaderboard_entry_list_t *list,
	rc_client_t *, void *userdata)
{
	LeaderboardFetch *fetch = static_cast<LeaderboardFetch*>(userdata);
	QList<RaLeaderboardEntry> entries;

	if ((result == RC_OK) && (list != nullptr))
	{
		for (uint32_t i = 0; i < list->num_entries; i++)
		{
			const rc_client_leaderboard_entry_t &src = list->entries[i];
			RaLeaderboardEntry entry;

			entry.rank = src.rank;
			entry.user = QString::fromUtf8(src.user ? src.user : "");
			entry.score = QString::fromUtf8(src.display);
			entry.submitted = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(src.submitted));

			entries.append(entry);
		}
	}
	if (list != nullptr)
	{
		rc_client_destroy_leaderboard_entry_list(list);
	}
	fetch->service->leaderboardFetches.erase(fetch);

	emit fetch->service->leaderboardEntriesReady(fetch->id, entries);

	delete fetch;
}
//----------------------------------------------------------------------------
void RetroAchievements::fetchLeaderboardEntries(uint32_t leaderboardId)
{
	FCEU_WRAPPER_SCOPED_LOCK();

	if ((client == nullptr) || !gameActive)
	{
		emit leaderboardEntriesReady(leaderboardId, QList<RaLeaderboardEntry>());
		return;
	}
	LeaderboardFetch *fetch = new LeaderboardFetch{ this, leaderboardId };

	// Registered first, because rc_client can call back before the function returns.
	leaderboardFetches.insert(fetch);

	rc_client_begin_fetch_leaderboard_entries(client, leaderboardId, 1, 10,
		&RetroAchievements::leaderboardEntriesCallback, fetch);
}
//----------------------------------------------------------------------------
void RC_CCONV RetroAchievements::eventHandler(const rc_client_event_t *event, rc_client_t *client)
{
	RetroAchievements *self = static_cast<RetroAchievements*>(rc_client_get_userdata(client));

	switch (event->type)
	{
		case RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED:
		{
			const rc_client_achievement_t *ach = event->achievement;

			FCEU_printf("RetroAchievements: unlocked '%s' (%u points): %s\n",
				ach->title, ach->points, ach->description);

			emit self->achievementUnlocked(QString::fromUtf8(ach->title), QString::fromUtf8(ach->description),
				ach->points, QString::fromUtf8(ach->badge_url ? ach->badge_url : ""));
		}
		break;
		case RC_CLIENT_EVENT_GAME_COMPLETED:
		{
			const rc_client_game_t *game = rc_client_get_game_info(client);

			FCEU_printf("RetroAchievements: all achievements for '%s' have been earned.\n", game->title);

			emit self->gameCompleted(QString::fromUtf8(game->title));
		}
		break;
		case RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_SHOW:
			self->indicators->showChallenge(event->achievement->id, shortText(event->achievement->title));
		break;
		case RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_HIDE:
			self->indicators->hideChallenge(event->achievement->id);
		break;
		case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_SHOW:
		case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_UPDATE:
			self->indicators->showProgress(QString("%1: %2")
				.arg(shortText(event->achievement->title), QString::fromUtf8(event->achievement->measured_progress)));
		break;
		case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_HIDE:
			self->indicators->hideProgress();
		break;
		case RC_CLIENT_EVENT_LEADERBOARD_TRACKER_SHOW:
		case RC_CLIENT_EVENT_LEADERBOARD_TRACKER_UPDATE:
			self->indicators->showTracker(event->leaderboard_tracker->id,
				QString::fromUtf8(event->leaderboard_tracker->display));
		break;
		case RC_CLIENT_EVENT_LEADERBOARD_TRACKER_HIDE:
			self->indicators->hideTracker(event->leaderboard_tracker->id);
		break;
		case RC_CLIENT_EVENT_LEADERBOARD_STARTED:
			FCEU_printf("RetroAchievements: leaderboard attempt started: %s\n", event->leaderboard->title);

			emit self->notification(tr("Leaderboard Attempt Started"), QString::fromUtf8(event->leaderboard->title), QString());
		break;
		case RC_CLIENT_EVENT_LEADERBOARD_FAILED:
			FCEU_printf("RetroAchievements: leaderboard attempt failed: %s\n", event->leaderboard->title);

			emit self->notification(tr("Leaderboard Attempt Failed"), QString::fromUtf8(event->leaderboard->title), QString());
		break;
		case RC_CLIENT_EVENT_LEADERBOARD_SUBMITTED:
			FCEU_printf("RetroAchievements: leaderboard score submitted: %s\n", event->leaderboard->title);
		break;
		case RC_CLIENT_EVENT_LEADERBOARD_SCOREBOARD:
		{
			const rc_client_leaderboard_scoreboard_t *board = event->leaderboard_scoreboard;
			const rc_client_leaderboard_t *info = rc_client_get_leaderboard_info(client, board->leaderboard_id);

			emit self->notification(tr("Leaderboard Score Submitted"),
				tr("%1\nScore: %2, rank %3 of %4")
					.arg(info ? QString::fromUtf8(info->title) : QString())
					.arg(QString::fromUtf8(board->submitted_score)).arg(board->new_rank).arg(board->num_entries),
				QString());
		}
		break;
		case RC_CLIENT_EVENT_SERVER_ERROR:
			FCEU_printf("RetroAchievements: server error in %s: %s\n",
				event->server_error->api, event->server_error->error_message);

			emit self->notification(tr("RetroAchievements Server Error"),
				QString::fromUtf8(event->server_error->error_message), QString());
		break;
		case RC_CLIENT_EVENT_DISCONNECTED:
			FCEU_printf("RetroAchievements: unable to reach the server, unlocks are pending.\n");

			emit self->notification(tr("RetroAchievements Disconnected"),
				tr("Unable to reach the server. Unlocks are kept and will be sent when it is back."), QString());
		break;
		case RC_CLIENT_EVENT_RECONNECTED:
			FCEU_printf("RetroAchievements: reconnected, pending unlocks were submitted.\n");

			emit self->notification(tr("RetroAchievements Reconnected"),
				tr("Pending unlocks were submitted."), QString());
		break;
		default:
		break;
	}
}
//----------------------------------------------------------------------------
// Entry points used by the emulator core. See driver.h.
//----------------------------------------------------------------------------
void FCEUD_RetroAchievementsGameLoaded(void)
{
	RetroAchievements *ra = RetroAchievements::instance();

	if (ra != nullptr)
	{
		ra->gameLoaded();
	}
}
//----------------------------------------------------------------------------
void FCEUD_RetroAchievementsGameClosed(void)
{
	RetroAchievements *ra = RetroAchievements::instance();

	if (ra != nullptr)
	{
		ra->gameClosed();
	}
}
//----------------------------------------------------------------------------
void FCEUD_RetroAchievementsFrame(void)
{
	RetroAchievements *ra = RetroAchievements::instance();

	if (ra != nullptr)
	{
		ra->frame();
	}
}
//----------------------------------------------------------------------------
void FCEUD_RetroAchievementsReset(void)
{
	RetroAchievements *ra = RetroAchievements::instance();

	if (ra != nullptr)
	{
		ra->reset();
	}
}
//----------------------------------------------------------------------------
bool FCEUD_RetroAchievementsSerialize(std::vector<uint8> &data)
{
	RetroAchievements *ra = RetroAchievements::instance();

	return (ra != nullptr) && ra->serializeProgress(data);
}
//----------------------------------------------------------------------------
void FCEUD_RetroAchievementsDeserialize(const uint8 *data, size_t size)
{
	RetroAchievements *ra = RetroAchievements::instance();

	if (ra != nullptr)
	{
		ra->deserializeProgress(data, size);
	}
}
//----------------------------------------------------------------------------

#endif // FCEUX_HAS_RETRO_ACHIEVEMENTS
