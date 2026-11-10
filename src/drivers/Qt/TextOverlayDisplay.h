// TextOverlayDisplay.h
//
// Emulator status values (frame counter, movie state, lag counter, rerecord count and FPS) shown
// through the shared ConsoleTextOverlay, which every Qt video driver draws. This is
// separate from the video overlay that the core draws into the emulator image.

#pragma once

#include <QObject>
#include <QMenu>
#include <QTimer>
#include <QWidget>

#include "Qt/ConsoleTextOverlay.h"

class TextOverlayDisplay_t : public QObject
{
	Q_OBJECT

public:
	explicit TextOverlayDisplay_t(QObject *parent = nullptr);
	~TextOverlayDisplay_t(void) override;

	enum Item
	{
		ITEM_FRAME = 0,
		ITEM_MOVIE,
		ITEM_LAG,
		ITEM_RERECORD,
		ITEM_FPS,
		ITEM_COUNT
	};

	struct ItemInfo
	{
		const char *text;     // label, translatable
		const char *tip;      // description, translatable
	};
	static const ItemInfo &itemInfo(Item item);

	// Whether an item is turned on. Each item is independent of the others and of the
	// video overlay. Changes are saved to the config and reported through changed().
	bool isEnabled(Item item);
	void setEnabled(Item item, bool enabled);

	static constexpr int UPDATE_RATES_HZ[] = { 5, 10, 20, 30 };
	int  getUpdateRate(void) const { return updateRate; }
	void setUpdateRate(int hz);

	// Options submenu with a checkable entry per display and the update rate choice
	QMenu *createMenu(QWidget *parent);

	static constexpr int DEFAULT_UPDATE_RATE_HZ = 10;

signals:
	// Any item or the update rate changed, so controls showing them can be refreshed
	void changed(void);

private:
	int *flagFor(Item item);
	const char *optionFor(Item item);
	void loadOptions(void);
	void applyOptions(void);
	void saveOption(const char *name, int value);

	ConsoleTextOverlay::Handle frameItem;
	ConsoleTextOverlay::Handle movieItem;
	ConsoleTextOverlay::Handle lagItem;
	ConsoleTextOverlay::Handle rerecordItem;
	ConsoleTextOverlay::Handle fpsItem;

	int showFrame = 0;
	int showMovie = 1;
	int showLag = 0;
	int showRerecord = 0;
	int showFps = 0;
	int updateRate = DEFAULT_UPDATE_RATE_HZ;

	QTimer *timer;

private slots:
	void refresh(void);
};
