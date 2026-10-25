// ProfilerViewer.cpp
//

#include <math.h>
#include <algorithm>

#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QToolTip>

#include "Qt/ProfilerViewer.h"

namespace
{
	constexpr int    RULER_H = 26;
	constexpr int    LABEL_W = 150;
	constexpr int    ROW_H = 20;
	constexpr int    LANE_PAD = 8;
	constexpr int    MIN_TEXT_W = 28;
	constexpr int    SNAP_PIXELS = 6;
	constexpr int    SCROLL_RES = 100000;
	constexpr double MIN_COUNTS_PER_PIXEL = 0.25;

	QPoint eventPos(const QMouseEvent *e)
	{
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
		return e->position().toPoint();
#else
		return e->pos();
#endif
	}

	QString formatSeconds(double sec, int decimals = 3)
	{
		const double a = fabs(sec);

		if (a >= 1.0)  { return QString::number(sec, 'f', decimals) + " s"; }
		if (a >= 1e-3) { return QString::number(sec * 1e3, 'f', decimals) + " ms"; }
		if (a >= 1e-6) { return QString::number(sec * 1e6, 'f', decimals) + " us"; }
		return QString::number(sec * 1e9, 'f', 0) + " ns";
	}

	double niceStep(double minStep)
	{
		const double mag = pow(10.0, floor(log10(minStep)));
		const double norm = minStep / mag;

		if (norm <= 1.0) { return mag; }
		if (norm <= 2.0) { return 2.0 * mag; }
		if (norm <= 5.0) { return 5.0 * mag; }
		return 10.0 * mag;
	}
}
//----------------------------------------------------------------------------
ProfilerViewer_t::ProfilerViewer_t(QWidget *parent)
	: QAbstractScrollArea(parent)
{
	setMouseTracking(true);
	setFocusPolicy(Qt::StrongFocus);
	viewport()->setMouseTracking(true);
	setMinimumSize(400, 200);

	horizontalScrollBar()->setRange(0, 0);
	connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hScrollChanged(int)));
	connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(vScrollChanged(int)));
}
//----------------------------------------------------------------------------
ProfilerViewer_t::~ProfilerViewer_t(void)
{
}
//----------------------------------------------------------------------------
int ProfilerViewer_t::plotWidth(void) const
{
	return std::max(1, viewport()->width() - LABEL_W);
}
//----------------------------------------------------------------------------
int ProfilerViewer_t::plotHeight(void) const
{
	return std::max(1, viewport()->height() - RULER_H);
}
//----------------------------------------------------------------------------
double ProfilerViewer_t::spanCounts(void) const
{
	return plotWidth() * countsPerPixel;
}
//----------------------------------------------------------------------------
double ProfilerViewer_t::maxCountsPerPixel(void) const
{
	if (!data)
	{
		return 1.0;
	}
	return std::max(MIN_COUNTS_PER_PIXEL, static_cast<double>(data->duration) / plotWidth());
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::clampView(void)
{
	if (!data)
	{
		return;
	}
	countsPerPixel = std::min(std::max(countsPerPixel, MIN_COUNTS_PER_PIXEL), maxCountsPerPixel());

	const double maxStart = std::max(0.0, static_cast<double>(data->duration) - spanCounts());
	viewStart = std::min(std::max(viewStart, 0.0), maxStart);
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::setData(std::shared_ptr<const ProfilerCaptureData> newData)
{
	data = newData;
	hoverBlock = nullptr;
	refActive = false;
	frames.clear();
	allBlocks.clear();
	longestOrder.clear();
	siteMatch.clear();
	findFilter.clear();
	foundIndex = -1;
	longestRank = -1;
	foundBlock = nullptr;
	frameSite = -1;
	frameSel = -1;
	siteColors.clear();
	siteLabels.clear();
	collapsed.clear();

	if (data)
	{
		for (const ProfilerSite &s : data->sites)
		{
			QString label = s.comment.isEmpty() ? s.func : s.comment;
			const uint32_t h = qHash(label + s.func);

			siteLabels.push_back(label);
			siteColors.push_back(QColor::fromHsv(static_cast<int>(h % 360), 110, 215));
		}
		collapsed.assign(data->threads.size(), false);
	}
	relayout();
	zoomFit();
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::relayout(void)
{
	laneTop.clear();
	contentHeight = 0;

	if (data)
	{
		for (size_t i = 0; i < data->threads.size(); i++)
		{
			const size_t rows = collapsed[i] ? 1 : std::max<size_t>(1, data->threads[i].depths.size());

			laneTop.push_back(contentHeight);
			contentHeight += static_cast<int>(rows) * ROW_H + LANE_PAD;
		}
	}
	verticalScrollBar()->setRange(0, std::max(0, contentHeight - plotHeight()));
	verticalScrollBar()->setPageStep(plotHeight());
	verticalScrollBar()->setSingleStep(ROW_H);
	viewport()->update();
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::updateScrollBars(void)
{
	QScrollBar *sb = horizontalScrollBar();
	updatingScroll = true;

	if (data && static_cast<double>(data->duration) > spanCounts())
	{
		const double total = static_cast<double>(data->duration);
		const double scrollable = total - spanCounts();

		sb->setRange(0, SCROLL_RES);
		sb->setPageStep(std::max(1, static_cast<int>(SCROLL_RES * spanCounts() / scrollable)));
		sb->setValue(static_cast<int>(viewStart / scrollable * SCROLL_RES));
	}
	else
	{
		sb->setRange(0, 0);
	}
	updatingScroll = false;
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::hScrollChanged(int value)
{
	if (data && !updatingScroll)
	{
		const double scrollable = static_cast<double>(data->duration) - spanCounts();

		viewStart = scrollable * value / SCROLL_RES;
		fitMode = false;
		clampView();
		viewport()->update();
	}
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::vScrollChanged(int)
{
	viewport()->update();
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::zoomAt(double factor, double anchorX)
{
	if (!data)
	{
		return;
	}
	// Keep the measurement reference where it is on screen while it is visible
	if (refActive)
	{
		const double refX = (refCounts - viewStart) / countsPerPixel;

		if (refX >= 0.0 && refX <= plotWidth())
		{
			anchorX = refX;
		}
	}
	const double anchorCounts = viewStart + anchorX * countsPerPixel;

	countsPerPixel /= factor;
	countsPerPixel = std::min(std::max(countsPerPixel, MIN_COUNTS_PER_PIXEL), maxCountsPerPixel());
	viewStart = anchorCounts - anchorX * countsPerPixel;
	fitMode = false;
	clampView();
	updateScrollBars();
	viewport()->update();
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::zoomIn(void)
{
	zoomAt(2.0, plotWidth() / 2.0);
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::zoomOut(void)
{
	zoomAt(0.5, plotWidth() / 2.0);
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::zoomFit(void)
{
	fitMode = true;
	viewStart = 0.0;
	countsPerPixel = maxCountsPerPixel();
	clampView();
	updateScrollBars();
	viewport()->update();
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::setFrameSite(int site)
{
	frames.clear();
	frameSite = site;
	frameSel = -1;

	if (data && site >= 0)
	{
		for (const ProfilerThreadData &t : data->threads)
		{
			for (const auto &row : t.depths)
			{
				for (const ProfilerBlock &b : row)
				{
					if (static_cast<int>(b.site) == site)
					{
						frames.emplace_back(b.start, b.end);
					}
				}
			}
		}
		std::sort(frames.begin(), frames.end());
	}
	viewport()->update();
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::gotoFrame(int index)
{
	if (!data || index < 0 || index >= frameCount())
	{
		return;
	}
	frameSel = index;

	const double start = static_cast<double>(frames[index].first);
	const double span = std::max(1.0, static_cast<double>(frames[index].second - frames[index].first));

	// Show the frame with a margin of 12.5% on each side
	countsPerPixel = (span * 1.25) / plotWidth();
	viewStart = start - span * 0.125;
	fitMode = false;
	clampView();
	updateScrollBars();
	viewport()->update();
}
//----------------------------------------------------------------------------
int ProfilerViewer_t::frameAtTime(double counts) const
{
	// frames is sorted by start time
	auto it = std::upper_bound(frames.begin(), frames.end(), counts,
		[](double t, const std::pair<uint64_t, uint64_t> &f){ return t < static_cast<double>(f.first); });

	if (it == frames.begin())
	{
		return -1;
	}
	--it;
	return (counts <= static_cast<double>(it->second)) ? static_cast<int>(it - frames.begin()) : -1;
}
//----------------------------------------------------------------------------
QString ProfilerViewer_t::frameDescription(int index) const
{
	if (!data || index < 0 || index >= frameCount())
	{
		return QString();
	}
	const double dur = data->countsToSeconds(frames[index].second - frames[index].first);
	const double start = data->countsToSeconds(frames[index].first);

	return tr("Frame %1 of %2: %3 long, starts at %4")
		.arg(index + 1).arg(frameCount()).arg(formatSeconds(dur)).arg(formatSeconds(start));
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::resizeEvent(QResizeEvent *event)
{
	QAbstractScrollArea::resizeEvent(event);

	if (fitMode)
	{
		zoomFit();
	}
	else
	{
		clampView();
		updateScrollBars();
	}
	relayout();
}
//----------------------------------------------------------------------------
int ProfilerViewer_t::laneAtContentY(int contentY) const
{
	for (size_t i = 0; i < laneTop.size(); i++)
	{
		const int bottom = (i + 1 < laneTop.size()) ? laneTop[i + 1] : contentHeight;

		if (contentY >= laneTop[i] && contentY < bottom)
		{
			return static_cast<int>(i);
		}
	}
	return -1;
}
//----------------------------------------------------------------------------
bool ProfilerViewer_t::hitTest(const QPoint &pos, int &lane, int &depth, const ProfilerBlock **block) const
{
	*block = nullptr;

	if (!data || pos.x() < LABEL_W || pos.y() < RULER_H)
	{
		return false;
	}
	lane = laneAtContentY(pos.y() - RULER_H + verticalScrollBar()->value());
	if (lane < 0)
	{
		return false;
	}
	const int rowY = pos.y() - RULER_H + verticalScrollBar()->value() - laneTop[lane];
	depth = rowY / ROW_H;

	const auto &depths = data->threads[lane].depths;
	if (depth >= static_cast<int>(depths.size()) || (collapsed[lane] && depth > 0))
	{
		return false;
	}
	const double t = viewStart + (pos.x() - LABEL_W) * countsPerPixel;
	const auto &row = depths[depth];
	auto it = std::lower_bound(row.begin(), row.end(), t,
		[](const ProfilerBlock &b, double time){ return static_cast<double>(b.end) < time; });

	// Allow one pixel of slack so very short blocks can still be hovered
	if (it != row.end() && static_cast<double>(it->start) <= t + countsPerPixel)
	{
		*block = &(*it);
		return true;
	}
	return false;
}
//----------------------------------------------------------------------------
double ProfilerViewer_t::timeAtPos(const QPoint &pos) const
{
	return viewStart + (pos.x() - LABEL_W) * countsPerPixel;
}
//----------------------------------------------------------------------------
// Returns the time under the cursor, snapped to the nearest block edge in the
// hovered lane when one is within SNAP_PIXELS.
double ProfilerViewer_t::snapTime(const QPoint &pos) const
{
	const double t = timeAtPos(pos);
	const int lane = laneAtContentY(pos.y() - RULER_H + verticalScrollBar()->value());

	if (!data || lane < 0)
	{
		return t;
	}
	const double range = SNAP_PIXELS * countsPerPixel;
	double best = t;
	double bestDist = range;
	const auto &depths = data->threads[lane].depths;
	const size_t rows = collapsed[lane] ? std::min<size_t>(1, depths.size()) : depths.size();

	for (size_t d = 0; d < rows; d++)
	{
		const auto &row = depths[d];
		// First block whose end is not before the window; any edge in the window belongs to it or the next ones
		auto it = std::lower_bound(row.begin(), row.end(), t - range,
			[](const ProfilerBlock &b, double time){ return static_cast<double>(b.end) < time; });

		for (; it != row.end() && static_cast<double>(it->start) <= t + range; ++it)
		{
			const double edges[2] = { static_cast<double>(it->start), static_cast<double>(it->end) };

			for (double e : edges)
			{
				const double dist = fabs(e - t);

				if (dist < bestDist)
				{
					bestDist = dist;
					best = e;
				}
			}
		}
	}
	return best;
}
//----------------------------------------------------------------------------
QString ProfilerViewer_t::blockToolTip(const ProfilerBlock &block) const
{
	const ProfilerSite &s = data->sites[block.site];
	const QString name = s.comment.isEmpty() ? s.func : s.comment;

	return QString("<b>%1</b><br>%2<br>%3:%4<br>Duration: %5<br>Start: %6")
		.arg(name.toHtmlEscaped())
		.arg(s.func.toHtmlEscaped())
		.arg(s.file.toHtmlEscaped())
		.arg(s.line)
		.arg(formatSeconds(data->countsToSeconds(block.end - block.start)))
		.arg(formatSeconds(data->countsToSeconds(block.start)));
}
//----------------------------------------------------------------------------
QString ProfilerViewer_t::blockSummary(const ProfilerBlock &block) const
{
	const ProfilerSite &s = data->sites[block.site];

	return QString("%1: %2 at %3")
		.arg(s.comment.isEmpty() ? s.func : s.comment)
		.arg(formatSeconds(data->countsToSeconds(block.end - block.start)))
		.arg(formatSeconds(data->countsToSeconds(block.start)));
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		dragging = true;
		dragMoved = false;
		dragPressPos = eventPos(event);
		dragViewStart = viewStart;
		dragScroll = verticalScrollBar()->value();
	}
	else if (event->button() == Qt::RightButton && refActive)
	{
		refActive = false;
		viewport()->update();
	}
	QAbstractScrollArea::mousePressEvent(event);
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::mouseMoveEvent(QMouseEvent *event)
{
	const QPoint pos = eventPos(event);

	cursorPos = pos;
	cursorValid = true;
	if (refActive)
	{
		viewport()->update();
	}

	if (dragging && data)
	{
		const QPoint d = pos - dragPressPos;

		if (dragMoved || d.manhattanLength() > 3)
		{
			dragMoved = true;
			viewport()->setCursor(Qt::ClosedHandCursor);
			QToolTip::hideText();

			viewStart = dragViewStart - d.x() * countsPerPixel;
			fitMode = false;
			clampView();
			updateScrollBars();
			verticalScrollBar()->setValue(dragScroll - d.y());
			viewport()->update();
		}
		return;
	}

	int lane = 0, depth = 0;
	const ProfilerBlock *block = nullptr;

	hitTest(pos, lane, depth, &block);

	if (block != hoverBlock)
	{
		hoverBlock = block;
		QToolTip::hideText();
	}
	if (block && !refActive)
	{
		QToolTip::showText(viewport()->mapToGlobal(pos), blockToolTip(*block), viewport());
	}
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		const QPoint pos = eventPos(event);

		// A click (no drag) on a thread label collapses or expands that lane
		if (dragging && !dragMoved && data && pos.x() < LABEL_W && pos.y() >= RULER_H)
		{
			const int lane = laneAtContentY(pos.y() - RULER_H + verticalScrollBar()->value());

			if (lane >= 0)
			{
				collapsed[lane] = !collapsed[lane];
				relayout();
			}
		}
		// A click (no drag) in the plot area sets the reference time; the next click clears it
		if (dragging && !dragMoved && data && pos.x() >= LABEL_W && pos.y() >= RULER_H)
		{
			refActive = !refActive;
			if (refActive)
			{
				refCounts = snapTime(pos);
				QToolTip::hideText();

				const int frame = frameAtTime(refCounts);
				if (frame >= 0 && frame != frameSel)
				{
					frameSel = frame;
					emit frameSelected(frame);
				}
			}
			viewport()->update();
		}
		dragging = false;
		viewport()->unsetCursor();
	}
	QAbstractScrollArea::mouseReleaseEvent(event);
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::leaveEvent(QEvent *event)
{
	hoverBlock = nullptr;
	cursorValid = false;
	QToolTip::hideText();
	viewport()->update();
	QAbstractScrollArea::leaveEvent(event);
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape && refActive)
	{
		refActive = false;
		viewport()->update();
		event->accept();
		return;
	}
	QAbstractScrollArea::keyPressEvent(event);
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::wheelEvent(QWheelEvent *event)
{
	const int delta = event->angleDelta().y();

	if (delta == 0)
	{
		return;
	}
	if (event->modifiers() & Qt::ShiftModifier)
	{
		QScrollBar *sb = verticalScrollBar();
		sb->setValue(sb->value() - (delta / 120) * ROW_H * 3);
	}
	else
	{
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
		const double x = event->position().x();
#else
		const double x = event->pos().x();
#endif
		zoomAt(pow(1.2, delta / 120.0), std::max(0.0, x - LABEL_W));
	}
	event->accept();
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::buildBlockIndex(void)
{
	if (!allBlocks.empty() || !data)
	{
		return;
	}
	for (size_t t = 0; t < data->threads.size(); t++)
	{
		const auto &depths = data->threads[t].depths;

		for (size_t d = 0; d < depths.size(); d++)
		{
			for (const ProfilerBlock &b : depths[d])
			{
				allBlocks.push_back({static_cast<uint32_t>(t), static_cast<uint32_t>(d), &b});
			}
		}
	}
	std::sort(allBlocks.begin(), allBlocks.end(), [](const BlockRef &a, const BlockRef &b)
	{
		if (a.block->start != b.block->start) return a.block->start < b.block->start;
		if (a.thread != b.thread) return a.thread < b.thread;
		return a.depth < b.depth;
	});
	longestOrder.resize(allBlocks.size());
	for (size_t i = 0; i < longestOrder.size(); i++)
	{
		longestOrder[i] = static_cast<int>(i);
	}
	std::stable_sort(longestOrder.begin(), longestOrder.end(), [this](int a, int b)
	{
		const ProfilerBlock *x = allBlocks[a].block, *y = allBlocks[b].block;
		return (x->end - x->start) > (y->end - y->start);
	});
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::setFindText(const QString &text)
{
	findFilter = text.trimmed();
	siteMatch.assign(data ? data->sites.size() : 0, false);

	for (size_t i = 0; i < siteMatch.size(); i++)
	{
		const ProfilerSite &s = data->sites[i];

		siteMatch[i] = findFilter.isEmpty() ||
			siteLabels[i].contains(findFilter, Qt::CaseInsensitive) ||
			s.func.contains(findFilter, Qt::CaseInsensitive) ||
			s.comment.contains(findFilter, Qt::CaseInsensitive);
	}
}
//----------------------------------------------------------------------------
bool ProfilerViewer_t::siteMatches(uint32_t site) const
{
	return site < siteMatch.size() ? static_cast<bool>(siteMatch[site]) : findFilter.isEmpty();
}
//----------------------------------------------------------------------------
// Searches from the current result (or the capture edge) in one direction, wrapping once
bool ProfilerViewer_t::findStep(int dir, bool fromEnd)
{
	buildBlockIndex();
	if (allBlocks.empty())
	{
		emit findResult(tr("No capture loaded"));
		return false;
	}
	const int n = static_cast<int>(allBlocks.size());
	int start;

	if (foundIndex < 0 || fromEnd)
	{
		start = (dir > 0) ? 0 : n - 1;
	}
	else
	{
		start = foundIndex + dir;
	}
	bool wrapped = false;

	for (int count = 0; count < n + 1; count++)
	{
		int i = start + dir * count;

		if (i < 0 || i >= n)
		{
			if (foundIndex < 0 || fromEnd)
			{
				break;
			}
			i = (i + n) % n;
			wrapped = true;
		}
		if (siteMatches(allBlocks[i].block->site))
		{
			longestRank = -1;
			selectBlock(i);
			if (wrapped)
			{
				emit findResult(tr("Wrapped. ") + blockSummary(*allBlocks[i].block));
			}
			return true;
		}
	}
	emit findResult(tr("No markers match \"%1\"").arg(findFilter));
	return false;
}
//----------------------------------------------------------------------------
bool ProfilerViewer_t::findFirst(void){ return findStep(1, true); }
bool ProfilerViewer_t::findLast(void){ return findStep(-1, true); }
bool ProfilerViewer_t::findNext(void){ return findStep(1, false); }
bool ProfilerViewer_t::findPrevious(void){ return findStep(-1, false); }
//----------------------------------------------------------------------------
bool ProfilerViewer_t::findLongest(void)
{
	buildBlockIndex();
	if (longestOrder.empty())
	{
		emit findResult(tr("No capture loaded"));
		return false;
	}
	longestRank = 0;
	selectBlock(longestOrder[0]);
	return true;
}
//----------------------------------------------------------------------------
bool ProfilerViewer_t::findNextLongest(void)
{
	buildBlockIndex();
	if (longestOrder.empty())
	{
		emit findResult(tr("No capture loaded"));
		return false;
	}
	if (longestRank + 1 >= static_cast<int>(longestOrder.size()))
	{
		emit findResult(tr("No more markers"));
		return false;
	}
	longestRank++;
	selectBlock(longestOrder[longestRank]);
	return true;
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::selectBlock(int index)
{
	const BlockRef &ref = allBlocks[index];
	const double start = static_cast<double>(ref.block->start);
	const double mid = (start + static_cast<double>(ref.block->end)) / 2.0;

	foundIndex = index;
	foundBlock = ref.block;

	// Keep the zoom level and center the block, unless it is wider than the view
	if (static_cast<double>(ref.block->end - ref.block->start) > spanCounts())
	{
		viewStart = start - spanCounts() * 0.05;
	}
	else
	{
		viewStart = mid - spanCounts() / 2.0;
	}
	fitMode = false;
	clampView();
	updateScrollBars();

	if (ref.depth > 0 && collapsed[ref.thread])
	{
		collapsed[ref.thread] = false;
		relayout();
	}
	const int rowTop = laneTop[ref.thread] + static_cast<int>(ref.depth) * ROW_H;
	QScrollBar *vs = verticalScrollBar();

	if (rowTop < vs->value())
	{
		vs->setValue(rowTop);
	}
	else if (rowTop + ROW_H > vs->value() + plotHeight())
	{
		vs->setValue(rowTop + ROW_H - plotHeight());
	}
	refActive = true;
	refCounts = start;

	const int frame = frameAtTime(start);
	if (frame >= 0 && frame != frameSel)
	{
		frameSel = frame;
		emit frameSelected(frame);
	}
	viewport()->update();

	QString text = blockSummary(*ref.block);
	if (longestRank >= 0)
	{
		text += tr("  (longest #%1)").arg(longestRank + 1);
	}
	emit findResult(text);
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::drawRow(QPainter &p, const std::vector<ProfilerBlock> &row, int y)
{
	const double t0 = viewStart;
	const double t1 = viewStart + spanCounts();
	const int left = LABEL_W;
	const int right = viewport()->width();
	const QFontMetrics fm = p.fontMetrics();

	auto it = std::lower_bound(row.begin(), row.end(), t0,
		[](const ProfilerBlock &b, double t){ return static_cast<double>(b.end) < t; });

	while (it != row.end() && static_cast<double>(it->start) <= t1)
	{
		const double x0 = LABEL_W + (static_cast<double>(it->start) - viewStart) / countsPerPixel;
		const double x1 = LABEL_W + (static_cast<double>(it->end) - viewStart) / countsPerPixel;
		const QColor &color = siteColors[it->site];

		if (x1 - x0 < 1.0)
		{
			// Sub-pixel block: draw a single pixel column and skip everything that ends in it
			const int px = std::max(left, static_cast<int>(floor(x0)));
			const double nextT = viewStart + ((px - LABEL_W) + 1) * countsPerPixel;

			p.fillRect(px, y, 1, ROW_H - 2, color.darker(130));
			it = std::lower_bound(it + 1, row.end(), nextT,
				[](const ProfilerBlock &b, double t){ return static_cast<double>(b.end) < t; });
			continue;
		}
		const double cx0 = std::max(x0, static_cast<double>(left));
		const double cx1 = std::min(x1, static_cast<double>(right));
		const QRectF r(cx0, y, cx1 - cx0, ROW_H - 2);

		p.fillRect(r, color);
		p.setPen(color.darker(170));
		p.drawRect(r);
		if (&*it == foundBlock)
		{
			p.setPen(QPen(QColor(255, 200, 0), 2));
			p.drawRect(r.adjusted(1, 1, -1, -1));
		}

		if (r.width() >= MIN_TEXT_W)
		{
			p.setPen(Qt::black);
			p.drawText(QRectF(r.left() + 3, r.top(), r.width() - 6, r.height()),
				Qt::AlignVCenter | Qt::AlignLeft,
				fm.elidedText(siteLabels[it->site], Qt::ElideRight, static_cast<int>(r.width()) - 6));
		}
		++it;
	}
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::drawRuler(QPainter &p)
{
	const int w = viewport()->width();
	const double freq = static_cast<double>(data->countFreq);

	p.fillRect(0, 0, w, RULER_H, palette().window());
	p.setPen(palette().color(QPalette::Mid));
	p.drawLine(0, RULER_H - 1, w, RULER_H - 1);

	const double secPerPixel = countsPerPixel / freq;
	const double step = niceStep(secPerPixel * 90.0);
	const double startSec = viewStart / freq;
	const double endSec = startSec + plotWidth() * secPerPixel;

	// Pick the label unit from the step size so every tick uses the same precision
	double unit = 1.0;
	const char *unitName = "s";
	if (step < 1e-6)      { unit = 1e-9; unitName = "ns"; }
	else if (step < 1e-3) { unit = 1e-6; unitName = "us"; }
	else if (step < 1.0)  { unit = 1e-3; unitName = "ms"; }
	const int decimals = std::min(6, std::max(0, static_cast<int>(ceil(-log10(step / unit) - 1e-9))));

	p.setClipRect(LABEL_W, 0, plotWidth(), RULER_H);
	p.setPen(palette().color(QPalette::WindowText));

	for (double t = floor(startSec / step) * step; t <= endSec + step; t += step)
	{
		const int x = LABEL_W + static_cast<int>((t - startSec) / secPerPixel);

		if (x < LABEL_W)
		{
			continue;
		}

		p.drawLine(x, RULER_H - 8, x, RULER_H - 1);
		p.drawText(x + 3, 0, 120, RULER_H - 8, Qt::AlignVCenter | Qt::AlignLeft,
			QString::number(t / unit, 'f', decimals) + " " + unitName);
	}
	p.setClipping(false);
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::drawLabels(QPainter &p)
{
	const int h = viewport()->height();
	const int vscroll = verticalScrollBar()->value();
	const QFontMetrics fm = p.fontMetrics();

	p.setClipRect(0, RULER_H, LABEL_W, h - RULER_H);
	p.fillRect(0, RULER_H, LABEL_W, h - RULER_H, palette().window());

	for (size_t i = 0; i < data->threads.size(); i++)
	{
		const int y = RULER_H + laneTop[i] - vscroll;
		const ProfilerThreadData &t = data->threads[i];
		QString name = t.name.isEmpty() ? tr("Thread %1").arg(t.id) : t.name;

		p.setPen(palette().color(QPalette::WindowText));
		p.drawText(QRect(4, y, LABEL_W - 8, ROW_H), Qt::AlignVCenter | Qt::AlignLeft,
			fm.elidedText((collapsed[i] ? QString::fromUtf8("\u25B8 ") : QString::fromUtf8("\u25BE ")) + name,
				Qt::ElideRight, LABEL_W - 8));
	}
	p.setClipping(false);
	p.setPen(palette().color(QPalette::Mid));
	p.drawLine(LABEL_W - 1, 0, LABEL_W - 1, h);
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::drawFrameMarkers(QPainter &p)
{
	if (frameSel < 0 || frameSel >= frameCount())
	{
		return;
	}
	const double x0 = LABEL_W + (static_cast<double>(frames[frameSel].first) - viewStart) / countsPerPixel;
	const double x1 = LABEL_W + (static_cast<double>(frames[frameSel].second) - viewStart) / countsPerPixel;

	p.setClipRect(LABEL_W, 0, plotWidth(), viewport()->height());
	p.fillRect(QRectF(x0, RULER_H - 4, x1 - x0, 3), QColor(220, 60, 60));
	p.setPen(QPen(QColor(220, 60, 60), 1, Qt::DashLine));
	p.drawLine(QPointF(x0, RULER_H), QPointF(x0, viewport()->height()));
	p.drawLine(QPointF(x1, RULER_H), QPointF(x1, viewport()->height()));
	p.setClipping(false);
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::drawMeasure(QPainter &p)
{
	if (!refActive || !data)
	{
		return;
	}
	const double xRef = LABEL_W + (refCounts - viewStart) / countsPerPixel;
	const int h = viewport()->height();

	p.setClipRect(LABEL_W, RULER_H, plotWidth(), h - RULER_H);
	p.setPen(QPen(QColor(30, 110, 220), 1, Qt::SolidLine));
	p.drawLine(QPointF(xRef, RULER_H), QPointF(xRef, h));

	if (cursorValid && cursorPos.x() >= LABEL_W && cursorPos.y() >= RULER_H)
	{
		const double tCur = snapTime(cursorPos);
		const double xCur = LABEL_W + (tCur - viewStart) / countsPerPixel;
		const double delta = (tCur - refCounts) / static_cast<double>(data->countFreq);

		p.drawLine(QPointF(xCur, RULER_H), QPointF(xCur, h));
		p.fillRect(QRectF(std::min(xRef, xCur), RULER_H, fabs(xCur - xRef), 3), QColor(30, 110, 220));

		const QString text = QString::fromUtf8("\u0394 ") + (delta < 0 ? "-" : "") + formatSeconds(fabs(delta));
		const QFontMetrics fm = p.fontMetrics();
		QRect box(0, 0, fm.horizontalAdvance(text) + 12, fm.height() + 6);

		int bx = static_cast<int>(xCur) + 12;
		int by = cursorPos.y() + 14;
		if (bx + box.width() > viewport()->width()) { bx = static_cast<int>(xCur) - 12 - box.width(); }
		if (by + box.height() > h) { by = cursorPos.y() - 14 - box.height(); }
		box.moveTo(bx, by);

		p.setClipping(false);
		p.setPen(QColor(30, 110, 220));
		p.setBrush(palette().base());
		p.drawRoundedRect(box, 3, 3);
		p.setPen(palette().color(QPalette::Text));
		p.drawText(box, Qt::AlignCenter, text);
		p.setBrush(Qt::NoBrush);
	}
	p.setClipping(false);
}
//----------------------------------------------------------------------------
void ProfilerViewer_t::paintEvent(QPaintEvent *)
{
	QPainter p(viewport());

	p.fillRect(viewport()->rect(), palette().base());

	if (!data || data->threads.empty())
	{
		p.setPen(palette().color(QPalette::Text));
		p.drawText(viewport()->rect(), Qt::AlignCenter, tr("No capture loaded"));
		return;
	}
	const int vscroll = verticalScrollBar()->value();
	const int h = viewport()->height();

	p.setClipRect(LABEL_W, RULER_H, plotWidth(), h - RULER_H);

	for (size_t i = 0; i < data->threads.size(); i++)
	{
		const int top = RULER_H + laneTop[i] - vscroll;
		const int bottom = RULER_H + ((i + 1 < laneTop.size()) ? laneTop[i + 1] : contentHeight) - vscroll;

		if (bottom < RULER_H || top > h)
		{
			continue;
		}
		if (i & 1)
		{
			p.fillRect(LABEL_W, top, plotWidth(), bottom - top, palette().alternateBase());
		}
		const auto &depths = data->threads[i].depths;
		const size_t rows = collapsed[i] ? std::min<size_t>(1, depths.size()) : depths.size();

		for (size_t d = 0; d < rows; d++)
		{
			const int y = top + static_cast<int>(d) * ROW_H;

			if (y + ROW_H >= RULER_H && y <= h)
			{
				drawRow(p, depths[d], y);
			}
		}
		p.setPen(palette().color(QPalette::Midlight));
		p.drawLine(LABEL_W, bottom - 1, viewport()->width(), bottom - 1);
	}
	p.setClipping(false);

	drawLabels(p);
	drawRuler(p);
	drawFrameMarkers(p);
	drawMeasure(p);
}
//----------------------------------------------------------------------------

