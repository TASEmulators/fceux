// RetroAchievementsOverlay.cpp
//
#ifdef FCEUX_HAS_RETRO_ACHIEVEMENTS

#include <vector>

#include "Qt/RetroAchievements/RetroAchievementsOverlay.h"

typedef ConsoleTextOverlay T;

//----------------------------------------------------------------------------
RaOverlayIndicators::RaOverlayIndicators(void)
	: progressItem(T::INVALID_HANDLE)
{
}
//----------------------------------------------------------------------------
RaOverlayIndicators::~RaOverlayIndicators(void)
{
	clear();
}
//----------------------------------------------------------------------------
T::Handle RaOverlayIndicators::createItem(T::Anchor anchor, const QColor &color)
{
	ConsoleTextOverlay *overlay = ConsoleTextOverlay::instance();
	const std::vector<T::Region> regions = { T::REGION_BOTTOM_BAR, T::REGION_TOP_BAR, T::REGION_IMAGE };
	const T::Handle h = overlay->createItem();

	overlay->setPlacement(h, regions, anchor);
	overlay->setColor(h, color);

	return h;
}
//----------------------------------------------------------------------------
void RaOverlayIndicators::showChallenge(uint32_t achievementId, const QString &title)
{
	ConsoleTextOverlay *overlay = ConsoleTextOverlay::instance();
	auto it = challenges.find(achievementId);

	if (it == challenges.end())
	{
		it = challenges.emplace(achievementId, createItem(T::ANCHOR_BOTTOM_RIGHT, QColor(255, 200, 0))).first;
	}
	overlay->setText(it->second, QString("Challenge: %1").arg(title));
	overlay->setVisible(it->second, true);
}
//----------------------------------------------------------------------------
void RaOverlayIndicators::hideChallenge(uint32_t achievementId)
{
	auto it = challenges.find(achievementId);

	if (it != challenges.end())
	{
		ConsoleTextOverlay::instance()->destroyItem(it->second);
		challenges.erase(it);
	}
}
//----------------------------------------------------------------------------
void RaOverlayIndicators::showProgress(const QString &text)
{
	ConsoleTextOverlay *overlay = ConsoleTextOverlay::instance();

	if (progressItem == T::INVALID_HANDLE)
	{
		progressItem = createItem(T::ANCHOR_BOTTOM_CENTER, QColor(120, 220, 255));
	}
	overlay->setText(progressItem, text);
	overlay->setVisible(progressItem, true);
}
//----------------------------------------------------------------------------
void RaOverlayIndicators::hideProgress(void)
{
	if (progressItem != T::INVALID_HANDLE)
	{
		ConsoleTextOverlay::instance()->destroyItem(progressItem);
		progressItem = T::INVALID_HANDLE;
	}
}
//----------------------------------------------------------------------------
void RaOverlayIndicators::showTracker(uint32_t leaderboardId, const QString &display)
{
	ConsoleTextOverlay *overlay = ConsoleTextOverlay::instance();
	auto it = trackers.find(leaderboardId);

	if (it == trackers.end())
	{
		it = trackers.emplace(leaderboardId, createItem(T::ANCHOR_TOP_CENTER, QColor(255, 255, 255))).first;
	}
	overlay->setText(it->second, display);
	overlay->setVisible(it->second, true);
}
//----------------------------------------------------------------------------
void RaOverlayIndicators::hideTracker(uint32_t leaderboardId)
{
	auto it = trackers.find(leaderboardId);

	if (it != trackers.end())
	{
		ConsoleTextOverlay::instance()->destroyItem(it->second);
		trackers.erase(it);
	}
}
//----------------------------------------------------------------------------
void RaOverlayIndicators::clear(void)
{
	ConsoleTextOverlay *overlay = ConsoleTextOverlay::instance();

	for (auto &kv : challenges)
	{
		overlay->destroyItem(kv.second);
	}
	challenges.clear();

	for (auto &kv : trackers)
	{
		overlay->destroyItem(kv.second);
	}
	trackers.clear();

	hideProgress();
}
//----------------------------------------------------------------------------

#endif // FCEUX_HAS_RETRO_ACHIEVEMENTS
