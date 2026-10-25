// ProfilerViewer.h
//
// Timeline widget: time on the horizontal axis, one lane per thread, nested markers
// drawn as rows below their parent.

#pragma once


#include <memory>
#include <utility>
#include <vector>

#include <QAbstractScrollArea>
#include <QColor>
#include <QPoint>
#include <QString>

#include "Qt/ProfilerCaptureFile.h"

class ProfilerViewer_t : public QAbstractScrollArea
{
	Q_OBJECT

public:
	ProfilerViewer_t(QWidget *parent = nullptr);
	~ProfilerViewer_t(void);

	void setData(std::shared_ptr<const ProfilerCaptureData> data);
	std::shared_ptr<const ProfilerCaptureData> getData(void) const { return data; }

	// Frames are all blocks of the chosen marker site, across threads, in time order.
	void setFrameSite(int site);
	int  frameCount(void) const { return static_cast<int>(frames.size()); }
	void gotoFrame(int index);
	QString frameDescription(int index) const;

	// Marker search. The text is matched case-insensitively against each marker's
	// label, function and comment; empty text matches every marker.
	void setFindText(const QString &text);

protected:
	void paintEvent(QPaintEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;

private:
	int  plotWidth(void) const;
	int  plotHeight(void) const;
	double spanCounts(void) const;
	double maxCountsPerPixel(void) const;
	void clampView(void);
	void zoomAt(double factor, double anchorX);
	void relayout(void);
	void updateScrollBars(void);
	int  laneAtContentY(int contentY) const;
	bool hitTest(const QPoint &pos, int &lane, int &depth, const ProfilerBlock **block) const;
	QString blockToolTip(const ProfilerBlock &block) const;
	QString blockSummary(const ProfilerBlock &block) const;
	void drawRow(QPainter &p, const std::vector<ProfilerBlock> &row, int y);
	void drawRuler(QPainter &p);
	void drawLabels(QPainter &p);
	void drawFrameMarkers(QPainter &p);
	void drawMeasure(QPainter &p);
	int  frameAtTime(double counts) const;
	double timeAtPos(const QPoint &pos) const;
	double snapTime(const QPoint &pos) const;

	struct BlockRef
	{
		uint32_t thread;
		uint32_t depth;
		const ProfilerBlock *block;
	};
	void buildBlockIndex(void);
	bool siteMatches(uint32_t site) const;
	bool findStep(int dir, bool fromEnd);
	bool findLongestFrom(int rank);
	void selectBlock(int index);

	std::shared_ptr<const ProfilerCaptureData> data;
	std::vector<QColor>  siteColors;
	std::vector<QString> siteLabels;
	std::vector<bool>    collapsed;
	std::vector<int>     laneTop;
	int contentHeight = 0;

	double viewStart = 0.0;       // counts
	double countsPerPixel = 1.0;
	bool   fitMode = true;
	bool   updatingScroll = false;

	int frameSite = -1;
	int frameSel = -1;
	std::vector<std::pair<uint64_t, uint64_t>> frames;

	bool    dragging = false;
	bool    dragMoved = false;
	QPoint  dragPressPos;
	double  dragViewStart = 0.0;
	int     dragScroll = 0;
	const ProfilerBlock *hoverBlock = nullptr;

	std::vector<BlockRef> allBlocks;      // every block, ordered by start time
	std::vector<int>      longestOrder;   // indices into allBlocks, longest first
	std::vector<bool>     siteMatch;
	QString findFilter;
	int  foundIndex = -1;
	int  longestRank = -1;
	int  longestMatchNumber = 0;
	const ProfilerBlock *foundBlock = nullptr;

	// Time measurement: a click sets the reference, the label follows the cursor
	bool    refActive = false;
	double  refCounts = 0.0;
	bool    cursorValid = false;
	QPoint  cursorPos;

signals:
	// Emitted when the selected frame changes because a reference was set inside it
	void frameSelected(int index);
	// Describes the outcome of the last find operation
	void findResult(const QString &text);

public slots:
	bool findFirst(void);
	bool findLast(void);
	bool findNext(void);
	bool findPrevious(void);
	bool findLongest(void);
	bool findNextLongest(void);
	void zoomIn(void);
	void zoomOut(void);
	void zoomFit(void);

private slots:
	void hScrollChanged(int value);
	void vScrollChanged(int value);
};


