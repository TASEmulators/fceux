// BoardControl.cpp
//
#include <algorithm>
#include <map>

#include <QCloseEvent>
#include <QPainter>
#include <QMouseEvent>
#include <QHelpEvent>
#include <QToolTip>
#include <QGridLayout>
#include <QPushButton>
#include <QHBoxLayout>

#include "boardctl.h"
#include "driver.h"
#include "Qt/main.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/BoardControl.h"

static BoardControlDialog_t *boardControlWin = nullptr;

//----------------------------------------------------
int openBoardControlWindow(QWidget *parent)
{
	if (boardControlWin != nullptr)
	{
		boardControlWin->show();
		boardControlWin->raise();
		boardControlWin->activateWindow();
		return -1;
	}
	boardControlWin = new BoardControlDialog_t(parent);

	boardControlWin->show();

	return 0;
}
//----------------------------------------------------
int closeBoardControlWindow(void)
{
	if (boardControlWin != nullptr)
	{
		boardControlWin->close();
	}
	return 0;
}
//----------------------------------------------------
// DipSwitchBank_t
//----------------------------------------------------
static const int dipMargin = 10;
static const int dipSlotW = 24;
static const int dipSlotH = 52;
static const int dipGap = 6;

DipSwitchBank_t::DipSwitchBank_t(const QVector<Item> &itemList, QWidget *parent)
	: QWidget(parent), items(itemList)
{
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	setCursor(Qt::PointingHandCursor);
	setMouseTracking(true);
}
//----------------------------------------------------
int DipSwitchBank_t::labelHeight(void) const
{
	return fontMetrics().height() + 4;
}
//----------------------------------------------------
QSize DipSwitchBank_t::sizeHint(void) const
{
	int n = items.size();
	int w = 2 * dipMargin + n * dipSlotW + (n > 0 ? (n - 1) * dipGap : 0);
	int h = 2 * dipMargin + 2 * labelHeight() + dipSlotH;

	return QSize(w, h);
}
//----------------------------------------------------
QSize DipSwitchBank_t::minimumSizeHint(void) const
{
	return sizeHint();
}
//----------------------------------------------------
QRect DipSwitchBank_t::switchRect(int index) const
{
	return QRect(dipMargin + index * (dipSlotW + dipGap), dipMargin + labelHeight(), dipSlotW, dipSlotH);
}
//----------------------------------------------------
int DipSwitchBank_t::switchAt(const QPoint &pos) const
{
	for (int i = 0; i < items.size(); i++)
	{
		// Include the gap and label areas so the whole column is clickable
		QRect r = switchRect(i);
		QRect column(r.left() - dipGap / 2, 0, r.width() + dipGap, height());

		if (column.contains(pos))
		{
			return i;
		}
	}
	return -1;
}
//----------------------------------------------------
void DipSwitchBank_t::setStates(const QVector<bool> &states)
{
	bool changed = false;

	for (int i = 0; i < items.size() && i < states.size(); i++)
	{
		if (items[i].on != states[i])
		{
			items[i].on = states[i];
			changed = true;
		}
	}
	if (changed)
	{
		update();
	}
}
//----------------------------------------------------
void DipSwitchBank_t::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	const QColor bodyColor(0x1c, 0x4f, 0xa8);
	const QColor slotColor(0x0b, 0x1f, 0x45);
	const QColor knobOn(0xf2, 0xf2, 0xf2);
	const QColor knobOff(0xc8, 0xc8, 0xc8);

	p.setPen(QPen(bodyColor.darker(160), 1));
	p.setBrush(bodyColor);
	p.drawRoundedRect(QRectF(0.5, 0.5, width() - 1, height() - 1), 4, 4);

	p.setPen(Qt::white);
	p.drawText(QRect(dipMargin, dipMargin / 2, width() - 2 * dipMargin, labelHeight()),
	           Qt::AlignLeft | Qt::AlignVCenter, tr("ON"));

	for (int i = 0; i < items.size(); i++)
	{
		QRect slot = switchRect(i);
		int half = slot.height() / 2;
		int pad = 3;
		QRect knob(slot.left() + pad, items[i].on ? slot.top() + pad : slot.top() + half,
		           slot.width() - 2 * pad, half - pad);

		p.setPen(QPen(Qt::black, 1));
		p.setBrush(slotColor);
		p.drawRoundedRect(slot, 2, 2);

		p.setBrush(items[i].on ? knobOn : knobOff);
		p.setPen(QPen(QColor(0x60, 0x60, 0x60), 1));
		p.drawRoundedRect(knob, 2, 2);

		// Grip line on the knob
		p.setPen(QPen(QColor(0x90, 0x90, 0x90), 1));
		p.drawLine(knob.left() + 4, knob.center().y(), knob.right() - 4, knob.center().y());

		p.setPen(Qt::white);
		p.drawText(QRect(slot.left() - dipGap / 2, slot.bottom() + 2, slot.width() + dipGap, labelHeight()),
		           Qt::AlignHCenter | Qt::AlignVCenter, items[i].label);
	}
}
//----------------------------------------------------
void DipSwitchBank_t::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		int idx = switchAt(event->pos());

		if (idx >= 0)
		{
			emit switchClicked(idx, !items[idx].on);
			return;
		}
	}
	QWidget::mousePressEvent(event);
}
//----------------------------------------------------
bool DipSwitchBank_t::event(QEvent *e)
{
	if (e->type() == QEvent::ToolTip)
	{
		QHelpEvent *he = static_cast<QHelpEvent*>(e);
		int idx = switchAt(he->pos());

		if (idx >= 0)
		{
			QToolTip::showText(he->globalPos(), items[idx].tooltip, this);
		}
		else
		{
			QToolTip::hideText();
		}
		return true;
	}
	return QWidget::event(e);
}
//----------------------------------------------------
BoardControlDialog_t::BoardControlDialog_t(QWidget *parent)
	: QDialog(parent), dipBox(nullptr)
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QPushButton *closeButton;

	boardControlWin = this;

	setWindowTitle(tr("Board Control"));

	mainLayout = new QVBoxLayout();
	sectionLayout = new QVBoxLayout();

	emptyLabel = new QLabel(tr("The loaded board has no controls available."));
	emptyLabel->setAlignment(Qt::AlignCenter);
	sectionLayout->addWidget(emptyLabel);

	closeButton = new QPushButton(tr("Close"));
	closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	connect(closeButton, SIGNAL(clicked(void)), this, SLOT(close(void)));

	hbox = new QHBoxLayout();
	hbox->addStretch(1);
	hbox->addWidget(closeButton);

	mainLayout->addLayout(sectionLayout);
	mainLayout->addStretch(1);
	mainLayout->addLayout(hbox);

	setLayout(mainLayout);

	updateTimer = new QTimer(this);
	connect(updateTimer, SIGNAL(timeout(void)), this, SLOT(refresh(void)));
	updateTimer->start(250);

	if (consoleWindow)
	{
		connect(consoleWindow, SIGNAL(romLoaded(void)), this, SLOT(rebuild(void)));
		connect(consoleWindow, SIGNAL(romUnload(void)), this, SLOT(onRomUnload(void)));
	}

	rebuild();
}
//----------------------------------------------------
BoardControlDialog_t::~BoardControlDialog_t(void)
{
	updateTimer->stop();
	boardControlWin = nullptr;
}
//----------------------------------------------------
void BoardControlDialog_t::closeEvent(QCloseEvent *event)
{
	done(result());
	deleteLater();
	event->accept();
}
//----------------------------------------------------
void BoardControlDialog_t::clearSections(void)
{
	// Widgets are dropped before the board registry is cleared on ROM close,
	// so no widget outlives the state it refers to.
	dipBanks.clear();

	if (dipBox != nullptr)
	{
		sectionLayout->removeWidget(dipBox);
		delete dipBox;
		dipBox = nullptr;
	}
}
//----------------------------------------------------
void BoardControlDialog_t::buildDipSwitchSection(void)
{
	QVBoxLayout *vbox;

	FCEU_WRAPPER_SCOPED_LOCK();

	const std::vector<BoardDipSwitch> &switches = FCEU_BoardGetDipSwitches();

	if (switches.empty())
	{
		return;
	}

	// One bank per backing byte, in registration order
	std::vector<uint8_t*> addrs;
	std::map<uint8_t*, QVector<size_t>> groups;

	for (size_t i = 0; i < switches.size(); i++)
	{
		if (groups.find(switches[i].addr) == groups.end())
		{
			addrs.push_back(switches[i].addr);
		}
		groups[switches[i].addr].append(i);
	}

	dipBox = new QGroupBox(tr("DIP Switches"));
	vbox = new QVBoxLayout();
	dipBox->setLayout(vbox);

	for (uint8_t *addr : addrs)
	{
		QVector<size_t> &indices = groups[addr];
		QVector<DipSwitchBank_t::Item> items;

		// Match the VS. System display: least significant bit on the left.
		std::stable_sort(indices.begin(), indices.end(),
		                 [&switches](size_t a, size_t b) { return switches[a].bit < switches[b].bit; });

		for (size_t idx : indices)
		{
			DipSwitchBank_t::Item item;

			item.label = QString::number(switches[idx].bit + 1);
			item.tooltip = QString::fromStdString(switches[idx].name);
			item.on = switches[idx].get();
			items.append(item);
		}

		DipSwitchBank_t *bank = new DipSwitchBank_t(items);

		connect(bank, &DipSwitchBank_t::switchClicked, this,
		        [this, indices](int pos, bool checked) { toggleDipSwitch(indices[pos], checked); });

		vbox->addWidget(bank, 0, Qt::AlignHCenter);
		dipBanks.append({ bank, indices });
	}
	sectionLayout->addWidget(dipBox);
}
//----------------------------------------------------
void BoardControlDialog_t::toggleDipSwitch(size_t idx, bool checked)
{
	if (FCEU_HardcoreActive())
	{
		return;
	}

	{
		FCEU_WRAPPER_SCOPED_LOCK();

		const std::vector<BoardDipSwitch> &switches = FCEU_BoardGetDipSwitches();

		if (idx < switches.size())
		{
			switches[idx].set(checked);
		}
	}
	refreshDipStates();
}
//----------------------------------------------------
void BoardControlDialog_t::rebuild(void)
{
	clearSections();

	buildDipSwitchSection();

	emptyLabel->setVisible(dipBox == nullptr);
}
//----------------------------------------------------
void BoardControlDialog_t::onRomUnload(void)
{
	clearSections();

	emptyLabel->setVisible(true);
}
//----------------------------------------------------
void BoardControlDialog_t::refresh(void)
{
	refreshDipStates();
}
//----------------------------------------------------
void BoardControlDialog_t::refreshDipStates(void)
{
	if (dipBanks.isEmpty())
	{
		return;
	}

	FCEU_WRAPPER_SCOPED_LOCK();

	const std::vector<BoardDipSwitch> &switches = FCEU_BoardGetDipSwitches();

	for (const DipBankInfo &info : dipBanks)
	{
		QVector<bool> states;

		for (size_t idx : info.indices)
		{
			states.append(idx < switches.size() ? switches[idx].get() : false);
		}
		info.widget->setStates(states);
	}
}
//----------------------------------------------------
