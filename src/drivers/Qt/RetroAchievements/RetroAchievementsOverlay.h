// RetroAchievementsOverlay.h
//
// Challenge, progress and leaderboard tracker indicators, drawn on the shared video overlay.
// The overlay items can be changed from any thread, so these are called straight from the
// rc_client event handler on the emulator thread.
#pragma once

#ifdef FCEUX_HAS_RETRO_ACHIEVEMENTS

#include <cstdint>
#include <map>

#include <QString>

#include "Qt/ConsoleTextOverlay.h"

class RaOverlayIndicators
{
	public:
		RaOverlayIndicators(void);
		~RaOverlayIndicators(void);

		void showChallenge(uint32_t achievementId, const QString &title);
		void hideChallenge(uint32_t achievementId);

		void showProgress(const QString &text);
		void hideProgress(void);

		void showTracker(uint32_t leaderboardId, const QString &display);
		void hideTracker(uint32_t leaderboardId);

		// Removes everything, for when the game is closed.
		void clear(void);

	private:
		ConsoleTextOverlay::Handle createItem(ConsoleTextOverlay::Anchor anchor, const QColor &color);

		std::map<uint32_t, ConsoleTextOverlay::Handle> challenges;
		std::map<uint32_t, ConsoleTextOverlay::Handle> trackers;
		ConsoleTextOverlay::Handle progressItem;
};

#endif // FCEUX_HAS_RETRO_ACHIEVEMENTS
