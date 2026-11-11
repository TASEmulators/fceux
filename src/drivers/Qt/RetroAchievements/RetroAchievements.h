// RetroAchievements.h
//
// RetroAchievements (rcheevos rc_client) service for the Qt GUI.
//
// Threading: every call into rc_client is made with the emulator lock held
// (see fceuWrapperLock). The emulator thread calls the frame/reset hooks from inside
// the core, which already holds the lock. Everything else (HTTP completions, the idle
// timer, login requests) runs on the GUI thread and takes the lock itself, so the
// emulator lock is the only mutex that serialises rc_client and no extra lock
// ordering exists.
#pragma once

#ifdef FCEUX_HAS_RETRO_ACHIEVEMENTS

#include <cstdint>
#include <string>
#include <vector>

#include <atomic>
#include <memory>

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QTimer>

#include <rc_client.h>

class QNetworkAccessManager;
class RaImageCache;
class RaOverlayIndicators;

// Plain copies of rc_client data for the GUI, which outlive the rc_client lists they come from.
struct RaGameSummary
{
	bool loaded = false;
	uint32_t gameId = 0;
	QString title;
	QString badgeUrl;
	QString richPresence;
	unsigned int numAchievements = 0;
	unsigned int numUnlocked = 0;
	unsigned int pointsTotal = 0;
	unsigned int pointsUnlocked = 0;
};

struct RaAchievementInfo
{
	uint32_t id = 0;
	QString title;
	QString description;
	QString badgeUrl;        // image to show for the current lock state
	QString measuredProgress;
	unsigned int points = 0;
	bool unlocked = false;
};

struct RaAchievementBucket
{
	QString label;
	QList<RaAchievementInfo> achievements;
};

struct RaLeaderboardInfo
{
	uint32_t id = 0;
	QString title;
	QString description;
	bool lowerIsBetter = false;
	bool tracking = false;
};

struct RaLeaderboardEntry
{
	uint32_t rank = 0;
	QString user;
	QString score;
	QDateTime submitted;
};

Q_DECLARE_METATYPE(RaLeaderboardEntry)

class RetroAchievements : public QObject
{
	Q_OBJECT

	public:
		enum LoginState
		{
			LOGGED_OUT = 0,
			LOGIN_IN_PROGRESS,
			LOGGED_IN
		};

		// Create/destroy the service on the GUI thread, after the config system is up.
		static void create(void);
		static void destroy(void);
		static RetroAchievements *instance(void);

		bool isEnabled(void) const { return client != nullptr; }
		LoginState getLoginState(void) const { return loginState; }
		QString getUserName(void) const { return loggedInUser; }

		// GUI thread only. Enabling creates the rc_client and logs in with the stored token.
		void setEnabled(bool enable);

		// GUI thread only. On success the user name and token (never the password) are stored in the config.
		void loginWithPassword(const QString &user, const QString &password);
		void logout(void);

		// Emulator hooks. Caller must hold the emulator lock.
		void gameLoaded(void);
		void gameClosed(void);
		void frame(void);
		void reset(void);

		// Progress stored in savestates. Caller must hold the emulator lock.
		bool serializeProgress(std::vector<uint8_t> &data);
		void deserializeProgress(const uint8_t *data, size_t size);

		// GUI thread only. These take the emulator lock.
		RaGameSummary getGameSummary(void);
		QList<RaAchievementBucket> getAchievements(void);
		QList<RaLeaderboardInfo> getLeaderboards(void);
		// Result arrives through leaderboardEntriesReady.
		void fetchLeaderboardEntries(uint32_t leaderboardId);

		RaImageCache *images(void) { return imageCache; }

	signals:
		void loginStateChanged(int state, const QString &userName);
		void loginFailed(const QString &message);
		void gameIdentified(unsigned int gameId, const QString &title);
		void achievementUnlocked(const QString &title, const QString &description,
			unsigned int points, const QString &badgeUrl);
		void gameCompleted(const QString &title);
		void gameUnloaded(void);
		void leaderboardEntriesReady(unsigned int leaderboardId, const QList<RaLeaderboardEntry> &entries);
		// Short messages for the user, such as leaderboard and connection events.
		void notification(const QString &title, const QString &text, const QString &imageUrl);

	private:
		RetroAchievements(void);
		~RetroAchievements(void);

		void initClient(void);
		void shutdownClient(void);
		void loginWithToken(const std::string &user, const std::string &token);
		// Reads the loaded game again from disk and starts identifying it. Needs a login.
		void identifyGame(void);
		void releaseRomImage(void);
		void setLoginState(LoginState state);
		void idle(void);
		// Writes the config file once the emulator lock has been released, because the disk write would
		// otherwise stall emulation.
		void saveConfigLater(void);

		// HTTP
		void startRequest(const QByteArray &url, const QByteArray &postData, const QByteArray &contentType,
			rc_client_server_callback_t callback, void *callbackData);

		static void RC_CCONV serverCall(const rc_api_request_t *request,
			rc_client_server_callback_t callback, void *callbackData, rc_client_t *client);
		static uint32_t RC_CCONV readMemory(uint32_t address, uint8_t *buffer, uint32_t numBytes, rc_client_t *client);
		static void RC_CCONV eventHandler(const rc_client_event_t *event, rc_client_t *client);
		static void RC_CCONV logMessage(const char *message, const rc_client_t *client);
		static void RC_CCONV loginCallback(int result, const char *errorMessage, rc_client_t *client, void *userdata);
		static void RC_CCONV loadGameCallback(int result, const char *errorMessage, rc_client_t *client, void *userdata);
		static void RC_CCONV leaderboardEntriesCallback(int result, const char *errorMessage,
			rc_client_leaderboard_entry_list_t *list, rc_client_t *client, void *userdata);

		rc_client_t *client;
		QNetworkAccessManager *netMgr;
		RaImageCache *imageCache;
		std::unique_ptr<RaOverlayIndicators> indicators;
		QTimer idleTimer;
		QByteArray userAgent;
		LoginState loginState;
		QString loggedInUser;
		bool gameActive;
		bool debugLog;
		std::atomic<bool> configSavePending;

		// Bumped when the client is destroyed, so queued requests of the old client are dropped.
		unsigned int generation;

		// ROM image of the game being identified. rc_client keeps using it until the load callback, and it is
		// released right after, so nothing is held while a game is just being played.
		std::string romName;
		std::vector<uint8_t> romImage;
};

#endif // FCEUX_HAS_RETRO_ACHIEVEMENTS
