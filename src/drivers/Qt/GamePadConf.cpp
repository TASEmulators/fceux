/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020 mjbudd77
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
// GamePadConf.cpp
//
#include <QDir>
#include <QInputDialog>
#include <QMessageBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QPainter>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QHeaderView>

#include "Qt/GamePadConf.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/sdl-joystick.h"
#include "Qt/fceuWrapper.h"

struct GamePadConfigLocalData_t
{
	std::string guid;
	std::string profile;
	bool advBindNeedsSave;

	struct
	{

		char needsSave;

	} btn[GAMEPAD_NUM_BUTTONS];

	GamePadConfigLocalData_t(void)
	{
		for (int i = 0; i < GAMEPAD_NUM_BUTTONS; i++)
		{
			btn[i].needsSave = 0;
		}
		advBindNeedsSave = 0;
	}

};

static GamePadConfigLocalData_t lcl[GAMEPAD_NUM_DEVICES];

static GamePadConfDialog_t *gamePadConfWin = NULL;
static bool updateRemapTree = false;

//----------------------------------------------------
int openGamePadConfWindow(QWidget *parent)
{
	if (gamePadConfWin != NULL)
	{
		return -1;
	}
	gamePadConfWin = new GamePadConfDialog_t(parent);

	gamePadConfWin->show();

	return 0;
}
//----------------------------------------------------
int closeGamePadConfWindow(void)
{
	if (gamePadConfWin != NULL)
	{
		gamePadConfWin->closeWindow();
	}
	return 0;
}
//----------------------------------------------------
GamePadConfDialog_t::GamePadConfDialog_t(QWidget *parent)
	: QDialog(parent), changeSeqStatus(0)
{
	//QWidget *mainWidget;
	QVBoxLayout *mainLayoutV;
	QHBoxLayout *mainLayoutH;
	QHBoxLayout *hbox, *hbox1, *hbox2;
	QVBoxLayout *vbox, *vbox1, *vbox2;
	QGridLayout *grid, *grid1;
	QCheckBox *udlr_chkbox;
	QGroupBox *frame1, *frame2;
	QLabel *label;
	QPushButton *newProfileButton;
	QPushButton *saveProfileButton;
	QPushButton *applyProfileButton;
	QPushButton *removeProfileButton;
	QPushButton *clearAllButton;
	QPushButton *closebutton;
	QPushButton *changeSeqButton = nullptr;
	QPushButton *clearButton[GAMEPAD_NUM_BUTTONS];
	QScrollArea *scroll = NULL;
	QStyle *style;
	std::string prefix;
	char stmp[256];
	bool useScroll = false;
	int useNativeMenuBar;
	QMenuBar *menuBar;
	QMenu *fileMenu, *extMenu;
	QAction *act;
	QTreeWidgetItem *item;
	const char *guid;

	style = this->style();

	gamePadConfWin = this;

	// Ensure that joysticks are enabled, no harm calling init again.
	InitJoysticks();

	if (useScroll)
	{
		scroll = new QScrollArea(this);
	}
	mainWidget = new QWidget();

	portNum = 0;
	configIndex = 0;
	buttonConfigStatus = 1;

	inputTimer = new QTimer(this);

	connect(inputTimer, &QTimer::timeout, this, &GamePadConfDialog_t::updatePeriodic);

	setWindowTitle(tr("GamePad Config"));

	menuBar = new QMenuBar(this);

	// This is needed for menu bar to show up on MacOS
	g_config->getOption( "SDL.UseNativeMenuBar", &useNativeMenuBar );

	menuBar->setNativeMenuBar( useNativeMenuBar ? true : false );

	//-----------------------------------------------------------------------
	// Menu Start
	//-----------------------------------------------------------------------
	// File
	fileMenu = menuBar->addMenu(tr("&File"));

	// File -> Close
	act = new QAction(tr("&Close"), this);
	act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Close Window"));
	connect(act, SIGNAL(triggered()), this, SLOT(closeWindow(void)) );
	
	fileMenu->addAction(act);

	// Extensions
	extMenu = menuBar->addMenu(tr("&Extensions"));

	// Extensions -> Show Adv Bindings
	act = new QAction(tr("&Show Adv Bindings"), this);
	//act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Show Adv Bindings"));
	act->setCheckable(true);
	connect(act, SIGNAL(triggered(bool)), this, SLOT(advBindingViewChanged(bool)) );
	
	extMenu->addAction(act);

	//-----------------------------------------------------------------------
	// Menu End
	//-----------------------------------------------------------------------

	grid1 = new QGridLayout();

	grid1->setColumnStretch(0, 1);
	grid1->setColumnStretch(1, 10);
	hbox1 = new QHBoxLayout();
	hbox2 = new QHBoxLayout();

	label = new QLabel(tr("Console Port:"));
	portSel = new QComboBox();
	grid1->addWidget(label, 0, 0);
	grid1->addWidget(portSel, 0, 1);

	portSel->addItem(tr("1"), 0);
	portSel->addItem(tr("2"), 1);
	portSel->addItem(tr("3"), 2);
	portSel->addItem(tr("4"), 3);

	label = new QLabel(tr("Device:"));
	devSel = new QComboBox();
	grid1->addWidget(label, 1, 0);
	grid1->addWidget(devSel, 1, 1);

	devSel->addItem(tr("Keyboard"), -1);

	for (int i = 0; i < MAX_JOYSTICKS; i++)
	{
		jsDev_t *js = getJoystickDevice(i);

		if (js != NULL)
		{
			if (js->isConnected())
			{
				sprintf(stmp, "%i: %s", i, js->getName());
				devSel->addItem(tr(stmp), i);
			}
		}
	}
	for (int i = 0; i < devSel->count(); i++)
	{
		if (devSel->itemData(i).toInt() == GamePad[portNum].getDeviceIndex())
		{
			devSel->setCurrentIndex(i);
		}
	}

	label = new QLabel(tr("GUID:"));
	guidLbl = new QLabel();

	grid1->addWidget(label, 2, 0);
	grid1->addWidget(guidLbl, 2, 1);

	guid = GamePad[portNum].getGUID();
	if ( guid )
	{
		guidLbl->setText(guid);
	}

	frame1 = new QGroupBox(tr("Mapping Profile:"));
	//grid   = new QGridLayout();
	vbox = new QVBoxLayout();

	//frame1->setLayout( grid );
	frame1->setLayout(vbox);

	hbox = new QHBoxLayout();
	vbox->addLayout(hbox);

	mapSel = new QComboBox();
	hbox->addWidget(mapSel);

	mapSel->setWhatsThis(tr("Combo box for selection of a saved button mapping profile for the selected device"));
	mapSel->addItem(tr("default"), 0);

	hbox = new QHBoxLayout();
	vbox->addLayout(hbox);

	applyProfileButton = new QPushButton(tr("Load"));
	applyProfileButton->setWhatsThis(tr("Sets Current Active Map to the Selected Profile"));
	applyProfileButton->setIcon(style->standardIcon(QStyle::SP_DialogApplyButton));
	hbox->addWidget(applyProfileButton);

	saveProfileButton = new QPushButton(tr("Save"));
	saveProfileButton->setWhatsThis(tr("Stores Current Active Map to the Selected Profile"));
	saveProfileButton->setIcon(style->standardIcon(QStyle::SP_DialogSaveButton));
	hbox->addWidget(saveProfileButton);

	hbox = new QHBoxLayout();
	vbox->addLayout(hbox);

	newProfileButton = new QPushButton(tr("New"));
	newProfileButton->setWhatsThis(tr("Create a New Map Profile"));
	newProfileButton->setIcon(style->standardIcon(QStyle::SP_FileIcon));
	hbox->addWidget(newProfileButton);

	removeProfileButton = new QPushButton(tr("Delete"));
	removeProfileButton->setWhatsThis(tr("Deletes the Selected Map Profile"));
	removeProfileButton->setIcon(style->standardIcon(QStyle::SP_TrashIcon));
	hbox->addWidget(removeProfileButton);

	mapMsg = new QLabel();
	vbox->addWidget(mapMsg);

	efs_chkbox = new QCheckBox(tr("Enable Four Score"));
	udlr_chkbox = new QCheckBox(tr("Allow Up+Down/Left+Right"));

	int fourScore;
	g_config->getOption("SDL.FourScore", &fourScore);
	efs_chkbox->setChecked(fourScore);

	int opposite_dirs;
	g_config->getOption("SDL.Input.EnableOppositeDirectionals", &opposite_dirs);
	udlr_chkbox->setChecked(opposite_dirs);

	confTabBar = new QTabBar();
	frame2 = new QGroupBox(tr("Current Active Button Mappings:"));
	grid = new QGridLayout();

	confTabBar->addTab( tr("Pri") );
	confTabBar->addTab( tr("Alt 1") );
	confTabBar->addTab( tr("Alt 2") );
	confTabBar->addTab( tr("Alt 3") );

	confTabBar->setTabToolTip( 0, tr("Primary Button Map") );
	confTabBar->setTabToolTip( 1, tr("Alternate Button Map #1") );
	confTabBar->setTabToolTip( 2, tr("Alternate Button Map #2") );
	confTabBar->setTabToolTip( 3, tr("Alternate Button Map #3") );

	confTabBar->setCurrentIndex(0);

	connect( confTabBar, SIGNAL(currentChanged(int)), this, SLOT(btnConfigChanged(int)) );

	//grid-> setHorizontalSpacing(10);

	frame2->setLayout(grid);

	for (int i = 0; i < GAMEPAD_NUM_BUTTONS; i++)
	{
		char text[64];
		QLabel *buttonName;

		sprintf(text, "%s:", GamePadNames[i]);

		//hbox2 = new QHBoxLayout();

		//hbox2->setAlignment(Qt::AlignCenter);

		buttonName = new QLabel(tr(text));
		keyName[i] = new QLabel();
		//keyState[i]    = new QLabel( tr("F") );
		//label          = new QLabel( tr("State:") );
		button[i] = new GamePadConfigButton_t(i);
		clearButton[i] = new QPushButton(tr("Clear"));

		grid->addWidget(buttonName, i, 0, Qt::AlignCenter);
		grid->addWidget(keyName[i], i, 1, Qt::AlignCenter);
		//grid->addWidget( label         , i, 2, Qt::AlignCenter );
		//grid->addWidget( keyState[i]   , i, 3, Qt::AlignCenter );
		grid->addWidget(button[i], i, 2, Qt::AlignCenter);
		grid->addWidget(clearButton[i], i, 3, Qt::AlignCenter);
	}
	updateCntrlrDpy();

	clearAllButton = new QPushButton(tr("Clear All"));
	closebutton = new QPushButton(tr("Close"));
	changeSeqButton = new QPushButton(tr("Change Sequentially"));

	clearAllButton->setIcon(style->standardIcon(QStyle::SP_LineEditClearButton));
	closebutton->setIcon(style->standardIcon(QStyle::SP_DialogCloseButton));
	changeSeqButton->setIcon(style->standardIcon(QStyle::QStyle::SP_ArrowDown));

	hbox1->addWidget(clearAllButton);
	hbox1->addWidget(changeSeqButton);
	hbox1->addWidget(closebutton);

	connect(button[0], SIGNAL(clicked()), this, SLOT(changeButton0(void)));
	connect(button[1], SIGNAL(clicked()), this, SLOT(changeButton1(void)));
	connect(button[2], SIGNAL(clicked()), this, SLOT(changeButton2(void)));
	connect(button[3], SIGNAL(clicked()), this, SLOT(changeButton3(void)));
	connect(button[4], SIGNAL(clicked()), this, SLOT(changeButton4(void)));
	connect(button[5], SIGNAL(clicked()), this, SLOT(changeButton5(void)));
	connect(button[6], SIGNAL(clicked()), this, SLOT(changeButton6(void)));
	connect(button[7], SIGNAL(clicked()), this, SLOT(changeButton7(void)));
	connect(button[8], SIGNAL(clicked()), this, SLOT(changeButton8(void)));
	connect(button[9], SIGNAL(clicked()), this, SLOT(changeButton9(void)));

	connect(clearButton[0], SIGNAL(clicked()), this, SLOT(clearButton0(void)));
	connect(clearButton[1], SIGNAL(clicked()), this, SLOT(clearButton1(void)));
	connect(clearButton[2], SIGNAL(clicked()), this, SLOT(clearButton2(void)));
	connect(clearButton[3], SIGNAL(clicked()), this, SLOT(clearButton3(void)));
	connect(clearButton[4], SIGNAL(clicked()), this, SLOT(clearButton4(void)));
	connect(clearButton[5], SIGNAL(clicked()), this, SLOT(clearButton5(void)));
	connect(clearButton[6], SIGNAL(clicked()), this, SLOT(clearButton6(void)));
	connect(clearButton[7], SIGNAL(clicked()), this, SLOT(clearButton7(void)));
	connect(clearButton[8], SIGNAL(clicked()), this, SLOT(clearButton8(void)));
	connect(clearButton[9], SIGNAL(clicked()), this, SLOT(clearButton9(void)));

	connect(newProfileButton, SIGNAL(clicked()), this, SLOT(newProfileCallback(void)));
	connect(applyProfileButton, SIGNAL(clicked()), this, SLOT(loadProfileCallback(void)));
	connect(saveProfileButton, SIGNAL(clicked()), this, SLOT(saveProfileCallback(void)));
	connect(removeProfileButton, SIGNAL(clicked()), this, SLOT(deleteProfileCallback(void)));

	connect(clearAllButton, SIGNAL(clicked()), this, SLOT(clearAllCallback(void)));
	connect(closebutton, SIGNAL(clicked()), this, SLOT(closeWindow(void)));
	connect(changeSeqButton, SIGNAL(clicked()), this, SLOT(changeSequentallyCallback(void)));

	connect(portSel, SIGNAL(activated(int)), this, SLOT(portSelect(int)));
	connect(devSel, SIGNAL(activated(int)), this, SLOT(deviceSelect(int)));
	connect(efs_chkbox, SIGNAL(stateChanged(int)), this, SLOT(ena4score(int)));
	connect(udlr_chkbox, SIGNAL(stateChanged(int)), this, SLOT(oppDirEna(int)));

	mainLayoutH = new QHBoxLayout();
	mainLayoutV = new QVBoxLayout();
	vbox1 = new QVBoxLayout();
	vbox2 = new QVBoxLayout();

	hbox2->addWidget(efs_chkbox);
	hbox2->addWidget(udlr_chkbox);

	vbox1->addLayout(grid1);
	//vbox1->addLayout( hbox2 );
	//vbox1->addLayout( hbox3 );
	vbox1->addWidget(frame1);
	vbox1->addLayout(hbox2);

	vbox2->addWidget(confTabBar);
	vbox2->addWidget(frame2);
	vbox2->addLayout(hbox1);

	gpView = new GamePadView_t(this);

	advOptLayout = new QGroupBox( tr("Advanced Key Bindings") );
	advOptLayout->setMaximumWidth(0);

	advOptWidthAnimation = new QPropertyAnimation( advOptLayout, "maximumWidth", this);
	advOptWidthAnimation->setDuration(500);
	advOptWidthAnimation->setStartValue(0);
	advOptWidthAnimation->setEndValue(512);
	advOptWidthAnimation->setEasingCurve( QEasingCurve::InOutCirc );

	connect( advOptWidthAnimation, SIGNAL(valueChanged(const QVariant &)), this, SLOT(advOptWidthChange(const QVariant &)));
	connect( advOptWidthAnimation, SIGNAL(finished(void)), this, SLOT(advOptResizeDone(void)) );

	mainLayoutV->addLayout(vbox1);
	mainLayoutV->addWidget(gpView);

	mainLayoutH->addLayout(mainLayoutV);
	mainLayoutH->addLayout(vbox2);
	mainLayoutH->addWidget(advOptLayout);
	mainLayoutH->setMenuBar( menuBar );

	mainWidget->setLayout(mainLayoutH);
	mainWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	hbox1 = new QHBoxLayout();
	vbox  = new QVBoxLayout();

	advOptLayout->setLayout(hbox1);
	hbox1->addLayout(vbox);

	newKeyBindBtn  = new QPushButton( tr("New") );
	editKeyBindBtn = new QPushButton( tr("Edit") );
	delKeyBindBtn  = new QPushButton( tr("Delete") );

	vbox->addWidget( newKeyBindBtn , 1 );
	vbox->addWidget( editKeyBindBtn, 1 );
	vbox->addWidget( delKeyBindBtn , 1 );
	vbox->addStretch(5);

	keyBindTree = new QTreeWidget();

	keyBindTree->setColumnCount(3);

	item = new QTreeWidgetItem();
	item->setText(0, QString::fromStdString("GP Button"));
	item->setText(1, QString::fromStdString("Press Func"));
	item->setText(2, QString::fromStdString("Release Func"));
	item->setTextAlignment(0, Qt::AlignLeft);
	item->setTextAlignment(1, Qt::AlignLeft);
	item->setTextAlignment(2, Qt::AlignLeft);

	keyBindTree->setHeaderItem(item);

	keyBindTree->header()->setSectionResizeMode(QHeaderView::Interactive);
	keyBindTree->setMinimumWidth( 256 );

	hbox1->addWidget(keyBindTree);

	connect( newKeyBindBtn, SIGNAL(clicked()), this, SLOT(newKeyBindingCallback(void)));
	connect(editKeyBindBtn, SIGNAL(clicked()), this, SLOT(editKeyBindingCallback(void)));
	connect( delKeyBindBtn, SIGNAL(clicked()), this, SLOT(delKeyBindingCallback(void)));

	if (useScroll && (scroll != NULL) )
	{
		scroll->setWidget(mainWidget);
		scroll->setWidgetResizable(true);
		scroll->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
		scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	}

	QHBoxLayout *dialogLayout = new QHBoxLayout();

	if (useScroll && (scroll != NULL) )
	{
		dialogLayout->addWidget(scroll);
	}
	else
	{
		dialogLayout->addWidget(mainWidget);
	}

	setLayout(dialogLayout);

	inputTimer->start(33); // 30hz

	for (int i = 0; i < GAMEPAD_NUM_DEVICES; i++)
	{
		sprintf(stmp, "SDL.Input.GamePad.%i.", i);
		prefix = stmp;

		g_config->getOption(prefix + "Profile", &lcl[i].profile);

		guid = GamePad[i].getGUID();
		if ( guid )
		{
			lcl[i].guid.assign(guid);
		}
	}

	loadMapList();

	if ( updateRemapTree )
	{
		refreshKeyBindTree();
		updateRemapTree = false;
	}
}

//----------------------------------------------------
GamePadConfDialog_t::~GamePadConfDialog_t(void)
{
	inputTimer->stop();
	buttonConfigStatus = 0;
	gamePadConfWin = NULL;

	printf("GamePad Window Deleted\n");
}

void GamePadConfDialog_t::resizeEvent(QResizeEvent *event)
{
	//printf("%ix%i\n", event->size().width(), event->size().height() );
}

void GamePadConfDialog_t::keyPressEvent(QKeyEvent *event)
{
	//printf("GamePad Window Key Press: 0x%x \n", event->key() );
	pushKeyEvent(event, 1);
}

void GamePadConfDialog_t::keyReleaseEvent(QKeyEvent *event)
{
	//printf("GamePad Window Key Release: 0x%x \n", event->key() );
	pushKeyEvent(event, 0);
}
//----------------------------------------------------
void GamePadConfDialog_t::loadMapList(void)
{
	QDir dir;
	QStringList filters, fileList;
	const char *baseDir = FCEUI_GetBaseDirectory();
	const char *guid;
	std::string path;
	std::string prefix, mapName;
	int index, devIdx;
	jsDev_t *js;
	size_t n = 0;
	char stmp[256];

	index = devSel->currentIndex();
	devIdx = devSel->itemData(index).toInt();

	if (devIdx < 0)
	{
		guid = "keyboard";
	}
	else
	{
		js = getJoystickDevice(devIdx);

		guid = js->getGUID();
	}

	if (guid == NULL)
	{
		return;
	}

	path = std::string(baseDir) + "/input/" + std::string(guid);

	dir.setPath(QString::fromStdString(path));

	filters << "*.txt";
	dir.setNameFilters(filters);

	fileList = dir.entryList(filters, QDir::Files, QDir::NoSort);

	sprintf(stmp, "SDL.Input.GamePad.%u.", portNum);
	prefix = stmp;

	g_config->getOption(prefix + "Profile", &mapName);

	mapSel->clear();
	mapSel->addItem(tr("default"), 0);
	n = 1;

	for (size_t i = 0; i < fileList.size(); i++)
	{
		size_t suffixIdx;
		std::string fileName = fileList[i].toStdString();

		suffixIdx = fileName.find_last_of('.');

		fileName.erase(suffixIdx);

		//printf("File: %s \n", fileName.c_str() );
		//

		if (fileName.compare("default") == 0)
			continue;

		mapSel->addItem(tr(fileName.c_str()), (int)i + 1);

		if (mapName.compare(fileName) == 0)
		{
			mapSel->setCurrentIndex(n);
		}
		n++;
	}
}
//----------------------------------------------------
void GamePadConfDialog_t::advBindingViewChanged(bool state)
{
	if ( state )
	{
		advOptWidthAnimation->setStartValue(0);
		advOptWidthAnimation->setEndValue(512);
		advOptWidthAnimation->start();
	}
	else
	{
		advOptWidthAnimation->setStartValue( advOptLayout->width() );
		advOptWidthAnimation->setEndValue(0);
		advOptWidthAnimation->start();
	}
}
//----------------------------------------------------
void GamePadConfDialog_t::advOptWidthChange(const QVariant &value)
{
	resize( minimumSizeHint() );
}
//----------------------------------------------------
void GamePadConfDialog_t::advOptResizeDone(void)
{
	resize( minimumSizeHint() );
}
//----------------------------------------------------
void GamePadConfDialog_t::refreshKeyBindTree( bool reset )
{
	int i;
	QTreeWidgetItem *item;
	std::list <gamepad_function_key_t*>::iterator it;
	gamepad_function_key_t *binding;
	const char *btnName[2];
	char btnSeq[128];

	if ( reset )
	{
		keyBindTree->clear();
	}

	i=0;

	for (it=GamePad[portNum].gpKeySeqList.begin(); it!=GamePad[portNum].gpKeySeqList.end(); it++)
	{
		binding = *it;

		if ( keyBindTree->topLevelItemCount() > i )
		{
			item = keyBindTree->topLevelItem(i);
		}
		else
		{
			item = NULL;
		}

		if ( item == NULL )
		{
			item = new QTreeWidgetItem();

			keyBindTree->addTopLevelItem(item);
		}

		btnName[0] = ButtonName( &binding->bmap[0] );

		btnSeq[0] = 0;

		if ( btnName[0][0] != 0 )
		{
			strcat( btnSeq, btnName[0] );
			strcat( btnSeq, " + ");
		}

		btnName[1] = ButtonName( &binding->bmap[1] );

		if ( btnName[1][0] != 0 )
		{
			strcat( btnSeq, btnName[1] );
		}

		item->setText(0, tr(btnSeq));
		item->setText(1, QString::fromStdString(binding->keySeq[0].name));
		item->setText(2, QString::fromStdString(binding->keySeq[1].name));

		item->setTextAlignment(0, Qt::AlignLeft);
		item->setTextAlignment(1, Qt::AlignLeft);
		item->setTextAlignment(2, Qt::AlignLeft);

		i++;
	}

	keyBindTree->viewport()->update();
}
//----------------------------------------------------
void GamePadConfDialog_t::updateCntrlrDpy(void)
{
	char keyNameStr[128];

	for (int i = 0; i < GAMEPAD_NUM_BUTTONS; i++)
	{
		if (GamePad[portNum].bmap[configIndex][i].ButtType == BUTTC_KEYBOARD)
		{
			snprintf(keyNameStr, sizeof(keyNameStr), "%s",
					 SDL_GetKeyName(GamePad[portNum].bmap[configIndex][i].ButtonNum));
		}
		else
		{
			strcpy(keyNameStr, ButtonName(&GamePad[portNum].bmap[configIndex][i]));
		}

		keyName[i]->setText(tr(keyNameStr));

		//if ( lcl[portNum].btn[i].needsSave )
		//{
		//	keyName[i]->setStyleSheet("color: red;");
		//}
		//else
		//{
		//	keyName[i]->setStyleSheet(NULL);
		//}
	}
}
//----------------------------------------------------
void GamePadConfDialog_t::portSelect(int index)
{
	const char *guid;
	//printf("Port Number:%i \n", index);
	portNum = index;
	updateCntrlrDpy();

	for (int i = 0; i < devSel->count(); i++)
	{
		if (devSel->itemData(i).toInt() == GamePad[portNum].getDeviceIndex())
		{
			devSel->setCurrentIndex(i);
		}
	}
	guid = GamePad[portNum].getGUID();

	if ( guid )
	{
		guidLbl->setText(guid);
	}

	loadMapList();

	refreshKeyBindTree(true);
}
//----------------------------------------------------
void GamePadConfDialog_t::deviceSelect(int index)
{
	jsDev_t *js;
	const char *guid;
	int devIdx = devSel->itemData(index).toInt();

	js = getJoystickDevice(devIdx);

	if (js != NULL)
	{
		if (js->isConnected())
		{
			guidLbl->setText(js->getGUID());
		}
	}
	else
	{
		guidLbl->setText("");
	}
	GamePad[portNum].setDeviceIndex(devIdx);

	guid = GamePad[portNum].getGUID();
	if ( guid )
	{
		lcl[portNum].guid.assign(guid);
	}
	lcl[portNum].profile.assign("default");

	loadMapList();

	updateCntrlrDpy();
}
//----------------------------------------------------
void GamePadConfDialog_t::ena4score(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;
	//printf("Set 'SDL.FourScore' = %i\n", value);
	g_config->setOption("SDL.FourScore", value);
}
//----------------------------------------------------
void GamePadConfDialog_t::oppDirEna(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;
	//printf("Set 'SDL.Input.EnableOppositeDirectionals' = %i\n", value);
	g_config->setOption("SDL.Input.EnableOppositeDirectionals", value);
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton(int padNo, int x)
{
	//char buf[256];
	//std::string prefix;
	const char *keyNameStr;

	if (buttonConfigStatus == 2)
	{
		buttonConfigStatus = 0;
		return;
	}
	buttonConfigStatus = 2;

	ButtonConfigBegin();

	button[x]->setText("Waiting");
	button[x]->setStyleSheet("background-color: green; color: white;");

	DWaitButton(NULL, &GamePad[padNo].bmap[configIndex][x], &buttonConfigStatus);

	button[x]->setText("Change");
	button[x]->setStyleSheet(NULL);

	if (buttonConfigStatus != 0)
	{
		keyNameStr = ButtonName(&GamePad[padNo].bmap[configIndex][x]);
		keyName[x]->setText( tr(keyNameStr) );
		lcl[padNo].btn[x].needsSave = 1;
	}

	ButtonConfigEnd();

	buttonConfigStatus = 1;
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton(int padNo, int x)
{
	GamePad[padNo].bmap[configIndex][x].ButtonNum = -1;

	//keyName[x]->setText( tr("") );
	keyName[x]->clear();

	lcl[padNo].btn[x].needsSave = 1;
}
//----------------------------------------------------
void GamePadConfDialog_t::closeEvent(QCloseEvent *event)
{
	if (changeSeqStatus != 0)
	{
		// change sequentially still in progress
		// now try to abort its progress
		changeSeqStatus = -1;
		// ... out from waiting gamepad button event loop
		buttonConfigStatus = 0;
		// and ignore this event
		event->ignore();
		return;
	}
	promptToSave();

	printf("GamePad Close Window Event\n");
	buttonConfigStatus = 0;
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------
void GamePadConfDialog_t::closeWindow(void)
{
	if (changeSeqStatus != 0)
	{
		// change sequentially still in progress
		// now try to abort its progress
		changeSeqStatus = -1;
		// ... out from waiting gamepad button event loop
		buttonConfigStatus = 0;
		return;
	}

	promptToSave();

	printf("Close Window\n");
	buttonConfigStatus = 0;
	done(0);
	deleteLater();
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton0(void)
{
	changeButton(portNum, 0);
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton1(void)
{
	changeButton(portNum, 1);
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton2(void)
{
	changeButton(portNum, 2);
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton3(void)
{
	changeButton(portNum, 3);
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton4(void)
{
	changeButton(portNum, 4);
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton5(void)
{
	changeButton(portNum, 5);
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton6(void)
{
	changeButton(portNum, 6);
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton7(void)
{
	changeButton(portNum, 7);
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton8(void)
{
	changeButton(portNum, 8);
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton9(void)
{
	changeButton(portNum, 9);
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton0(void)
{
	clearButton(portNum, 0);
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton1(void)
{
	clearButton(portNum, 1);
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton2(void)
{
	clearButton(portNum, 2);
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton3(void)
{
	clearButton(portNum, 3);
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton4(void)
{
	clearButton(portNum, 4);
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton5(void)
{
	clearButton(portNum, 5);
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton6(void)
{
	clearButton(portNum, 6);
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton7(void)
{
	clearButton(portNum, 7);
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton8(void)
{
	clearButton(portNum, 8);
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton9(void)
{
	clearButton(portNum, 9);
}
//----------------------------------------------------
void GamePadConfDialog_t::clearAllCallback(void)
{
	for (int i = 0; i < GAMEPAD_NUM_BUTTONS; i++)
	{
		clearButton(portNum, i);
	}
}
//----------------------------------------------------
void GamePadConfDialog_t::btnConfigChanged(int idx)
{
	configIndex = idx;

	updateCntrlrDpy();
}
//----------------------------------------------------
void GamePadConfDialog_t::saveConfig(void)
{
	int i;
	char stmp[256];
	const char *guid;
	std::string prefix, mapName;

	sprintf(stmp, "SDL.Input.GamePad.%u.", portNum);
	prefix = stmp;

	mapName = mapSel->currentText().toStdString();

	guid = GamePad[portNum].getGUID();

	if ( guid )
	{
		g_config->setOption(prefix + "DeviceGUID", guid);
	}
	g_config->setOption(prefix + "Profile", mapName.c_str());

	for (i = 0; i < GAMEPAD_NUM_BUTTONS; i++)
	{
		lcl[portNum].btn[i].needsSave = 0;
	}
	lcl[portNum].advBindNeedsSave = 0;

	g_config->save();
}
//----------------------------------------------------
void GamePadConfDialog_t::createNewProfile(const char *name)
{
	char stmp[256];
	//printf("Creating: %s \n", name );

	GamePad[portNum].createProfile(name);

	mapSel->addItem(tr(name));

	mapSel->setCurrentIndex(mapSel->count() - 1);
	saveConfig();

	sprintf(stmp, "Mapping Created: %s/%s \n", GamePad[portNum].getGUID(), name);
	mapMsg->setText(tr(stmp));
}
//----------------------------------------------------
void GamePadConfDialog_t::newProfileCallback(void)
{
	int ret;
	QInputDialog dialog(this);

	dialog.setWindowTitle(tr("New Profile"));
	dialog.setLabelText(tr("Specify New Profile Name"));
	dialog.setOkButtonText(tr("Create"));

	ret = dialog.exec();

	if (QDialog::Accepted == ret)
	{
		createNewProfile(dialog.textValue().toStdString().c_str());
	}
}
//----------------------------------------------------
void GamePadConfDialog_t::loadProfileCallback(void)
{
	char stmp[256];
	int index, devIdx, ret;
	std::string mapName;

	index = devSel->currentIndex();
	devIdx = devSel->itemData(index).toInt();

	mapName = mapSel->currentText().toStdString();

	GamePad[portNum].setDeviceIndex(devIdx);

	if (mapName.compare("default") == 0)
	{
		ret = GamePad[portNum].loadDefaults();
	}
	else
	{
		ret = GamePad[portNum].loadProfile(mapName.c_str());
	}
	if (ret == 0)
	{
		saveConfig();

		sprintf(stmp, "Mapping Loaded: %s/%s \n", GamePad[portNum].getGUID(), mapName.c_str());
	}
	else
	{
		sprintf(stmp, "Error: Failed to Load Mapping: %s/%s \n", GamePad[portNum].getGUID(), mapName.c_str());
	}
	mapMsg->setText(tr(stmp));

	updateCntrlrDpy();

	refreshKeyBindTree(true);
}
//----------------------------------------------------
void GamePadConfDialog_t::saveProfileCallback(void)
{
	int ret;
	std::string mapName;
	char stmp[256];

	mapName = mapSel->currentText().toStdString();

	ret = GamePad[portNum].saveCurrentMapToFile(mapName.c_str());

	if (ret == 0)
	{
		saveConfig();

		sprintf(stmp, "Mapping Saved: %s/%s \n", GamePad[portNum].getGUID(), mapName.c_str());
	}
	else
	{
		sprintf(stmp, "Error: Failed to Save Mapping: %s \n", mapName.c_str());
	}
	mapMsg->setText(tr(stmp));
}
//----------------------------------------------------
void GamePadConfDialog_t::deleteProfileCallback(void)
{
	int ret;
	std::string mapName;
	char stmp[256];

	mapName = mapSel->currentText().toStdString();

	ret = GamePad[portNum].deleteMapping(mapName.c_str());

	if (ret == 0)
	{
		sprintf(stmp, "Mapping Deleted: %s/%s \n", GamePad[portNum].getGUID(), mapName.c_str());
	}
	else
	{
		sprintf(stmp, "Error: Failed to Delete Mapping: %s \n", mapName.c_str());
	}
	mapMsg->setText(tr(stmp));

	loadMapList();
}
//----------------------------------------------------
void GamePadConfDialog_t::promptToSave(void)
{
	int i, j, n;
	std::string msg;
	QMessageBox msgBox(this);
	char saveRequired = 0;
	char padNeedsSave[GAMEPAD_NUM_DEVICES];
	char stmp[256];

	n = 0;
	for (i = 0; i < GAMEPAD_NUM_DEVICES; i++)
	{
		padNeedsSave[i] = 0;

		for (j = 0; j < GAMEPAD_NUM_BUTTONS; j++)
		{
			if (lcl[i].btn[j].needsSave || lcl[i].advBindNeedsSave)
			{
				padNeedsSave[i] = 1;
				saveRequired = 1;
				n++;
				break;
			}
		}
	}

	if (!saveRequired)
	{
		return;
	}
	sprintf(stmp, "Warning: Gamepad mappings have not been saved for port%c ", (n > 1) ? 's' : ' ');

	msg.assign(stmp);

	j = n;
	for (i = 0; i < GAMEPAD_NUM_DEVICES; i++)
	{
		if (padNeedsSave[i])
		{
			sprintf(stmp, "%i", i + 1);

			msg.append(stmp);

			j--;

			if (j > 1)
			{
				msg.append(", ");
			}
			else if (j == 1)
			{
				msg.append(" and ");
			}
		}
	}
	msg.append(".\n");
	msg.append("Mappings will not persist on emulator restart unless saved.");

	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setText(tr(msg.c_str()));

	msgBox.exec();
}
//----------------------------------------------------
void GamePadConfDialog_t::newKeyBindingCallback(void)
{
	GamePadFuncConfigDialog *dialog = new GamePadFuncConfigDialog( portNum, NULL, this );

	dialog->show();
}
//----------------------------------------------------
void GamePadConfDialog_t::editKeyBindingCallback(void)
{
	QTreeWidgetItem *item;

	item = keyBindTree->currentItem();

	if ( item == NULL )
	{
		printf( "No Item Selected\n");
		return;
	}
	int i, row;
	gamepad_function_key_t *k = NULL;
	std::list <gamepad_function_key_t*>::iterator it;

	row = keyBindTree->indexOfTopLevelItem(item);

	i=0;
	for (it=GamePad[portNum].gpKeySeqList.begin(); it!=GamePad[portNum].gpKeySeqList.end(); it++)
	{
		if ( i == row )
		{
			k = *it; break;
		}
	}
	GamePadFuncConfigDialog *dialog = new GamePadFuncConfigDialog( portNum, k, this );

	dialog->show();
}
//----------------------------------------------------
void GamePadConfDialog_t::delKeyBindingCallback(void)
{
	QTreeWidgetItem *item;

	item = keyBindTree->currentItem();

	if ( item == NULL )
	{
		printf( "No Item Selected\n");
		return;
	}
	int i, row;
	gamepad_function_key_t *k = NULL;
	std::list <gamepad_function_key_t*>::iterator it;

	row = keyBindTree->indexOfTopLevelItem(item);

	i=0;
	for (it=GamePad[portNum].gpKeySeqList.begin(); it!=GamePad[portNum].gpKeySeqList.end(); it++)
	{
		if ( i == row )
		{
			k = *it;
			GamePad[portNum].gpKeySeqList.erase(it);
			delete k;
			break;
		}
	}
	refreshKeyBindTree(true);
}
//----------------------------------------------------
void GamePadConfDialog_t::updatePeriodic(void)
{
	bool buttonEnable;
	char jsFound[ MAX_JOYSTICKS ];
	QTreeWidgetItem *item;

	memset( jsFound, 0, sizeof(jsFound) );

	for (int i = 0; i < devSel->count(); i++)
	{
		int devIdx = devSel->itemData(i).toInt();

		if ( devIdx >= 0 )
		{
			jsDev_t *js = getJoystickDevice(devIdx);

			if (js != NULL)
			{
				if (!js->isConnected())
				{
					//printf("Removing Disconnected JS\n");
					devSel->removeItem(i);
					deviceSelect( devSel->currentIndex() );
				}
			}
			else
			{
				//printf("Removing NULL JS\n");
				devSel->removeItem(i);
				deviceSelect( devSel->currentIndex() );
			}
			jsFound[ devIdx ] = 1;
		}
	}

	for (int i = 0; i < MAX_JOYSTICKS; i++)
	{
		jsDev_t *js = getJoystickDevice(i);

		if (js != NULL)
		{
			if (js->isConnected())
			{
				if ( !jsFound[i] )
				{
					char stmp[256];
					//printf("Adding Newly Connected JS\n");
					sprintf(stmp, "%i: %s", i, js->getName());
					devSel->addItem(tr(stmp), i);
				}
			}
		}
	}

	for (int i = 0; i < GAMEPAD_NUM_BUTTONS; i++)
	{
		//const char *txt, *style;
		//if ( GamePad[portNum].bmap[i].state )
		//{
		//   txt = "  T  ";
		//   style = "background-color: green; color: white;";
		//}
		//else
		//{
		//   txt = "  F  ";
		//   style = "background-color: red; color: white;";
		//}
		//keyState[i]->setText( tr(txt) );
		//keyState[i]->setStyleSheet( style );

		if (lcl[portNum].btn[i].needsSave)
		{
			keyName[i]->setStyleSheet("color: red;");
		}
		else
		{
			keyName[i]->setStyleSheet(NULL);
		}
	}

	bool fourScore = false;
	g_config->getOption("SDL.FourScore", &fourScore);
	if (fourScore != efs_chkbox->isChecked())
	{
		efs_chkbox->setChecked(fourScore);
	}


	gpView->setPort(portNum);
	gpView->update();

	refreshKeyBindTree();

	item = keyBindTree->currentItem();

	if ( item == NULL )
	{
		buttonEnable = false;
	}
	else
	{
		buttonEnable = true;
	}
	editKeyBindBtn->setEnabled(buttonEnable);
	 delKeyBindBtn->setEnabled(buttonEnable);
}

//----------------------------------------------------

void GamePadConfDialog_t::changeSequentallyCallback(void)
{
	// get pointer to button
	QPushButton *changeSeqButton = qobject_cast<QPushButton *>(sender());
	if (!changeSeqButton)
		return;

	// disable it for user input
	changeSeqButton->setEnabled(false);

	// change button for every button on gamepad
	for (int i = 0; i < GAMEPAD_NUM_BUTTONS; ++i)
	{
		changeSeqStatus += 1;
		if (this->isHidden())
			break;
		changeButton(portNum, i);
		if (this->isHidden())
			break;
		// abort?
		if (changeSeqStatus == -1)
			break;
	}
	// return all back
	changeSeqStatus = 0;
	changeSeqButton->setEnabled(true);
}

//----------------------------------------------------
GamePadConfigButton_t::GamePadConfigButton_t(int i)
{
	idx = i;
	setText("Change");
}
//----------------------------------------------------
void GamePadConfigButton_t::keyPressEvent(QKeyEvent *event)
{
	//printf("GamePad Button Key Press: 0x%x \n", event->key() );
	pushKeyEvent(event, 1);
}

void GamePadConfigButton_t::keyReleaseEvent(QKeyEvent *event)
{
	//printf("GamePad Button Key Release: 0x%x \n", event->key() );
	pushKeyEvent(event, 0);
}
//----------------------------------------------------
//----------------------------------------------------
// Game Pad Viewer
//----------------------------------------------------
//----------------------------------------------------
GamePadView_t::GamePadView_t(QWidget *parent)
	: QWidget(parent)
{
	this->setFocusPolicy(Qt::StrongFocus);
	this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	this->setMouseTracking(true);
	viewWidth = 128 * 3;
	viewHeight = 128;
	setMinimumWidth(viewWidth);
	setMinimumHeight(viewHeight);

	font.setFamily("Courier New");
	font.setStyle(QFont::StyleNormal);
	font.setStyleHint(QFont::Monospace);
	QFontMetrics fm(font);

#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
	pxCharWidth = fm.horizontalAdvance(QLatin1Char('2'));
#else
	pxCharWidth = fm.width(QLatin1Char('2'));
#endif
	pxCharHeight  = fm.capHeight();
	pxLineSpacing = fm.lineSpacing();
	pxLineLead    = fm.leading();
	pxLineLead2   = pxLineLead / 2;
	pxLeftBearing = fm.leftBearing(QLatin1Char('A'));

	portNum = 0;
}
//----------------------------------------------------
GamePadView_t::~GamePadView_t(void)
{
}
//----------------------------------------------------
void GamePadView_t::setPort(int port)
{
	portNum = port;
}
//----------------------------------------------------
void GamePadView_t::resizeEvent(QResizeEvent *event)
{
	viewWidth = event->size().width();
	viewHeight = event->size().height();

	//printf("%ix%i\n", event->size().width(), event->size().height() );
}
//----------------------------------------------------
void GamePadView_t::keyPressEvent(QKeyEvent *event)
{
	pushKeyEvent(event, 1);
}
//----------------------------------------------------
void GamePadView_t::keyReleaseEvent(QKeyEvent *event)
{
	pushKeyEvent(event, 0);
}
//----------------------------------------------------
void GamePadView_t::mouseMoveEvent(QMouseEvent *event)
{
}
//----------------------------------------------------
void GamePadView_t::mousePressEvent(QMouseEvent *event)
{
}
//----------------------------------------------------
void GamePadView_t::drawLetterOnButton(QPainter &painter, QRect &rect, QColor &color, int ch)
{
	int x, y;
	char c[2];

	c[0] = ch;
	c[1] = 0;

	x = rect.x() + (rect.width() - pxCharWidth) / 2;
	y = rect.y() + (rect.height() - pxCharHeight) / 2;

	painter.setPen(color);
	painter.setFont(font);
	painter.drawText(x + pxLeftBearing, y + pxCharHeight - pxLineLead2, tr(c));
}
//----------------------------------------------------
void GamePadView_t::setFontPixelSize(int px)
{
	font.setPixelSize( px );
	QFontMetrics fm(font);

#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
	pxCharWidth = fm.horizontalAdvance(QLatin1Char('2'));
#else
	pxCharWidth = fm.width(QLatin1Char('2'));
#endif
	pxCharHeight  = fm.capHeight();
	pxLineSpacing = fm.lineSpacing();
	pxLineLead    = fm.leading();
	pxLineLead2   = pxLineLead / 2;
	pxLeftBearing = fm.leftBearing(QLatin1Char('A'));
}
//----------------------------------------------------
void GamePadView_t::paintEvent(QPaintEvent *event)
{
	int w, h, x, y, b = 5;
	int w3, bw, bh, ws, hs, ht, ws2;
	QPainter painter(this);
	QColor white(255, 255, 255);
	QColor black(0, 0, 0);
	QColor clear(0, 0, 0, 0);
	QColor gray(128, 128, 128);
	QColor green(0, 255, 0);
	QRect gp1, gp2;
	QRect upBtn, dnBtn, lBtn, rBtn, cBtn;
	QRect selBtn, stBtn;
	QRect bBtn, aBtn, tbBtn, taBtn, tBox;

	w = viewWidth;
	h = viewHeight;

	if (w > (3 * h))
	{
		w = 3 * h;
	}
	else
	{
		h = w / 3;
	}
	x = (viewWidth - w) / 2;
	y = (viewHeight - h) / 2;

	gp1.setRect(x, y, w, h);

	painter.fillRect(gp1, gray);

	x = x + b;
	y = y + b;
	w = w - (2 * b);
	h = h - (2 * b);

	gp2.setRect(x, y, w, h);

	painter.fillRect(gp2, black);

	w3 = w / 3;
	bw = w3 / 5;
	bh = bw;

	x = gp2.x() + bw;
	y = gp2.y() + (h / 5);

	upBtn.setRect(x + bw, y, bw, bh);
	dnBtn.setRect(x + bw, y + (bh * 2), bw, bh);
	lBtn.setRect(x, y + bh, bw, bh);
	rBtn.setRect(x + (bw * 2), y + bh, bw, bh);
	cBtn.setRect(x + bw, y + bh, bw, bh);

	painter.fillRect(upBtn.x() - b, upBtn.y() - b, upBtn.width() + (2 * b), 3 * upBtn.height() + (2 * b), gray);
	painter.fillRect(lBtn.x() - b, lBtn.y() - b, 3 * lBtn.width() + (2 * b), lBtn.height() + (2 * b), gray);

	painter.fillRect(cBtn, black);

	if (GamePad[portNum].bmapState[4])
	{
		painter.fillRect(upBtn, green);
	}
	else
	{
		painter.fillRect(upBtn, black);
	}

	if (GamePad[portNum].bmapState[5])
	{
		painter.fillRect(dnBtn, green);
	}
	else
	{
		painter.fillRect(dnBtn, black);
	}

	if (GamePad[portNum].bmapState[6])
	{
		painter.fillRect(lBtn, green);
	}
	else
	{
		painter.fillRect(lBtn, black);
	}

	if (GamePad[portNum].bmapState[7])
	{
		painter.fillRect(rBtn, green);
	}
	else
	{
		painter.fillRect(rBtn, black);
	}

	bw = w3 / 3;
	bh = bw / 2;

	x = gp2.x() + w3;
	y = gp2.y() + (3 * h / 5);

	selBtn.setRect(x + (bw / 3), y, bw, bh);
	stBtn.setRect(x + (5 * bw) / 3, y, bw, bh);

	if (GamePad[portNum].bmapState[2])
	{
		painter.fillRect(selBtn, green);
	}
	else
	{
		painter.fillRect(selBtn, gray);
	}

	if (GamePad[portNum].bmapState[3])
	{
		painter.fillRect(stBtn, green);
	}
	else
	{
		painter.fillRect(stBtn, gray);
	}

	//bw = w3 / 3;
	bh = h / 3;
	bw = bh;
	setFontPixelSize( bh / 2 );
	painter.setFont(font);

	ht = pxLineSpacing;
	bh = (h - ht) / 3;
	bw = bh;
	hs = (ht - pxCharHeight) / 2;
	ws = (w3 - (5 * pxCharWidth)) / 2;

	painter.setPen(white);

	x = gp2.x() + (2 * w3) + ws;
	y = gp2.y() + (ht - hs);

	painter.drawText(x, y, tr("Turbo"));

	ws = (w3 - (2 * bw)) / 3;
	hs = (h - (2 * bh) - ht) / 3;
	ws2 = (ws / 2);

	x = gp2.x() + (2 * w3);
	y = gp2.y() + (ht + hs);

	tbBtn.setRect(x + ws, y, bw, bh);
	taBtn.setRect(x + bw + (2 * ws), y, bw, bh);

	y = gp2.y() + (ht + bh + (2 * hs));

	bBtn.setRect(x + ws, y, bw, bh);
	aBtn.setRect(x + bw + (2 * ws), y, bw, bh);

	if (GamePad[portNum].bmapState[9])
	{
		painter.setBrush(Qt::green);
	}
	else
	{
		painter.setBrush(Qt::red);
	}
	painter.setPen(white);
	painter.drawEllipse(tbBtn);
	drawLetterOnButton(painter, tbBtn, black, 'B');

	if (GamePad[portNum].bmapState[8])
	{
		painter.setBrush(Qt::green);
	}
	else
	{
		painter.setBrush(Qt::red);
	}
	painter.setPen(white);
	painter.drawEllipse(taBtn);
	drawLetterOnButton(painter, taBtn, black, 'A');

	if (GamePad[portNum].bmapState[1])
	{
		painter.setBrush(Qt::green);
	}
	else
	{
		painter.setBrush(Qt::red);
	}
	painter.setPen(white);
	painter.drawEllipse(bBtn);
	drawLetterOnButton(painter, bBtn, black, 'B');

	if (GamePad[portNum].bmapState[0])
	{
		painter.setBrush(Qt::green);
	}
	else
	{
		painter.setBrush(Qt::red);
	}
	painter.setPen(white);
	painter.drawEllipse(aBtn);
	drawLetterOnButton(painter, aBtn, black, 'A');

	painter.setBrush(clear);

	tBox.setTop(gp2.y() + 2);
	tBox.setLeft(gp2.x() + (2 * w3) + ws2);
	tBox.setWidth(w3 - ws);
	tBox.setBottom(taBtn.y() + (aBtn.y() - taBtn.y() + bh) / 2);

	painter.setPen(white);
	painter.drawRect(tBox);
}
//----------------------------------------------------
// Game Pad Function Config
//----------------------------------------------------
GamePadFuncConfigDialog::GamePadFuncConfigDialog( int portNumIn, gamepad_function_key_t *fk, QWidget *parent )
	: QDialog(parent)
{
	QHBoxLayout *hbox;
	QVBoxLayout *mainLayout;
	QGridLayout *grid;
	//QLabel *lbl;
	QGroupBox *frame;
	QPushButton *okButton, *cancelButton;
	QPushButton *clearButton[4];
	const char *keyNameStr;

	portNum = portNumIn;

	if ( fk == NULL )
	{
		editMode = false;
		k = new gamepad_function_key_t();
	}
	else
	{
		editMode = true;
		k = fk;
	}

	if ( editMode )
	{
		setWindowTitle( tr("Edit Gamepad Key Mapping") );
	}
	else
	{
		setWindowTitle( tr("Add Gamepad Key Mapping") );
	}

	btnLbl[0]    = new QLineEdit();
	btnLbl[1]    = new QLineEdit();
	keySeqLbl[0] = new QLineEdit();
	keySeqLbl[1] = new QLineEdit();

	btnLbl[0]->setReadOnly(true);
	btnLbl[1]->setReadOnly(true);
	keySeqLbl[0]->setReadOnly(true);
	keySeqLbl[1]->setReadOnly(true);

	btnLbl[0]->setAlignment( Qt::AlignCenter );
	btnLbl[1]->setAlignment( Qt::AlignCenter );
	keySeqLbl[0]->setAlignment( Qt::AlignCenter );
	keySeqLbl[1]->setAlignment( Qt::AlignCenter );

	b[0]  = new GamePadConfigButton_t(0);
	b[1]  = new GamePadConfigButton_t(1);
	hk[0] = new GamePadConfigHotKey_t(0,k);
	hk[1] = new GamePadConfigHotKey_t(1,k);

	clearButton[0] = new QPushButton( tr("Clear") );
	clearButton[1] = new QPushButton( tr("Clear") );
	clearButton[2] = new QPushButton( tr("Clear") );
	clearButton[3] = new QPushButton( tr("Clear") );

	mainLayout = new QVBoxLayout();

	setLayout(mainLayout);

	frame = new QGroupBox( tr("Game Pad Button Sequence:") );
	mainLayout->addWidget( frame );

	grid = new QGridLayout();
	frame->setLayout( grid );

	//grid->setColumnMinimumWidth( 1, 20 );

	grid->addWidget( new QLabel( tr("Modifier Button:") ), 0, 0 );
	grid->addWidget( btnLbl[0], 0, 1 );
	grid->addWidget( b[0], 0, 2 );
	grid->addWidget( clearButton[0], 0, 3 );

	grid->addWidget( new QLabel( tr("Primary Button:") ), 1, 0 );
	grid->addWidget( btnLbl[1], 1, 1 );
	grid->addWidget( b[1], 1, 2 );
	grid->addWidget( clearButton[1], 1, 3 );

	frame = new QGroupBox( tr("Maps to Key Sequence:") );
	mainLayout->addWidget( frame );

	grid = new QGridLayout();
	frame->setLayout( grid );

	grid->addWidget( new QLabel( tr("On Press:") ), 0, 0 );
	grid->addWidget( keySeqLbl[0], 0, 1 );
	grid->addWidget( hk[0], 0, 2 );
	grid->addWidget( clearButton[2], 0, 3 );

	grid->addWidget( new QLabel( tr("On Release:") ), 1, 0 );
	grid->addWidget( keySeqLbl[1], 1, 1 );
	grid->addWidget( hk[1], 1, 2 );
	grid->addWidget( clearButton[3], 1, 3 );

	hbox         = new QHBoxLayout();
	okButton     = new QPushButton( tr("OK") );
	cancelButton = new QPushButton( tr("Cancel") );

	    okButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
	cancelButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));

	mainLayout->addLayout( hbox );
	hbox->addWidget( cancelButton, 1 );
	hbox->addStretch( 5 );
	hbox->addWidget(     okButton, 1 );

	connect(     okButton, SIGNAL(clicked(void)), this, SLOT(acceptCB(void)) );
	connect( cancelButton, SIGNAL(clicked(void)), this, SLOT(rejectCB(void)) );

	okButton->setDefault(true);

	//ret = dialog.exec();
	buttonConfigStatus = 0;

	connect( b[0] , SIGNAL(clicked(void)), this, SLOT(changeButton0(void)) );
	connect( b[1] , SIGNAL(clicked(void)), this, SLOT(changeButton1(void)) );
	connect( hk[0], SIGNAL(clicked(void)), this, SLOT(changeKeySeq0(void) ) );
	connect( hk[1], SIGNAL(clicked(void)), this, SLOT(changeKeySeq1(void) ) );

	connect( clearButton[0], SIGNAL(clicked(void)), this, SLOT(clearButton0(void)) );
	connect( clearButton[1], SIGNAL(clicked(void)), this, SLOT(clearButton1(void)) );
	connect( clearButton[2], SIGNAL(clicked(void)), this, SLOT(clearButton2(void)) );
	connect( clearButton[3], SIGNAL(clicked(void)), this, SLOT(clearButton3(void)) );

	hk[0]->setKeyNameLbl( keySeqLbl[0] );
	hk[1]->setKeyNameLbl( keySeqLbl[1] );

	keyNameStr = ButtonName(&k->bmap[0]);

	btnLbl[0]->setText( tr(keyNameStr) );

	keyNameStr = ButtonName(&k->bmap[1]);

	btnLbl[1]->setText( tr(keyNameStr) );

	keySeqLbl[0]->setText( QString::fromStdString(k->keySeq[0].name) );
	keySeqLbl[1]->setText( QString::fromStdString(k->keySeq[1].name) );
}
//----------------------------------------------------
GamePadFuncConfigDialog::~GamePadFuncConfigDialog(void)
{
	int ret;

	buttonConfigStatus = 0;

	ret = result();

	if ( ret == QDialog::Accepted )
	{
		if ( !editMode )
		{
			GamePad[portNum].gpKeySeqList.push_back( k );
		}
		lcl[portNum].advBindNeedsSave = 1;
	}
	else
	{
		if ( !editMode )
		{
			delete k;
		}
	}

	updateRemapTree = true;
}
//----------------------------------------------------
void GamePadFuncConfigDialog::closeEvent(QCloseEvent *event)
{
	done( result() );
	deleteLater();
	event->accept();
}
//----------------------------------------------------
void GamePadFuncConfigDialog::acceptCB(void)
{
	done( QDialog::Accepted );
	deleteLater();
}
//----------------------------------------------------
void GamePadFuncConfigDialog::rejectCB(void)
{
	done( QDialog::Rejected );
	deleteLater();
}
//----------------------------------------------------
void GamePadFuncConfigDialog::closeWindow(void)
{
	done( result() );
	deleteLater();
}
//----------------------------------------------------
void GamePadFuncConfigDialog::changeButton(int x)
{
	//char buf[256];
	//std::string prefix;
	const char *keyNameStr;

	if (buttonConfigStatus == 2)
	{
		buttonConfigStatus = 0;
		return;
	}
	buttonConfigStatus = 2;

	ButtonConfigBegin();

	b[x]->setText("Waiting");
	b[x]->setStyleSheet("background-color: green; color: white;");

	DWaitButton(NULL, &k->bmap[x], &buttonConfigStatus);

	b[x]->setText("Change");
	b[x]->setStyleSheet(NULL);

	if (buttonConfigStatus != 0)
	{
		keyNameStr = ButtonName(&k->bmap[x]);
		btnLbl[x]->setText( tr(keyNameStr) );
	}

	ButtonConfigEnd();

	buttonConfigStatus = 1;
}
//----------------------------------------------------
void GamePadFuncConfigDialog::changeButton0(void)
{
	changeButton(0);
}
//----------------------------------------------------
void GamePadFuncConfigDialog::changeButton1(void)
{
	changeButton(1);
}
//----------------------------------------------------
void GamePadFuncConfigDialog::changeKeySeq0(void)
{
	int ret;
	HotKeySelectDialog_t hkd;

	ret = hkd.exec();

	if ( ret == QDialog::Accepted )
	{
		hotkey_t *hkp;
		char keyName[128];

		//printf("Accepted Hot Key: %i\n", hkd.getSelHotKey() );
		k->hk[0] = hkd.getSelHotKey();

		hkp = &Hotkeys[ k->hk[0] ];

		hkp->getString(keyName);

		k->keySeq[0].key      = hkp->qkey.value;
		k->keySeq[0].modifier = hkp->qkey.modifier;
		k->keySeq[0].name.assign(hkp->getConfigName());

		if ( keySeqLbl[0] )
		{
			keySeqLbl[0]->setText( tr(hkp->getConfigName()) );
		}
	}
	//else
	//{
	//	printf("Rejected Hot Key\n");
	//}

	//hk[0]->setCaptureState(true);
	//hk[0]->setStyleSheet("background-color: green; color: white;");
}
//----------------------------------------------------
void GamePadFuncConfigDialog::changeKeySeq1(void)
{
	int ret;
	HotKeySelectDialog_t hkd;

	ret = hkd.exec();

	if ( ret == QDialog::Accepted )
	{
		hotkey_t *hkp;
		char keyName[128];

		//printf("Accepted Hot Key: %i\n", hkd.getSelHotKey() );
		k->hk[1] = hkd.getSelHotKey();

		hkp = &Hotkeys[ k->hk[1] ];

		hkp->getString(keyName);

		k->keySeq[1].key      = hkp->qkey.value;
		k->keySeq[1].modifier = hkp->qkey.modifier;
		k->keySeq[1].name.assign(hkp->getConfigName());

		if ( keySeqLbl[1] )
		{
			keySeqLbl[1]->setText( tr(hkp->getConfigName()) );
		}
	}
	//else
	//{
	//	printf("Rejected Hot Key\n");
	//}

	//hk[1]->setCaptureState(true);
	//hk[1]->setStyleSheet("background-color: green; color: white;");
}
//----------------------------------------------------
void GamePadFuncConfigDialog::clearButton0(void)
{
	k->bmap[0].ButtType  = -1;
	k->bmap[0].DeviceNum = -1;
	k->bmap[0].ButtonNum = -1;
	k->bmap[0].state     =  0;

	btnLbl[0]->clear();
}
//----------------------------------------------------
void GamePadFuncConfigDialog::clearButton1(void)
{
	k->bmap[1].ButtType  = -1;
	k->bmap[1].DeviceNum = -1;
	k->bmap[1].ButtonNum = -1;
	k->bmap[1].state     =  0;

	btnLbl[1]->clear();
}
//----------------------------------------------------
void GamePadFuncConfigDialog::clearButton2(void)
{
	k->keySeq[0].key = 0;
	k->keySeq[0].modifier = 0;
	k->keySeq[0].name.clear();
	k->hk[0] = -1;

	keySeqLbl[0]->clear();
}
//----------------------------------------------------
void GamePadFuncConfigDialog::clearButton3(void)
{
	k->keySeq[1].key = 0;
	k->keySeq[1].modifier = 0;
	k->keySeq[1].name.clear();
	k->hk[1] = -1;

	keySeqLbl[1]->clear();
}
//----------------------------------------------------
GamePadConfigHotKey_t::GamePadConfigHotKey_t(int idxIn, gamepad_function_key_t *fk)
{
	setText("Change");

	k = fk;
	keySeqLbl = NULL;
	captureState = false;
	idx = idxIn;
}
//----------------------------------------------------
void GamePadConfigHotKey_t::setKeyNameLbl( QLineEdit *lbl )
{
	keySeqLbl = lbl;
}
//----------------------------------------------------
void GamePadConfigHotKey_t::keyPressEvent(QKeyEvent *event)
{
	bool isModifier;
	//printf("GamePad Hot Key Press: 0x%x  '%s'\n", event->key(), event->text().toStdString().c_str() );
	
	isModifier = (event->key() == Qt::Key_Shift   ) ||
	             (event->key() == Qt::Key_Control ) ||
	             (event->key() == Qt::Key_Meta    ) ||
	             (event->key() == Qt::Key_Alt     ) ||
	             (event->key() == Qt::Key_AltGr   ) ||
	             (event->key() == Qt::Key_CapsLock);

	if ( captureState && !isModifier )
	{
		QKeySequence ks( convKeyEvent2Sequence(event) );

		k->keySeq[idx].key      = event->key();
		k->keySeq[idx].modifier = event->modifiers();
		k->keySeq[idx].name     = ks.toString().toStdString();

		if ( keySeqLbl )
		{
			keySeqLbl->setText( ks.toString() );
		}
		captureState = false;
		setStyleSheet(NULL);
	}
}

void GamePadConfigHotKey_t::keyReleaseEvent(QKeyEvent *event)
{
	//printf("GamePad Hot Key Release: 0x%x \n", event->key() );
}
//----------------------------------------------------
// Hot Key Selection Dialog
//----------------------------------------------------
HotKeySelectDialog_t::HotKeySelectDialog_t( QWidget *parent )
	: QDialog(parent)
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QTreeWidgetItem *item;
	std::string prefix = "SDL.Hotkeys.";

	hotKeyIdx = -1;

	setWindowTitle("Hotkey Select");

	resize(512, 512);

	mainLayout = new QVBoxLayout();

	tree = new QTreeWidget(this);

	tree->setColumnCount(2);
	tree->setSelectionMode( QAbstractItemView::SingleSelection );

	item = new QTreeWidgetItem();
	item->setText(0, QString::fromStdString("Command"));
	item->setText(1, QString::fromStdString("Key"));
	item->setTextAlignment(0, Qt::AlignLeft);
	item->setTextAlignment(1, Qt::AlignCenter);

	tree->setHeaderItem(item);

	tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

	for (int i = 0; i < HK_MAX; i++)
	{
		char keyName[128];
		std::string optionName = prefix + Hotkeys[i].getConfigName();

		//g_config->getOption (optionName.c_str (), &keycode);
		Hotkeys[i].getString(keyName);

		item = new QTreeWidgetItem();

		tree->addTopLevelItem(item);

		//item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren );
		//item->setCheckState( 0, Qt::Checked );

		item->setText(0, QString::fromStdString(optionName));
		item->setText(1, QString::fromStdString(keyName));

		item->setTextAlignment(0, Qt::AlignLeft);
		item->setTextAlignment(1, Qt::AlignCenter);

	}

	connect( tree, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
			this, SLOT(hotkeyItemClicked(QTreeWidgetItem *, int)));

	mainLayout->addWidget(tree);

	okButton = new QPushButton( tr("Ok") );
	okButton->setIcon(style()->standardIcon(QStyle::SP_DialogOkButton));
	okButton->setEnabled(false);
	connect(okButton, SIGNAL(clicked(void)), this, SLOT(acceptCB(void)));

	cancelButton = new QPushButton( tr("Cancel") );
	cancelButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
	connect(cancelButton, SIGNAL(clicked(void)), this, SLOT(rejectCB(void)));

	hbox = new QHBoxLayout();
	hbox->addWidget( cancelButton, 1 );
	hbox->addStretch(5);
	hbox->addWidget( okButton, 1 );
	mainLayout->addLayout( hbox );

	setLayout(mainLayout);
}
//----------------------------------------------------
HotKeySelectDialog_t::~HotKeySelectDialog_t(void)
{

}
//----------------------------------------------------
void HotKeySelectDialog_t::hotkeyItemClicked(QTreeWidgetItem *item, int column)
{
	int row = tree->indexOfTopLevelItem(item);

	if ( (row >= 0) && (row < HK_MAX) )
	{
		hotKeyIdx = row;
		okButton->setEnabled(true);
	}
}
//----------------------------------------------------
void HotKeySelectDialog_t::closeEvent(QCloseEvent *event)
{
	done( result() );
	deleteLater();
	event->accept();
}
//----------------------------------------------------
void HotKeySelectDialog_t::acceptCB(void)
{
	done( QDialog::Accepted );
	deleteLater();
}
//----------------------------------------------------
void HotKeySelectDialog_t::rejectCB(void)
{
	done( QDialog::Rejected );
	deleteLater();
}
//----------------------------------------------------
void HotKeySelectDialog_t::closeWindow(void)
{
	done( result() );
	deleteLater();
}
//----------------------------------------------------
