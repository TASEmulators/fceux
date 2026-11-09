// StateRecorderTimeline.h
//
// Dockable timeline for the state recorder. Offers rewind, fast forward, play
// and pause controls along with a scrubbable view of the recorded history.
// Snapshots that lie ahead of the current position are shown as ghosted, since
// they are discarded as soon as emulation resumes.

#pragma once

#include <QDockWidget>
#include <QWidget>
#include <QToolButton>
#include <QLabel>
#include <QTimer>
#include <QElapsedTimer>
#include <QColor>
#include <QIcon>
#include <atomic>

#include "../../state.h"

class StateTimelineBar_t : public QWidget
{
	Q_OBJECT

public:
	StateTimelineBar_t(QWidget *parent = 0);

	QSize sizeHint(void) const override;
	QSize minimumSizeHint(void) const override;

	void setData(const StateRecorderTimelineData &newData, double framesPerSecond);

	// Number of snapshots after the current position, which are discarded on resume
	int numFutureSnaps(void) const;

	void setBgColor(const QColor &c);

signals:
	void seekRequested(int pos);

protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void leaveEvent(QEvent *event) override;
	bool event(QEvent *event) override;

private:
	QRectF trackRect(void) const;
	double frameToX(double frame, const QRectF &track) const;
	int    snapAtX(double x) const;
	QString formatFrame(int frame) const;
	void   seekToX(double x);

	QColor fgColor(void) const;
	QColor blendedFg(double t) const;

	QColor bgColor = Qt::black;
	StateRecorderTimelineData data;
	double hz = 60.0;
	int firstFrame = 0;
	int lastFrame = 1;
	int hoverSnap = -1;
	int lastSeekSnap = -1;
	bool dragging = false;

	// State at the last repaint. Repainting any widget makes Qt recomposite the whole
	// window, which is costly next to an OpenGL viewport, so the bar only repaints
	// when something would visibly move.
	int paintedNumSnaps = -1;
	int paintedFirstSnapFrame = 0;
	int paintedLastSnapFrame = 0;
	int paintedCurrentSnap = -1;
	int paintedCurrentFrame = 0;
	int paintedLastFrame = 0;
};

class StateRecorderTimelineDock_t : public QDockWidget
{
	Q_OBJECT

public:
	StateRecorderTimelineDock_t(QWidget *parent = 0);
	~StateRecorderTimelineDock_t(void);

	// Asks for a refresh. Safe to call from any thread. Requests are coalesced and the
	// dock refreshes at most once per MIN_REFRESH_INTERVAL_MS.
	void requestRefresh(void);

	static constexpr int MIN_REFRESH_INTERVAL_MS = 100;

protected:
	void showEvent(QShowEvent *event) override;

private:
	void stepSnapshot(int direction);
	void applyBgColor(const QColor &c);

	QColor bgColor = Qt::black;
	QWidget *container;
	QIcon rewindIcon, forwardIcon, playIcon, pauseIcon;

	StateTimelineBar_t *bar;
	QToolButton *rewindButton;
	QToolButton *playButton;
	QToolButton *forwardButton;
	QLabel      *infoLabel;
	QTimer      *throttleTimer;
	QElapsedTimer refreshTimer;
	std::atomic<bool> refreshQueued{false};

	int  lastNumSnaps = 0;
	int  lastNumFuture = 0;
	int  lastCurrentSnap = -1;
	int  lastCurrentFrame = 0;
	bool wasPaused = false;
	QString notice;
	QElapsedTimer noticeTimer;

private slots:
	void scheduleRefresh(void);
	void refresh(void);
	void rewindStep(void);
	void forwardStep(void);
	void togglePlay(void);
	void seekTo(int pos);
};
