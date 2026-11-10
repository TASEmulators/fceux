// TextOverlayDisplay.cpp
//
#include <QActionGroup>
#include <QSignalBlocker>
#include <vector>

#include "../../fceu.h"
#include "../../profiler.h"
#include "../../input.h"
#include "../../movie.h"

#include "Qt/main.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"
#include "Qt/TextOverlayDisplay.h"

//----------------------------------------------------------------------------
TextOverlayDisplay_t::TextOverlayDisplay_t(QObject *parent)
	: QObject(parent)
{
	typedef ConsoleTextOverlay T;
	T *overlay = T::instance();

	// The status lines go in the side bars first, then top and bottom, and finally the
	// image. They are anchored to the bottom left and stack upwards in creation order, so
	// from the bottom up the lines are frame, movie, lag and rerecords.
	const std::vector<T::Region> regions =
	{
		T::REGION_LEFT_BAR, T::REGION_RIGHT_BAR, T::REGION_TOP_BAR, T::REGION_BOTTOM_BAR
	};

	for (T::Handle *h : { &frameItem, &movieItem, &lagItem, &rerecordItem, &fpsItem })
	{
		*h = overlay->createItem();
		overlay->setPlacement(*h, regions, T::ANCHOR_BOTTOM_LEFT);
		overlay->setVisible(*h, false);
	}

	// FPS sits in the upper right corner instead: the right bar, then the top bar, and
	// finally the top right of the image.
	overlay->setPlacement(fpsItem, { T::REGION_RIGHT_BAR, T::REGION_TOP_BAR }, T::ANCHOR_TOP_RIGHT);
	loadOptions();

	timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &TextOverlayDisplay_t::refresh);

	applyOptions();
}
//----------------------------------------------------------------------------
TextOverlayDisplay_t::~TextOverlayDisplay_t(void)
{
	ConsoleTextOverlay *overlay = ConsoleTextOverlay::instance();

	overlay->destroyItem(frameItem);
	overlay->destroyItem(movieItem);
	overlay->destroyItem(lagItem);
	overlay->destroyItem(rerecordItem);
	overlay->destroyItem(fpsItem);
}
//----------------------------------------------------------------------------
void TextOverlayDisplay_t::loadOptions(void)
{
	g_config->getOption("SDL.TextOverlay.ShowFrame", &showFrame);
	g_config->getOption("SDL.TextOverlay.ShowMovie", &showMovie);
	g_config->getOption("SDL.TextOverlay.ShowLag", &showLag);
	g_config->getOption("SDL.TextOverlay.ShowRerecord", &showRerecord);
	g_config->getOption("SDL.TextOverlay.ShowFPS", &showFps);
	g_config->getOption("SDL.TextOverlay.UpdateRate", &updateRate);

	if ((updateRate != 5) && (updateRate != 10) && (updateRate != 20) && (updateRate != 30))
	{
		updateRate = DEFAULT_UPDATE_RATE_HZ;
	}
}
//----------------------------------------------------------------------------
void TextOverlayDisplay_t::saveOption(const char *name, int value)
{
	g_config->setOption(name, value);
}
//----------------------------------------------------------------------------
void TextOverlayDisplay_t::applyOptions(void)
{
	const bool any = showFrame || showMovie || showLag || showRerecord || showFps;

	timer->setInterval(1000 / updateRate);

	if (any)
	{
		if (!timer->isActive())
		{
			timer->start();
		}
		refresh();
	}
	else
	{
		timer->stop();

		ConsoleTextOverlay *overlay = ConsoleTextOverlay::instance();

		overlay->setVisible(frameItem, false);
		overlay->setVisible(movieItem, false);
		overlay->setVisible(lagItem, false);
		overlay->setVisible(rerecordItem, false);
		overlay->setVisible(fpsItem, false);
	}
}
//----------------------------------------------------------------------------
const TextOverlayDisplay_t::ItemInfo &TextOverlayDisplay_t::itemInfo(Item item)
{
	static const ItemInfo info[ITEM_COUNT] =
	{
		{ QT_TR_NOOP("Frame Counter"), QT_TR_NOOP("Show the frame counter, with the movie length while a movie is loaded.") },
		{ QT_TR_NOOP("Movie State"), QT_TR_NOOP("Show the movie mode (play, record or finished), record mode and read only state while a movie is loaded.") },
		{ QT_TR_NOOP("Lag Counter"), QT_TR_NOOP("Show the lag frame count, in red while the game is lagging.") },
		{ QT_TR_NOOP("Rerecord Count"), QT_TR_NOOP("Show the movie rerecord count while a movie is loaded.") },
		{ QT_TR_NOOP("FPS"), QT_TR_NOOP("Show the emulation speed in frames per second.") },
	};
	return info[item];
}
//----------------------------------------------------------------------------
int *TextOverlayDisplay_t::flagFor(Item item)
{
	switch (item)
	{
		case ITEM_FRAME:    return &showFrame;
		case ITEM_MOVIE:    return &showMovie;
		case ITEM_FPS:      return &showFps;
		case ITEM_LAG:      return &showLag;
		default:            return &showRerecord;
	}
}
//----------------------------------------------------------------------------
const char *TextOverlayDisplay_t::optionFor(Item item)
{
	switch (item)
	{
		case ITEM_FRAME:    return "SDL.TextOverlay.ShowFrame";
		case ITEM_MOVIE:    return "SDL.TextOverlay.ShowMovie";
		case ITEM_FPS:      return "SDL.TextOverlay.ShowFPS";
		case ITEM_LAG:      return "SDL.TextOverlay.ShowLag";
		default:            return "SDL.TextOverlay.ShowRerecord";
	}
}
//----------------------------------------------------------------------------
bool TextOverlayDisplay_t::isEnabled(Item item)
{
	return *flagFor(item) != 0;
}
//----------------------------------------------------------------------------
void TextOverlayDisplay_t::setEnabled(Item item, bool enabled)
{
	int *flag = flagFor(item);

	if ((*flag != 0) == enabled)
	{
		return;
	}
	*flag = enabled ? 1 : 0;
	saveOption(optionFor(item), *flag);
	applyOptions();
	emit changed();
}
//----------------------------------------------------------------------------
void TextOverlayDisplay_t::setUpdateRate(int hz)
{
	if ((hz == updateRate) || ((hz != 5) && (hz != 10) && (hz != 20) && (hz != 30)))
	{
		return;
	}
	updateRate = hz;
	saveOption("SDL.TextOverlay.UpdateRate", updateRate);
	applyOptions();
	emit changed();
}
//----------------------------------------------------------------------------
QMenu *TextOverlayDisplay_t::createMenu(QWidget *parent)
{
	QMenu *menu = new QMenu(tr("Window Text Over&lay"), parent);
	menu->menuAction()->setStatusTip(tr("Show emulator status text over the video, drawn by the video driver at the window resolution, including outside of the game image."));
	menu->menuAction()->setToolTip(menu->menuAction()->statusTip());

	std::vector<QAction *> itemActs;

	for (int i = 0; i < ITEM_COUNT; i++)
	{
		const Item item = static_cast<Item>(i);
		const ItemInfo &info = itemInfo(item);

		QAction *act = new QAction(tr(info.text), menu);
		act->setCheckable(true);
		act->setChecked(isEnabled(item));
		act->setStatusTip(tr(info.tip));
		act->setToolTip(act->statusTip());

		connect(act, &QAction::toggled, this, [this, item](bool checked)
		{
			setEnabled(item, checked);
		});
		menu->addAction(act);
		itemActs.push_back(act);
	}
	menu->addSeparator();

	QMenu *rateMenu = menu->addMenu(tr("&Update Rate"));
	rateMenu->menuAction()->setStatusTip(tr("How often the displayed values are refreshed."));
	rateMenu->menuAction()->setToolTip(rateMenu->menuAction()->statusTip());

	QActionGroup *group = new QActionGroup(rateMenu);
	group->setExclusive(true);

	std::vector<QAction *> rateActs;

	for (int hz : UPDATE_RATES_HZ)
	{
		QAction *act = new QAction(tr("%1 Hz").arg(hz), rateMenu);
		act->setCheckable(true);
		act->setChecked(updateRate == hz);
		group->addAction(act);
		rateMenu->addAction(act);
		rateActs.push_back(act);

		connect(act, &QAction::triggered, this, [this, hz]()
		{
			setUpdateRate(hz);
		});
	}

	// Keep the menu in step with changes made from the Video Config window
	connect(this, &TextOverlayDisplay_t::changed, menu, [this, itemActs, rateActs]()
	{
		for (int i = 0; i < ITEM_COUNT; i++)
		{
			QSignalBlocker blocker(itemActs[i]);
			itemActs[i]->setChecked(isEnabled(static_cast<Item>(i)));
		}
		for (size_t i = 0; i < rateActs.size(); i++)
		{
			QSignalBlocker blocker(rateActs[i]);
			rateActs[i]->setChecked(UPDATE_RATES_HZ[i] == updateRate);
		}
	});
	return menu;
}
//----------------------------------------------------------------------------
void TextOverlayDisplay_t::refresh(void)
{
	FCEU_PROFILE_FUNC("Text overlay display refresh");

	ConsoleTextOverlay *overlay = ConsoleTextOverlay::instance();

	// The emulator lock is deliberately not taken. These are plain values that are only
	// displayed, so a briefly stale value is harmless, and taking the lock this often would
	// make the emulation thread spin while it waits.
	const bool loaded = (GameInfo != nullptr);
	const int  movieState = movieMode;

	overlay->setVisible(frameItem, loaded && showFrame);
	overlay->setVisible(movieItem, loaded && showMovie && (movieState != MOVIEMODE_INACTIVE));
	overlay->setVisible(lagItem, loaded && showLag);
	overlay->setVisible(fpsItem, loaded && showFps);
	overlay->setVisible(rerecordItem, loaded && showRerecord && (movieState != MOVIEMODE_INACTIVE));

	if (!loaded)
	{
		return;
	}

	if (showFrame)
	{
		const int frame = currFrameCounter;
		const int movieFrames = static_cast<int>(currMovieData.records.size());
		const int recordMode = movieRecordMode;

		// The length is shown while a movie has one. A truncating recording grows as it goes.
		const bool hasLength = (movieState == MOVIEMODE_PLAY) || (movieState == MOVIEMODE_FINISHED) ||
			((movieState == MOVIEMODE_RECORD) && (recordMode != MOVIE_RECORD_MODE_TRUNCATE));

		if (hasLength)
		{
			overlay->setText(frameItem, QString("Frame: %1/%2").arg(frame).arg(movieFrames));
		}
		else
		{
			overlay->setText(frameItem, QString("Frame: %1").arg(frame));
		}
		overlay->setColor(frameItem, QColor(255, 255, 255));
	}

	if (showMovie && (movieState != MOVIEMODE_INACTIVE))
	{
		const int  recordMode = movieRecordMode;
		const bool readOnly = movie_readonly;
		const char *stateStr;
		QColor color(120, 200, 255);

		switch (movieState)
		{
			case MOVIEMODE_PLAY:
				stateStr = "Play";
				break;
			case MOVIEMODE_RECORD:
				stateStr = "Record";
				color = QColor(255, 120, 120);
				break;
			case MOVIEMODE_FINISHED:
				stateStr = "Finished";
				color = QColor(255, 200, 60);
				break;
			default:
				stateStr = "TAS Editor";
				break;
		}

		// Record mode and read only state use the same markers as the video overlay
		const char *modeStr = "";
		if (recordMode == MOVIE_RECORD_MODE_OVERWRITE)
		{
			modeStr = " [W]";
		}
		else if (recordMode == MOVIE_RECORD_MODE_INSERT)
		{
			modeStr = " [I]";
		}
		const char *roStr = (movieState == MOVIEMODE_RECORD) ? (readOnly ? " R-O" : "") : (readOnly ? "" : " R+W");

		overlay->setText(movieItem, QString("%1%2%3").arg(stateStr, modeStr, roStr));
		overlay->setColor(movieItem, color);
	}

	if (showLag)
	{
		const bool lagging = (lagFlag != 0);

		overlay->setText(lagItem, QString("Lag: %1").arg(lagCounter));
		overlay->setColor(lagItem, lagging ? QColor(255, 80, 80) : QColor(80, 230, 100));
	}

	if (showRerecord && (movieState != MOVIEMODE_INACTIVE))
	{
		overlay->setText(rerecordItem, QString("Rerecords: %1").arg(currMovieData.rerecordCount));
		overlay->setColor(rerecordItem, QColor(255, 200, 60));
	}

	if (showFps)
	{
		overlay->setText(fpsItem, QString("FPS: %1").arg(emulatorMeasuredFps, 0, 'f', 1));
		overlay->setColor(fpsItem, QColor(255, 255, 255));
	}
}
//----------------------------------------------------------------------------
