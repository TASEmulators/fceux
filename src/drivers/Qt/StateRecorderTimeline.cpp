// StateRecorderTimeline.cpp
//
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <stdint.h>

#include <QPainter>
#include <QPalette>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QHelpEvent>
#include <QToolTip>
#include <QStyle>
#include <QHBoxLayout>
#include <QFontMetrics>
#include <QPixmap>
#include <QPolygonF>

#include "../../fceu.h"
#include "../../state.h"
#include "../../driver.h"

#include "../../profiler.h"

#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/StateRecorderTimeline.h"

static constexpr int BAR_MARGIN = 10;
static constexpr int NOTICE_DURATION_MS = 4000;
static const QColor futureColor(224, 128, 40);

static std::atomic<StateRecorderTimelineDock_t*> activeDock{nullptr};

enum TransportGlyph { GLYPH_REWIND, GLYPH_FORWARD, GLYPH_PLAY, GLYPH_PAUSE };

// The standard style icons are dark and unreadable on a dark background, so the
// transport glyphs are drawn in the color that contrasts with the background.
static QIcon makeGlyphIcon(TransportGlyph glyph, const QColor &fg)
{
	constexpr int S = 64;
	QIcon icon;

	for (QIcon::Mode mode : { QIcon::Normal, QIcon::Disabled })
	{
		QPixmap pix(S, S);
		pix.fill(Qt::transparent);

		QColor color = fg;
		color.setAlphaF((mode == QIcon::Normal) ? 1.0 : 0.3);

		QPainter p(&pix);
		p.setRenderHint(QPainter::Antialiasing, true);
		p.setPen(Qt::NoPen);
		p.setBrush(color);

		const double m = 12.0, mid = S / 2.0, top = 12.0, bot = S - 12.0;

		switch (glyph)
		{
			case GLYPH_REWIND:
				p.drawPolygon(QPolygonF() << QPointF(mid, top) << QPointF(mid, bot) << QPointF(m, S / 2.0));
				p.drawPolygon(QPolygonF() << QPointF(S - m, top) << QPointF(S - m, bot) << QPointF(mid, S / 2.0));
			break;
			case GLYPH_FORWARD:
				p.drawPolygon(QPolygonF() << QPointF(m, top) << QPointF(m, bot) << QPointF(mid, S / 2.0));
				p.drawPolygon(QPolygonF() << QPointF(mid, top) << QPointF(mid, bot) << QPointF(S - m, S / 2.0));
			break;
			case GLYPH_PLAY:
				p.drawPolygon(QPolygonF() << QPointF(m + 4, top - 2) << QPointF(m + 4, bot + 2) << QPointF(S - m, S / 2.0));
			break;
			case GLYPH_PAUSE:
				p.drawRect(QRectF(m + 4, top - 2, 13, bot - top + 4));
				p.drawRect(QRectF(S - m - 17, top - 2, 13, bot - top + 4));
			break;
		}
		p.end();

		icon.addPixmap(pix, mode);
	}
	return icon;
}

static void recorderChanged(void)
{
	StateRecorderTimelineDock_t *dock = activeDock.load();

	if (dock != nullptr)
	{
		dock->requestRefresh();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
StateTimelineBar_t::StateTimelineBar_t(QWidget *parent)
	: QWidget(parent)
{
	setMouseTracking(true);
	setAutoFillBackground(true);
}
//----------------------------------------------------------------------------
QSize StateTimelineBar_t::sizeHint(void) const
{
	return QSize(400, 56);
}
//----------------------------------------------------------------------------
QSize StateTimelineBar_t::minimumSizeHint(void) const
{
	return QSize(120, 46);
}
//----------------------------------------------------------------------------
void StateTimelineBar_t::setData(const StateRecorderTimelineData &newData, double framesPerSecond)
{
	FCEU_PROFILE_FUNC("Timeline bar setData");
	data = newData;
	hz = (framesPerSecond > 1.0) ? framesPerSecond : 60.0;

	firstFrame = data.currentFrame;
	lastFrame  = data.currentFrame;

	for (int f : data.snapFrames)
	{
		firstFrame = std::min(firstFrame, f);
		lastFrame  = std::max(lastFrame , f);
	}
	if (lastFrame <= firstFrame)
	{
		lastFrame = firstFrame + 1;
	}
	const int numSnaps = static_cast<int>(data.snapFrames.size());

	if (hoverSnap >= numSnaps)
	{
		hoverSnap = -1;
	}
	const int firstSnapFrame = numSnaps ? data.snapFrames.front() : 0;
	const int lastSnapFrame  = numSnaps ? data.snapFrames.back()  : 0;

	// A frame delta of at least this much moves something by a pixel or more
	const double framesPerPixel = double(lastFrame - firstFrame) / trackRect().width();

	const bool changed =
		(numSnaps != paintedNumSnaps) ||
		(firstSnapFrame != paintedFirstSnapFrame) ||
		(lastSnapFrame != paintedLastSnapFrame) ||
		(data.currentSnap != paintedCurrentSnap) ||
		(std::abs(data.currentFrame - paintedCurrentFrame) >= framesPerPixel) ||
		(std::abs(lastFrame - paintedLastFrame) >= framesPerPixel);

	if (changed)
	{
		paintedNumSnaps = numSnaps;
		paintedFirstSnapFrame = firstSnapFrame;
		paintedLastSnapFrame = lastSnapFrame;
		paintedCurrentSnap = data.currentSnap;
		paintedCurrentFrame = data.currentFrame;
		paintedLastFrame = lastFrame;
		update();
	}
}
//----------------------------------------------------------------------------
int StateTimelineBar_t::numFutureSnaps(void) const
{
	const int n = static_cast<int>(data.snapFrames.size());

	return (data.currentSnap >= 0) ? (n - data.currentSnap - 1) : 0;
}
//----------------------------------------------------------------------------
void StateTimelineBar_t::setBgColor(const QColor &c)
{
	bgColor = c;
	update();
}
//----------------------------------------------------------------------------
QColor StateTimelineBar_t::fgColor(void) const
{
	return (bgColor.lightness() < 128) ? QColor(Qt::white) : QColor(Qt::black);
}
//----------------------------------------------------------------------------
QColor StateTimelineBar_t::blendedFg(double t) const
{
	const QColor fg = fgColor();

	return QColor::fromRgbF(bgColor.redF()   + (fg.redF()   - bgColor.redF())   * t,
	                        bgColor.greenF() + (fg.greenF() - bgColor.greenF()) * t,
	                        bgColor.blueF()  + (fg.blueF()  - bgColor.blueF())  * t);
}
//----------------------------------------------------------------------------
QRectF StateTimelineBar_t::trackRect(void) const
{
	const int textH = QFontMetrics(font()).height();
	const double top = 10.0;
	const double h = std::max(8.0, double(height() - textH - 22));

	return QRectF(BAR_MARGIN, top, std::max(1, width() - 2 * BAR_MARGIN), h);
}
//----------------------------------------------------------------------------
double StateTimelineBar_t::frameToX(double frame, const QRectF &track) const
{
	return track.left() + ((frame - firstFrame) / double(lastFrame - firstFrame)) * track.width();
}
//----------------------------------------------------------------------------
int StateTimelineBar_t::snapAtX(double x) const
{
	const QRectF track = trackRect();
	int best = -1;
	double bestDist = 0.0;

	for (size_t i = 0; i < data.snapFrames.size(); i++)
	{
		const double dist = std::fabs(frameToX(data.snapFrames[i], track) - x);

		if ((best < 0) || (dist < bestDist))
		{
			best = static_cast<int>(i);
			bestDist = dist;
		}
	}
	return best;
}
//----------------------------------------------------------------------------
QString StateTimelineBar_t::formatFrame(int frame) const
{
	const int totalSec = static_cast<int>(std::max(0, frame) / hz);

	return tr("%1 (%2:%3)").arg(frame).arg(totalSec / 60).arg(totalSec % 60, 2, 10, QChar('0'));
}
//----------------------------------------------------------------------------
void StateTimelineBar_t::seekToX(double x)
{
	const int snap = snapAtX(x);

	if ((snap >= 0) && (snap != lastSeekSnap))
	{
		lastSeekSnap = snap;
		emit seekRequested(snap);
	}
}
//----------------------------------------------------------------------------
void StateTimelineBar_t::paintEvent(QPaintEvent *event)
{
	FCEU_PROFILE_FUNC("Timeline bar paint");
	QPainter p(this);
	const QPalette pal = palette();
	const QRectF track = trackRect();
	const int numSnaps = static_cast<int>(data.snapFrames.size());
	const int numFuture = numFutureSnaps();

	p.setRenderHint(QPainter::Antialiasing, true);

	p.fillRect(rect(), bgColor);

	p.setPen(blendedFg(0.35));
	p.setBrush(blendedFg(0.10));
	p.drawRoundedRect(track, 3, 3);

	if (numSnaps == 0)
	{
		p.setPen(blendedFg(0.5));
		p.drawText(track, Qt::AlignCenter, tr("No history recorded"));
		return;
	}

	const double curX = std::min(std::max(frameToX(data.currentFrame, track), track.left()), track.right());

	// Elapsed history
	const QColor pastColor = pal.color(QPalette::Highlight);
	p.setPen(Qt::NoPen);
	p.setBrush(QColor(pastColor.red(), pastColor.green(), pastColor.blue(), 110));
	p.drawRoundedRect(QRectF(track.left(), track.top(), curX - track.left(), track.height()), 3, 3);

	// History that is lost if emulation resumes from the current position
	if (numFuture > 0)
	{
		const QRectF futureRect(curX, track.top(), track.right() - curX, track.height());
		QPen dashed(futureColor);

		dashed.setStyle(Qt::DashLine);
		p.setPen(dashed);
		p.setBrush(QBrush(futureColor, Qt::BDiagPattern));
		p.drawRect(futureRect);
	}

	// Snapshot ticks. There can be far more snapshots than pixels, so the ticks are
	// collapsed to one per pixel column and drawn in a single batch per color.
	{
		const int trackLeft = static_cast<int>(track.left());
		const int numCols = static_cast<int>(track.width()) + 1;
		const double scale = track.width() / double(lastFrame - firstFrame);
		const int tickTop = static_cast<int>(track.bottom() - track.height() * 0.45);
		const int tickBottom = static_cast<int>(track.bottom());

		// 0: no tick, 1: elapsed snapshot, 2: snapshot ahead of the playhead
		std::vector<uint8_t> columns(numCols, 0);

		for (int i = 0; i < numSnaps; i++)
		{
			const int col = static_cast<int>((data.snapFrames[i] - firstFrame) * scale);

			if ((col >= 0) && (col < numCols))
			{
				columns[col] |= (i > data.currentSnap) ? 2 : 1;
			}
		}
		QVector<QLine> pastLines, futureLines;

		for (int col = 0; col < numCols; col++)
		{
			const QLine line(trackLeft + col, tickTop, trackLeft + col, tickBottom);

			if (columns[col] & 1)
			{
				pastLines.push_back(line);
			}
			else if (columns[col] & 2)
			{
				futureLines.push_back(line);
			}
		}
		p.setRenderHint(QPainter::Antialiasing, false);

		QColor c = fgColor();
		c.setAlpha(150);
		p.setPen(QPen(c, 1.0));
		p.drawLines(pastLines);

		c = futureColor;
		c.setAlpha(150);
		p.setPen(QPen(c, 1.0));
		p.drawLines(futureLines);

		p.setRenderHint(QPainter::Antialiasing, true);

		// The current and hovered snapshots stand out
		for (int i : { data.currentSnap, hoverSnap })
		{
			if ((i < 0) || (i >= numSnaps))
			{
				continue;
			}
			const double x = frameToX(data.snapFrames[i], track);

			p.setPen(QPen((i > data.currentSnap) ? futureColor : fgColor(), 2.0));
			p.drawLine(QPointF(x, track.top()), QPointF(x, track.bottom()));
		}
	}

	// Playhead
	{
		const QColor c = fgColor();
		QPolygonF head;

		head << QPointF(curX - 5, track.top() - 8) << QPointF(curX + 5, track.top() - 8) << QPointF(curX, track.top() - 1);

		p.setPen(QPen(c, 2.0));
		p.drawLine(QPointF(curX, track.top() - 1), QPointF(curX, track.bottom() + 3));
		p.setPen(Qt::NoPen);
		p.setBrush(c);
		p.drawPolygon(head);
	}

	// Time labels
	{
		const double textY = track.bottom() + 6;
		const QRectF leftRect(track.left(), textY, track.width() / 2, height() - textY);
		const QRectF rightRect(track.center().x(), textY, track.width() / 2, height() - textY);

		p.setPen(fgColor());
		p.drawText(leftRect, Qt::AlignLeft | Qt::AlignTop, formatFrame(firstFrame));

		if (numFuture > 0)
		{
			p.setPen(futureColor);
		}
		p.drawText(rightRect, Qt::AlignRight | Qt::AlignTop, formatFrame(lastFrame));
	}
}
//----------------------------------------------------------------------------
void StateTimelineBar_t::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		dragging = true;
		lastSeekSnap = -1;
		seekToX(event->pos().x());
	}
}
//----------------------------------------------------------------------------
void StateTimelineBar_t::mouseMoveEvent(QMouseEvent *event)
{
	FCEU_PROFILE_FUNC("Timeline bar mouse move");
	const int snap = snapAtX(event->pos().x());

	if (dragging)
	{
		seekToX(event->pos().x());
	}
	if (snap != hoverSnap)
	{
		hoverSnap = snap;
		update();
	}
}
//----------------------------------------------------------------------------
void StateTimelineBar_t::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		dragging = false;
	}
}
//----------------------------------------------------------------------------
void StateTimelineBar_t::leaveEvent(QEvent *event)
{
	QWidget::leaveEvent(event);

	if (hoverSnap != -1)
	{
		hoverSnap = -1;
		update();
	}
}
//----------------------------------------------------------------------------
bool StateTimelineBar_t::event(QEvent *event)
{
	if (event->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent = static_cast<QHelpEvent*>(event);
		const int snap = snapAtX(helpEvent->pos().x());

		if (snap >= 0)
		{
			QString text = tr("Snapshot %1 of %2\nFrame %3")
				.arg(snap + 1).arg(data.snapFrames.size()).arg(formatFrame(data.snapFrames[snap]));

			if (snap > data.currentSnap)
			{
				text += tr("\nDiscarded if emulation resumes from the current position");
			}
			QToolTip::showText(helpEvent->globalPos(), text, this);
		}
		else
		{
			QToolTip::hideText();
		}
		return true;
	}
	return QWidget::event(event);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
StateRecorderTimelineDock_t::StateRecorderTimelineDock_t(QWidget *parent)
	: QDockWidget(tr("State Recorder Timeline"), parent)
{
	setObjectName("StateRecorderTimelineDock");
	setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);

	container = new QWidget(this);
	container->setAutoFillBackground(true);
	QHBoxLayout *layout = new QHBoxLayout(container);
	layout->setContentsMargins(4, 2, 4, 2);

	rewindButton  = new QToolButton(container);
	playButton    = new QToolButton(container);
	forwardButton = new QToolButton(container);

	rewindButton->setToolTip(tr("Rewind to the previous snapshot (hold to keep rewinding)"));
	rewindButton->setAutoRepeat(true);
	rewindButton->setAutoRepeatDelay(400);
	rewindButton->setAutoRepeatInterval(120);

	forwardButton->setToolTip(tr("Fast forward to the next snapshot (hold to keep going)"));
	forwardButton->setAutoRepeat(true);
	forwardButton->setAutoRepeatDelay(400);
	forwardButton->setAutoRepeatInterval(120);

	const int btnSize = style()->pixelMetric(QStyle::PM_ToolBarIconSize) + 8;
	for (QToolButton *btn : { rewindButton, playButton, forwardButton })
	{
		btn->setAutoRaise(true);
		btn->setMinimumSize(btnSize, btnSize);
	}

	bar = new StateTimelineBar_t(container);

	infoLabel = new QLabel(container);
	infoLabel->setMinimumWidth(QFontMetrics(font()).horizontalAdvance("Frame 000000 (00:00)") + 8);
	infoLabel->setTextFormat(Qt::PlainText);

	layout->addWidget(rewindButton);
	layout->addWidget(playButton);
	layout->addWidget(forwardButton);
	layout->addWidget(bar, 1);
	layout->addWidget(infoLabel);

	setWidget(container);
	applyBgColor(bgColor);

	connect(rewindButton , &QToolButton::clicked, this, &StateRecorderTimelineDock_t::rewindStep);
	connect(forwardButton, &QToolButton::clicked, this, &StateRecorderTimelineDock_t::forwardStep);
	connect(playButton   , &QToolButton::clicked, this, &StateRecorderTimelineDock_t::togglePlay);
	connect(bar, &StateTimelineBar_t::seekRequested, this, &StateRecorderTimelineDock_t::seekTo);

	throttleTimer = new QTimer(this);
	throttleTimer->setSingleShot(true);
	connect(throttleTimer, &QTimer::timeout, this, &StateRecorderTimelineDock_t::refresh);

	// The recorder reports every change to the history and to the pause state, so
	// there is no need to poll it.
	activeDock.store(this);
	FCEU_StateRecorderSetChangeCallback(recorderChanged);
}
//----------------------------------------------------------------------------
StateRecorderTimelineDock_t::~StateRecorderTimelineDock_t(void)
{
	// The callback runs with the wrapper lock held, so once this returns it is not in use
	FCEU_WRAPPER_SCOPED_LOCK();
	FCEU_StateRecorderSetChangeCallback(nullptr);
	activeDock.store(nullptr);
}
//----------------------------------------------------------------------------
void StateRecorderTimelineDock_t::applyBgColor(const QColor &c)
{
	bgColor = c;

	const QColor fg = (c.lightness() < 128) ? QColor(Qt::white) : QColor(Qt::black);

	QPalette pal = container->palette();
	pal.setColor(QPalette::Window, c);
	pal.setColor(QPalette::WindowText, fg);
	pal.setColor(QPalette::ButtonText, fg);
	pal.setColor(QPalette::Text, fg);
	container->setPalette(pal);

	bar->setBgColor(c);

	rewindIcon  = makeGlyphIcon(GLYPH_REWIND , fg);
	forwardIcon = makeGlyphIcon(GLYPH_FORWARD, fg);
	playIcon    = makeGlyphIcon(GLYPH_PLAY   , fg);
	pauseIcon   = makeGlyphIcon(GLYPH_PAUSE  , fg);

	rewindButton->setIcon(rewindIcon);
	forwardButton->setIcon(forwardIcon);
	playButton->setIcon(wasPaused ? playIcon : pauseIcon);

	// The native hover and pressed look uses a light button color that hides the glyph,
	// so highlight with a translucent tint of the foreground color instead.
	const QString rgb = QString("%1,%2,%3").arg(fg.red()).arg(fg.green()).arg(fg.blue());
	const QString css = QString(
		"QToolButton { background: transparent; border: 1px solid transparent; border-radius: 3px; }"
		"QToolButton:hover { background: rgba(%1,40); border-color: rgba(%1,90); }"
		"QToolButton:pressed { background: rgba(%1,80); border-color: rgba(%1,120); }"
		"QToolButton:disabled { background: transparent; border-color: transparent; }").arg(rgb);

	for (QToolButton *btn : { rewindButton, playButton, forwardButton })
	{
		btn->setStyleSheet(css);
	}
}
//----------------------------------------------------------------------------
void StateRecorderTimelineDock_t::requestRefresh(void)
{
	if (!refreshQueued.exchange(true))
	{
		QMetaObject::invokeMethod(this, [this](){ scheduleRefresh(); }, Qt::QueuedConnection);
	}
}
//----------------------------------------------------------------------------
void StateRecorderTimelineDock_t::scheduleRefresh(void)
{
	if (throttleTimer->isActive())
	{
		return;
	}
	const qint64 elapsed = refreshTimer.isValid() ? refreshTimer.elapsed() : MIN_REFRESH_INTERVAL_MS;

	if (elapsed >= MIN_REFRESH_INTERVAL_MS)
	{
		refresh();
	}
	else
	{
		throttleTimer->start(static_cast<int>(MIN_REFRESH_INTERVAL_MS - elapsed));
	}
}
//----------------------------------------------------------------------------
void StateRecorderTimelineDock_t::showEvent(QShowEvent *event)
{
	QDockWidget::showEvent(event);
	refresh();
}
//----------------------------------------------------------------------------
void StateRecorderTimelineDock_t::refresh(void)
{
	refreshQueued.store(false);

	if (!isVisible())
	{
		return;
	}
	refreshTimer.start();

	FCEU_PROFILE_FUNC("Timeline dock refresh total");

	StateRecorderTimelineData data;
	bool running, paused;
	double hz = 60.0;
	QColor videoBg = bgColor;

	{
		FCEU_PROFILE_FUNC("Timeline dock refresh lock+query");
		FCEU_WRAPPER_SCOPED_LOCK();

		running = (GameInfo != nullptr) && (FCEU_StateRecorderGetTimeline(data) == 0);
		paused  = (EmulationPaused & (EMULATIONPAUSED_PAUSED | EMULATIONPAUSED_TIMER)) != 0;
		hz      = double(FCEUI_GetDesiredFPS()) / 16777216.0;

		if (consoleWindow != nullptr)
		{
			videoBg = *consoleWindow->getVideoBgColorPtr();
		}
	}
	if (videoBg != bgColor)
	{
		applyBgColor(videoBg);
	}
	const int numSnaps = static_cast<int>(data.snapFrames.size());

	bar->setData(data, hz);

	const int numFuture = bar->numFutureSnaps();

	// Resuming from a rewound position throws away the snapshots that were ahead of it
	if ((lastNumFuture > 0) && (numFuture == 0) && (numSnaps < lastNumSnaps))
	{
		notice = tr("Discarded %1 snapshot(s) after frame %2").arg(lastNumSnaps - numSnaps).arg(lastCurrentFrame);
		noticeTimer.start();

		// Nothing else may trigger a refresh by then, so schedule one to clear the notice
		QTimer::singleShot(NOTICE_DURATION_MS + 50, this, [this](){ requestRefresh(); });
	}
	lastNumSnaps = numSnaps;
	lastNumFuture = numFuture;
	lastCurrentSnap = data.currentSnap;
	lastCurrentFrame = data.currentFrame;

	FCEU_PROFILE_FUNC("Timeline dock refresh widget updates");

	rewindButton->setEnabled(running && (numSnaps > 0));
	forwardButton->setEnabled(running && (data.currentSnap >= 0) && (data.currentSnap + 1 < numSnaps));
	playButton->setEnabled(running);

	if (paused != wasPaused)
	{
		wasPaused = paused;
		playButton->setIcon(paused ? playIcon : pauseIcon);
	}
	if (numFuture > 0)
	{
		playButton->setToolTip(tr("Play (discards %1 snapshot(s) ahead of the current position)").arg(numFuture));
	}
	else
	{
		playButton->setToolTip(paused ? tr("Play") : tr("Pause"));
	}

	if (!running)
	{
		infoLabel->setText(tr("State recorder\nis not running"));
	}
	else if (noticeTimer.isValid() && !noticeTimer.hasExpired(NOTICE_DURATION_MS))
	{
		infoLabel->setText(notice);
	}
	else
	{
		const int pos = (data.currentSnap >= 0) ? (data.currentSnap + 1) : numSnaps;
		const int totalSec = static_cast<int>(std::max(0, data.currentFrame) / (hz > 1.0 ? hz : 60.0));
		const QString time = tr("%1:%2").arg(totalSec / 60).arg(totalSec % 60, 2, 10, QChar('0'));

		// The exact frame changes every frame while running, and every label change
		// costs a repaint, so it is only shown when stopped or rewound.
		if (paused || (numFuture > 0))
		{
			infoLabel->setText(tr("Frame %1 (%2)\nSnapshot %3 of %4").arg(data.currentFrame).arg(time).arg(pos).arg(numSnaps));
		}
		else
		{
			infoLabel->setText(tr("Running (%1)\nSnapshot %2 of %3").arg(time).arg(pos).arg(numSnaps));
		}
	}
}
//----------------------------------------------------------------------------
void StateRecorderTimelineDock_t::stepSnapshot(int direction)
{
	FCEU_PROFILE_FUNC("Timeline dock step snapshot");
	{
		FCEU_WRAPPER_SCOPED_LOCK();

		StateRecorderTimelineData data;

		if ((FCEU_StateRecorderGetTimeline(data) != 0) || data.snapFrames.empty())
		{
			return;
		}
		const int numSnaps = static_cast<int>(data.snapFrames.size());
		const int cur = data.currentSnap;
		int target;

		if (direction < 0)
		{
			if (cur < 0)
			{
				target = numSnaps - 1;
			}
			else
			{	// Still running past the last snapshot, so that one is the first step back
				target = (data.currentFrame > data.snapFrames[cur]) ? cur : (cur - 1);
			}
			target = std::max(target, 0);
		}
		else
		{
			target = cur + 1;

			if ((cur < 0) || (target >= numSnaps))
			{
				return;
			}
		}
		// Hold on the loaded snapshot so the history ahead of it stays until play is pressed
		FCEUI_SetEmulationPaused(EMULATIONPAUSED_PAUSED);
		FCEU_StateRecorderLoadPosition(target);
	}
	refresh();
}
//----------------------------------------------------------------------------
void StateRecorderTimelineDock_t::rewindStep(void)
{
	stepSnapshot(-1);
}
//----------------------------------------------------------------------------
void StateRecorderTimelineDock_t::forwardStep(void)
{
	stepSnapshot(1);
}
//----------------------------------------------------------------------------
void StateRecorderTimelineDock_t::togglePlay(void)
{
	{
		FCEU_WRAPPER_SCOPED_LOCK();

		if (EmulationPaused & EMULATIONPAUSED_TIMER)
		{
			FCEUI_SetEmulationPaused(0);
		}
		else
		{
			FCEUI_ToggleEmulationPause();
		}
	}
	refresh();
}
//----------------------------------------------------------------------------
void StateRecorderTimelineDock_t::seekTo(int pos)
{
	FCEU_PROFILE_FUNC("Timeline dock seek");
	{
		FCEU_WRAPPER_SCOPED_LOCK();

		FCEUI_SetEmulationPaused(EMULATIONPAUSED_PAUSED);
		FCEU_StateRecorderLoadPosition(pos);
	}
	refresh();
}
//----------------------------------------------------------------------------
