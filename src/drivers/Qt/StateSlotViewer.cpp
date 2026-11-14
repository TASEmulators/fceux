// StateSlotViewer.cpp
//
#include <stdint.h>
#include <string.h>

#include <algorithm>

#include <zlib.h>

#include <QPainter>
#include <QColor>
#include <QPalette>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QDateTime>
#include <QFileInfo>
#include <QFile>
#include <QtEndian>
#include <QFontMetrics>

#include "../../fceu.h"
#include "../../file.h"
#include "../../state.h"
#include "../../driver.h"
#include "../../profiler.h"

#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/StateSlotViewer.h"

static constexpr int THUMB_MARGIN = 6;
static constexpr int THUMB_BORDER = 3;
static constexpr int NES_SCREEN_WIDTH = 256;
static constexpr int NES_SCREEN_HEIGHT = 240;

// A state file that was modified this recently may still be in the middle of
// being written, so a failed read is retried rather than remembered.
static constexpr qint64 RETRY_WINDOW_MS = 2000;

//----------------------------------------------------------------------------
// Savestates already embed the (overlay free) back buffer as chunk 8, so the
// screenshot of a saved state is recovered from the existing file format.
static bool readStateScreenshot(const std::string &path, uint8_t *dest)
{
	constexpr size_t screenSize = 256 * 256;
	constexpr uint32_t maxStateSize = 512u * 1024u * 1024u;

	QFile file(QString::fromStdString(path));

	if (!file.open(QIODevice::ReadOnly))
	{
		return false;
	}

	uint8_t header[16];
	if ((file.read(reinterpret_cast<char*>(header), 16) != 16) || memcmp(header, "FCSX", 4))
	{
		return false; // truncated, or pre-FCEUX format state without a screenshot
	}

	const uint32_t totalSize = qFromLittleEndian<uint32_t>(header + 4);
	const uint32_t comprLen  = qFromLittleEndian<uint32_t>(header + 12);

	if ((totalSize > maxStateSize) || ((comprLen != ~0u) && (comprLen > maxStateSize)))
	{
		return false;
	}

	std::vector<uint8_t> data(totalSize);

	if (comprLen != ~0u)
	{
		std::vector<uint8_t> cbuf(comprLen);

		if (file.read(reinterpret_cast<char*>(cbuf.data()), comprLen) != static_cast<qint64>(comprLen))
		{
			return false;
		}
		uLongf destLen = totalSize;

		if ((uncompress(data.data(), &destLen, cbuf.data(), comprLen) != Z_OK) || (destLen != totalSize))
		{
			return false;
		}
	}
	else if (file.read(reinterpret_cast<char*>(data.data()), totalSize) != static_cast<qint64>(totalSize))
	{
		return false;
	}

	size_t pos = 0;
	while ((pos + 5) <= data.size())
	{
		const uint8_t  type = data[pos];
		const uint32_t size = qFromLittleEndian<uint32_t>(&data[pos + 1]);
		pos += 5;

		if (size > (data.size() - pos))
		{
			break;
		}

		if (type == 8)
		{
			// Older states pad the buffer with 8 extra bytes
			if (size < screenSize)
			{
				return false;
			}
			memcpy(dest, &data[pos], screenSize);
			return true;
		}
		pos += size;
	}
	return false;
}
//----------------------------------------------------------------------------
StateSlotThumb_t::StateSlotThumb_t(QWidget *parent)
	: QWidget(parent)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setToolTip(tr("Click to select this slot, double click to load it."));
}
//----------------------------------------------------------------------------
bool StateSlotThumb_t::event(QEvent *event)
{
	if (event->type() == QEvent::ToolTip)
	{
		setToolTip(FCEU_HardcoreActive() ? tr("Click to select this slot. Loading states is disabled in Hardcore mode.")
			: tr("Click to select this slot, double click to load it."));
	}
	return QWidget::event(event);
}
//----------------------------------------------------------------------------
QSize StateSlotThumb_t::sizeHint(void) const
{
	return QSize(96, 100);
}
//----------------------------------------------------------------------------
QSize StateSlotThumb_t::minimumSizeHint(void) const
{
	return QSize(56, 64);
}
//----------------------------------------------------------------------------
// Holds the thumbnails. The number of thumbnails shown adapts to the space that
// is given, so the preferred size is a fixed modest value instead of one derived
// from the thumbnails, which would make the dock claim more room than needed.
class StateSlotContainer_t : public QWidget
{
public:
	StateSlotContainer_t(QWidget *parent = 0) : QWidget(parent) {}

	QSize sizeHint(void) const override
	{
		return horizontal ? QSize(360, 124) : QSize(124, 360);
	}

	bool horizontal = true;
};
//----------------------------------------------------------------------------
void StateSlotThumb_t::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);

	QPainter painter(this);

	if (!valid)
	{
		return;
	}
	const QFontMetrics fm(font());
	const int captionH = fm.height() + 4;

	painter.fillRect(rect(), bgColor);

	// Text and outlines are drawn in a color that contrasts with the background
	const QColor fg = (bgColor.lightness() < 128) ? QColor(Qt::white) : QColor(Qt::black);
	auto fgMix = [&](double t)
	{
		return QColor::fromRgbF(bgColor.redF() + (fg.redF() - bgColor.redF()) * t,
		                        bgColor.greenF() + (fg.greenF() - bgColor.greenF()) * t,
		                        bgColor.blueF() + (fg.blueF() - bgColor.blueF()) * t);
	};

	if (selected)
	{
		QPen pen(palette().highlight().color(), THUMB_BORDER);
		pen.setJoinStyle(Qt::MiterJoin);
		painter.setPen(pen);
		painter.drawRect(rect().adjusted(THUMB_BORDER / 2, THUMB_BORDER / 2,
		                                 -(THUMB_BORDER + 1) / 2, -(THUMB_BORDER + 1) / 2));
	}

	const QRect area = rect().adjusted(THUMB_MARGIN, THUMB_MARGIN, -THUMB_MARGIN, -THUMB_MARGIN - captionH);
	const QRect captionRect(THUMB_MARGIN, height() - THUMB_MARGIN - captionH, width() - 2 * THUMB_MARGIN, captionH);

	if (hasImage && !image.isNull() && area.width() > 0 && area.height() > 0)
	{
		QSize sz = image.size();
		sz.scale(area.size(), Qt::KeepAspectRatio);

		QRect target(QPoint(0, 0), sz);
		target.moveCenter(area.center());

		painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
		painter.drawImage(target, image);
	}
	else
	{
		painter.setPen(QPen(fgMix(0.35), 1, Qt::DashLine));
		painter.drawRect(area.adjusted(0, 0, -1, -1));
		painter.setPen(fgMix(0.5));
		painter.drawText(area, Qt::AlignCenter, exists ? tr("No Preview") : tr("Empty"));
	}

	QFont f = font();
	f.setBold(selected);
	painter.setFont(f);
	painter.setPen(selected ? fg : fgMix(0.6));
	painter.drawText(captionRect, Qt::AlignCenter, tr("Slot %1").arg(slotId));
}
//----------------------------------------------------------------------------
void StateSlotThumb_t::mousePressEvent(QMouseEvent *event)
{
	if (valid && (event->button() == Qt::LeftButton))
	{
		emit selectRequested(slotId);
	}
	QWidget::mousePressEvent(event);
}
//----------------------------------------------------------------------------
void StateSlotThumb_t::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (valid && exists && !FCEU_HardcoreActive() && (event->button() == Qt::LeftButton))
	{
		emit loadRequested(slotId);
	}
	QWidget::mouseDoubleClickEvent(event);
}
//----------------------------------------------------------------------------
void StateSlotThumb_t::contextMenuEvent(QContextMenuEvent *event)
{
	if (!valid)
	{
		return;
	}
	QMenu menu(this);
	QAction *selAct  = menu.addAction(tr("&Select Slot"));
	QAction *loadAct = menu.addAction(tr("&Load State"));
	QAction *saveAct = menu.addAction(tr("S&ave State"));

	loadAct->setEnabled(exists && !FCEU_HardcoreActive());

	QAction *act = menu.exec(event->globalPos());

	if (act == selAct)
	{
		emit selectRequested(slotId);
	}
	else if (act == loadAct)
	{
		emit loadRequested(slotId);
	}
	else if (act == saveAct)
	{
		emit saveRequested(slotId);
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
StateSlotDock_t::StateSlotDock_t(QWidget *parent)
	: QDockWidget(tr("State Slots"), parent)
{
	setObjectName("StateSlotDock");
	setAllowedAreas(Qt::AllDockWidgetAreas);

	container = new StateSlotContainer_t(this);
	container->setObjectName("stateSlotContents");
	container->setAutoFillBackground(true);

	thumbLayout = new QBoxLayout(QBoxLayout::LeftToRight, container);
	thumbLayout->setContentsMargins(2, 2, 2, 2);
	thumbLayout->setSpacing(4);

	for (int i = 0; i < MAX_THUMBS; i++)
	{
		StateSlotThumb_t *thumb = new StateSlotThumb_t(container);

		connect(thumb, &StateSlotThumb_t::selectRequested, this, &StateSlotDock_t::selectRequested);
		connect(thumb, &StateSlotThumb_t::loadRequested, this, &StateSlotDock_t::loadRequested);
		connect(thumb, &StateSlotThumb_t::saveRequested, this, &StateSlotDock_t::saveRequested);

		thumb->setVisible(i < visibleCount);
		thumbLayout->addWidget(thumb, 1);
		thumbs.push_back(thumb);
	}
	setWidget(container);
	applyBgColor(bgColor);

	connect(this, &QDockWidget::dockLocationChanged, this, &StateSlotDock_t::dockAreaChanged);

	pollTimer = new QTimer(this);
	connect(pollTimer, &QTimer::timeout, this, &StateSlotDock_t::refresh);
	pollTimer->start(100);
}
//----------------------------------------------------------------------------
StateSlotDock_t::~StateSlotDock_t(void)
{
}
//----------------------------------------------------------------------------
void StateSlotDock_t::showEvent(QShowEvent *event)
{
	QDockWidget::showEvent(event);
	refresh();
}
//----------------------------------------------------------------------------
void StateSlotDock_t::resizeEvent(QResizeEvent *event)
{
	QDockWidget::resizeEvent(event);
	updateVisibleCount();
}
//----------------------------------------------------------------------------
void StateSlotDock_t::dockAreaChanged(Qt::DockWidgetArea area)
{
	if ((area == Qt::LeftDockWidgetArea) || (area == Qt::RightDockWidgetArea))
	{
		thumbLayout->setDirection(QBoxLayout::TopToBottom);
		container->horizontal = false;
	}
	else if (area != Qt::NoDockWidgetArea)
	{
		thumbLayout->setDirection(QBoxLayout::LeftToRight);
		container->horizontal = true;
	}
	container->updateGeometry();
	updateVisibleCount();
}
//----------------------------------------------------------------------------
void StateSlotDock_t::applyBgColor(const QColor &c)
{
	bgColor = c;

	QPalette pal = container->palette();
	pal.setColor(QPalette::Window, c);
	container->setPalette(pal);
	// A local background rule takes precedence over application-wide QWidget rules.
	container->setStyleSheet(QString("QWidget#stateSlotContents { background: %1; }").arg(c.name()));

	for (StateSlotThumb_t *thumb : thumbs)
	{
		thumb->bgColor = c;
		thumb->update();
	}
}
//----------------------------------------------------------------------------
void StateSlotDock_t::updateVisibleCount(void)
{
	// Thumbnails are sized by the cross axis of the dock (height when docked at the
	// top or bottom), so the number that fit along the main axis follows from that.
	const bool horizontal = (thumbLayout->direction() == QBoxLayout::LeftToRight);
	const QMargins m = thumbLayout->contentsMargins();
	const int spacing = thumbLayout->spacing();
	const int captionH = QFontMetrics(font()).height() + 4;
	const int chrome = 2 * THUMB_MARGIN;
	const double aspect = double(NES_SCREEN_WIDTH) / 224.0;
	const QSize avail = widget()->size();

	int count = 3;

	if (horizontal)
	{
		const int imgH = avail.height() - m.top() - m.bottom() - chrome - captionH;
		const int cellW = int(imgH * aspect) + chrome;

		if ((imgH > 0) && (cellW > 0))
		{
			count = (avail.width() - m.left() - m.right() + spacing) / (cellW + spacing);
		}
	}
	else
	{
		const int imgW = avail.width() - m.left() - m.right() - chrome;
		const int cellH = int(imgW / aspect) + chrome + captionH;

		if ((imgW > 0) && (cellH > 0))
		{
			count = (avail.height() - m.top() - m.bottom() + spacing) / (cellH + spacing);
		}
	}

	// Always an odd count so that the selected slot sits in the middle
	count = std::min(std::max(count, 3), int(MAX_THUMBS));
	if ((count % 2) == 0)
	{
		count--;
	}

	if (count == visibleCount)
	{
		return;
	}
	visibleCount = count;

	for (int i = 0; i < MAX_THUMBS; i++)
	{
		thumbs[i]->setVisible(i < visibleCount);
	}
	refresh();
}
//----------------------------------------------------------------------------
bool StateSlotDock_t::loadThumbImage(StateSlotThumb_t *thumb, const std::string &path)
{
	std::vector<uint8_t> raw(256 * 256);
	uint32_t palette[256];
	int firstLine = 0, lastLine = NES_SCREEN_HEIGHT - 1;

	thumb->image = QImage();
	thumb->hasImage = false;

	if (!readStateScreenshot(path, raw.data()))
	{
		return false;
	}

	{
		FCEU_WRAPPER_SCOPED_LOCK();

		for (int i = 0; i < 256; i++)
		{
			uint8 r, g, b;
			FCEUD_GetPalette(i, &r, &g, &b);
			palette[i] = 0xFF000000u | (uint32_t(r) << 16) | (uint32_t(g) << 8) | b;
		}
		firstLine = FSettings.FirstSLine;
		lastLine  = FSettings.LastSLine;
	}

	if ((firstLine < 0) || (lastLine >= NES_SCREEN_HEIGHT) || (lastLine < firstLine))
	{
		firstLine = 0;
		lastLine  = NES_SCREEN_HEIGHT - 1;
	}
	const int rows = lastLine - firstLine + 1;

	QImage img(NES_SCREEN_WIDTH, rows, QImage::Format_RGB32);

	for (int y = 0; y < rows; y++)
	{
		uint32_t *dst = reinterpret_cast<uint32_t*>(img.scanLine(y));
		const uint8_t *src = &raw[(firstLine + y) * 256];

		for (int x = 0; x < NES_SCREEN_WIDTH; x++)
		{
			dst[x] = palette[src[x]];
		}
	}
	thumb->image = img;
	thumb->hasImage = true;

	return true;
}
//----------------------------------------------------------------------------
void StateSlotDock_t::refresh(void)
{
	if (!isVisible())
	{
		return;
	}

	FCEU_PROFILE_FUNC("State slot dock refresh");

	int bank = 0, current = 0;
	bool gameLoaded = false;
	std::string paths[MAX_THUMBS];
	int slotIds[MAX_THUMBS];
	const int count = visibleCount;
	QColor videoBg = bgColor;

	{
		FCEU_WRAPPER_SCOPED_LOCK();

		if (consoleWindow != nullptr)
		{
			videoBg = *consoleWindow->getVideoBgColorPtr();
		}
		gameLoaded = (GameInfo != nullptr);
		bank       = FCEUI_GetStateBank();
		current    = CurrentState;

		// Slot selection steps through 1,2,...,9,0 (or 0,1,...,9 when zero based)
		// and then continues in the next bank, so neighbors are laid out along
		// that sequence.
		const int offset = zeroBasedStateSlots ? 0 : 9;
		const int position = (bank * 10) + ((current + offset) % 10);

		for (int i = 0; i < count; i++)
		{
			const int p = position + i - (count / 2);

			if (!gameLoaded || (p < 0) || (p >= MAX_SAVE_STATE_SLOTS))
			{
				slotIds[i] = -1;
				continue;
			}
			slotIds[i] = ((p / 10) * 10) + (((p % 10) + (zeroBasedStateSlots ? 0 : 1)) % 10);
			paths[i] = FCEU_MakeFName(FCEUMKF_STATE, slotIds[i], 0);
		}
	}

	if (videoBg != bgColor)
	{
		applyBgColor(videoBg);
	}

	for (int i = 0; i < count; i++)
	{
		StateSlotThumb_t *thumb = thumbs[i];
		const bool wasExists = thumb->exists;
		const bool wasSelected = thumb->selected;
		const bool wasValid = thumb->valid;
		const int  wasSlotId = thumb->slotId;
		const bool hadImage = thumb->hasImage;

		thumb->valid = (slotIds[i] >= 0);

		if (!thumb->valid)
		{
			thumb->exists = false;
			thumb->hasImage = false;
			thumb->image = QImage();
			thumb->path.clear();
			thumb->fileSize = thumb->fileTime = -1;
			thumb->selected = false;
			thumb->slotId = -1;
		}
		else
		{
			thumb->slotId = slotIds[i];
			thumb->selected = (i == count / 2);

			const QFileInfo info(QString::fromStdString(paths[i]));

			if (!info.exists())
			{
				thumb->exists = false;
				thumb->hasImage = false;
				thumb->image = QImage();
				thumb->path.clear();
				thumb->fileSize = thumb->fileTime = -1;
			}
			else
			{
				const qint64 mtime = info.lastModified().toMSecsSinceEpoch();
				const qint64 size  = info.size();

				thumb->exists = true;

				if ((thumb->path != paths[i]) || (thumb->fileTime != mtime) || (thumb->fileSize != size))
				{
					const bool ok = loadThumbImage(thumb, paths[i]);
					const bool recentlyWritten = (QDateTime::currentMSecsSinceEpoch() - mtime) < RETRY_WINDOW_MS;

					if (ok || !recentlyWritten)
					{
						thumb->path = paths[i];
						thumb->fileTime = mtime;
						thumb->fileSize = size;
					}
					else
					{
						thumb->path.clear();
					}
					thumb->update();
				}
			}
		}

		if ((wasValid != thumb->valid) || (wasExists != thumb->exists) || (wasSelected != thumb->selected) ||
		    (wasSlotId != thumb->slotId) || (hadImage != thumb->hasImage))
		{
			thumb->update();
		}
	}
}
