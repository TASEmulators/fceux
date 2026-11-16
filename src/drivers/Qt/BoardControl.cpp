// BoardControl.cpp
//
#include <QCloseEvent>
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
	dipChecks.clear();

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
	QGridLayout *grid;

	FCEU_WRAPPER_SCOPED_LOCK();

	const std::vector<BoardDipSwitch> &switches = FCEU_BoardGetDipSwitches();

	if (switches.empty())
	{
		return;
	}

	dipBox = new QGroupBox(tr("DIP Switches"));
	grid = new QGridLayout();
	dipBox->setLayout(grid);

	for (size_t i = 0; i < switches.size(); i++)
	{
		QCheckBox *chk = new QCheckBox(QString::fromStdString(switches[i].name));

		chk->setChecked(switches[i].get());
		connect(chk, &QCheckBox::clicked, this, [this, i](bool checked) { toggleDipSwitch(i, checked); });

		grid->addWidget(chk, static_cast<int>(i), 0);
		dipChecks.append(chk);
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

	FCEU_WRAPPER_SCOPED_LOCK();

	const std::vector<BoardDipSwitch> &switches = FCEU_BoardGetDipSwitches();

	if (idx < switches.size())
	{
		switches[idx].set(checked);
	}
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
	if (dipChecks.isEmpty())
	{
		return;
	}

	FCEU_WRAPPER_SCOPED_LOCK();

	const std::vector<BoardDipSwitch> &switches = FCEU_BoardGetDipSwitches();

	for (int i = 0; i < dipChecks.size() && static_cast<size_t>(i) < switches.size(); i++)
	{
		dipChecks[i]->setChecked(switches[i].get());
	}
}
//----------------------------------------------------
