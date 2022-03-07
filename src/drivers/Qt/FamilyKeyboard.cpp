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
// FamilyKeyboard.cpp
#include <SDL.h>
#include <QHeaderView>
#include <QCloseEvent>
#include <QFileDialog>
#include <QGroupBox>
#include <QPainter>
#include <QFontMetrics>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFontDialog>
#include <QSettings>
#include <QDir>

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/FamilyKeyboard.h"

static const char *keyNames[] =
{
    "F1", "F1",
    "F2", "F2",
    "F3", "F3",
    "F4", "F4",
    "F5", "F5",
    "F6", "F6",
    "F7", "F7",
    "F8", "F8",
    "1", "!",
    "2", "\"",
    "3", "#",
    "4", "$",
    "5", "%",
    "6", "&",
    "7", "'",
    "8", "(",
    "9", ")",
    "0", "0",
    "-", "=",
    "^", "^",
    "¥", "¥",
    "STOP", "STOP",
    "ESC", "ESC",
    "Q", "Q",
    "W", "W",
    "E", "E",
    "R", "R",
    "T", "T",
    "Y", "Y",
    "U", "U",
    "I", "I",
    "O", "O",
    "P", "P",
    "@", "@",
    "[", "[",
    "RETURN", "RETURN",
    "CTR", "CTR",
    "A", "A",
    "S", "S",
    "D", "D",
    "F", "F",
    "G", "G",
    "H", "H",
    "J", "J",
    "K", "K",
    "L", "L",
    ";", "+",
    ":", "*",
    "]", "]",
    "KANA", "KANA",
    "SHIFT", "SHIFT",
    "Z", "Z",
    "X", "X",
    "C", "C",
    "V", "V",
    "B", "B",
    "N", "N",
    "M", "M",
    ",", "<",
    ".", ">",
    "/", "?",
    "_", "_",
    "SHIFT", "SHIFT",
    "GRPH", "GRPH",
    "SPACE", "SPACE",
    "HOM", "CLR",
    "INS", "INS",
    "DEL", "DEL",
    "UP", "UP",
    "LEFT", "LEFT",
    "RIGHT", "RIGHT",
    "DOWN", "DOWN",
     NULL, NULL
};

static FKBConfigDialog *fkbWin = NULL;
static bool fkbActv = false;
//*********************************************************************************
int openFamilyKeyboardDialog(QWidget *parent)
{
	if (fkbWin != NULL)
	{
		fkbWin->activateWindow();
		fkbWin->raise();
		fkbWin->setFocus();
	}
	else
	{
		fkbWin = new FKBConfigDialog(parent);
		fkbWin->show();
	}
	return 0;
}
//*********************************************************************************
char getFamilyKeyboardVirtualKey( int idx )
{
	char state = 0;

	if (fkbWin != NULL)
	{
		state = fkbWin->keyboard->key[idx].vState;
	}
	return state;
}
//*********************************************************************************
FamilyKeyboardWidget::FamilyKeyboardWidget( QWidget *parent )
{
	QFont font;
	std::string fontString;

	setMouseTracking(true);
	setFocusPolicy(Qt::StrongFocus);

	g_config->getOption("SDL.FamilyKeyboardFont", &fontString);

	if ( fontString.size() > 0 )
	{
		font.fromString( QString::fromStdString( fontString ) );
	}
	else
	{
		font.setFamily("Courier New");
		font.setStyle( QFont::StyleNormal );
		font.setStyleHint( QFont::Monospace );
	}
	setFont( font );
		
	setMinimumSize( 512, 256 );

	calcFontData();

	keyPressed = -1;
	keyUnderMouse = -1;
	ctxMenuKey = -1;

	// Set Shift Keys to Toggle State On Press
	key[50].toggleOnPress = true;
	key[62].toggleOnPress = true;

	updateTimer = new QTimer(this);

	connect(updateTimer, &QTimer::timeout, this, &FamilyKeyboardWidget::updatePeriodic);

	updateTimer->start(50); // 20hz
}
//*********************************************************************************
FamilyKeyboardWidget::~FamilyKeyboardWidget(void)
{
	updateTimer->stop();
	
}
//*********************************************************************************
void FamilyKeyboardWidget::setFont( const QFont &newFont )
{
	QWidget::setFont(newFont);

	calcFontData();
}
//*********************************************************************************
void FamilyKeyboardWidget::calcFontData(void)
{
	QWidget::setFont(font());
	QFontMetrics metrics(font());
#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
	pxCharWidth = metrics.horizontalAdvance(QLatin1Char('2'));
#else
	pxCharWidth = metrics.width(QLatin1Char('2'));
#endif
	pxCharHeight   = metrics.capHeight();
	//pxLineSpacing  = metrics.lineSpacing() * 1.25;
	//pxCursorHeight = metrics.height();
	
	pxBtnGridX = (3 * pxCharWidth) + 2;
	pxBtnGridY = (2 * pxCharHeight);

	if ( pxBtnGridX % 2 )
	{
		pxBtnGridX++;
	}
	if ( pxBtnGridY % 2 )
	{
		pxBtnGridY++;
	}
	setMinimumWidth( pxBtnGridX * 26 );
	setMinimumHeight( pxBtnGridY * 8 );
}
//*********************************************************************************
void FamilyKeyboardWidget::updatePeriodic(void)
{
	updateHardwareStatus();

	update();
}
//*********************************************************************************
void FamilyKeyboardWidget::updateHardwareStatus(void)
{
	const uint8 *hwKeyState = getFamilyKeyboardState();

	for (int i=0; i<FAMILYKEYBOARD_NUM_BUTTONS; i++)
	{
		key[i].hwState = hwKeyState[i];
	}
}
//*********************************************************************************
int FamilyKeyboardWidget::getKeyAtPoint( QPoint p )
{
	for (int i=0; i<NUM_KEYS; i++)
	{
		if ( key[i].rect.contains(p) )
		{
			return i;
		}
	}
	return -1;
}
//*********************************************************************************
void FamilyKeyboardWidget::mousePressEvent(QMouseEvent * event)
{
	keyPressed = keyUnderMouse = getKeyAtPoint(event->pos());

	if ( (keyPressed >= 0) && (event->button() == Qt::LeftButton) )
	{
		key[ keyPressed ].pressed();
	}
	update();
}
//*********************************************************************************
void FamilyKeyboardWidget::mouseReleaseEvent(QMouseEvent * event)
{
	keyUnderMouse = getKeyAtPoint(event->pos());

	if ( keyPressed >= 0 )
	{
		key[ keyPressed ].released();

		keyPressed = -1;
	}
	update();
}
//*********************************************************************************
void FamilyKeyboardWidget::mouseMoveEvent(QMouseEvent * event)
{
	int tmpKeyUnderMouse = getKeyAtPoint(event->pos());

	//printf("Mouse Move: Key:%i \n", keyUnderMouse );

	if ( tmpKeyUnderMouse != keyUnderMouse )
	{
		keyUnderMouse = tmpKeyUnderMouse;
		update();
	}
}
//*********************************************************************************
void FamilyKeyboardWidget::mouseDoubleClickEvent(QMouseEvent * event)
{
	keyUnderMouse = getKeyAtPoint(event->pos());
}
//*********************************************************************************
void FamilyKeyboardWidget::leaveEvent(QEvent *event)
{
	keyUnderMouse = -1;
	update();
}
//*********************************************************************************
void FamilyKeyboardWidget::contextMenuEvent(QContextMenuEvent *event)
{
	ctxMenuKey = keyUnderMouse = getKeyAtPoint(event->pos());

	if ( keyUnderMouse < 0 )
	{
		return;
	}

	QAction *act;
	QMenu menu(this);

	act = new QAction(tr("Map Key Binding..."), &menu);
	//act->setShortcut( QKeySequence(tr("E")));
	connect( act, SIGNAL(triggered(void)), this, SLOT(ctxMapPhysicalKey(void)) );
	menu.addAction( act );

	act = new QAction(tr("Toggle State on Press"), &menu);
	act->setCheckable(true);
	act->setChecked( key[ ctxMenuKey ].toggleOnPress );
	//act->setShortcut( QKeySequence(tr("E")));
	connect( act, SIGNAL(triggered(void)), this, SLOT(ctxChangeToggleOnPress(void)) );
	menu.addAction( act );

	menu.exec(event->globalPos());

	event->accept();
}
//*********************************************************************************
void FamilyKeyboardWidget::ctxMapPhysicalKey(void)
{
	FKBKeyMapDialog *mapDialog = new FKBKeyMapDialog( ctxMenuKey, this );

	mapDialog->show();
	mapDialog->enterButtonLoop();

	if ( fkbWin )
	{
		fkbWin->updateBindingList();
	}
}
//*********************************************************************************
void FamilyKeyboardWidget::ctxChangeToggleOnPress(void)
{
	key[ ctxMenuKey ].toggleOnPress = !key[ ctxMenuKey ].toggleOnPress;
}
//*********************************************************************************
void FamilyKeyboardWidget::keyPressEvent(QKeyEvent *event)
{
	//printf("Key Press: 0x%x \n", event->key() );
	pushKeyEvent( event, 1 );

	event->accept();
}
//*********************************************************************************
void FamilyKeyboardWidget::keyReleaseEvent(QKeyEvent *event)
{
	//printf("Key Release: 0x%x \n", event->key() );
	pushKeyEvent( event, 0 );

	event->accept();
}
//*********************************************************************************
void FamilyKeyboardWidget::drawButton( QPainter &painter, int idx, int x, int y, int w, int h )
{
	int i = idx, j;
	QColor bgColor;

	key[i].rect = QRect( x, y, w, h );

	if ( key[i].isDown() )
	{
		if ( keyUnderMouse == idx )
		{
			bgColor = QColor( Qt::darkGreen );
		}
		else
		{
			bgColor = QColor( Qt::green );
		}
	}
	else
	{
		if ( keyUnderMouse == idx )
		{
			bgColor = QColor( Qt::gray );
		}
		else
		{
			bgColor = QColor( Qt::lightGray );
		}
	}

	j = i*2;

	if ( key[50].isDown() || key[62].isDown() )
	{
		j++;
	}

	painter.fillRect( key[i].rect, bgColor );
	painter.drawText( key[i].rect, Qt::AlignCenter, tr(keyNames[j]) );
	painter.drawRect( key[i].rect );

}
//*********************************************************************************
void FamilyKeyboardWidget::paintEvent(QPaintEvent *event)
{
	int i, x, y, w, h, xs, ys;
	QPainter painter(this);

	// Row 1
	x = pxBtnGridX / 2;
	y = pxBtnGridY / 2;

	w = pxBtnGridX * 2;
	h = pxBtnGridY;

	xs = w / 4;
	ys = h / 4;

	for (i=0; i<8; i++)
	{
		drawButton( painter, i, x, y, w, h );
		
		x += (w + xs);
	}

	// Row 2
	x  = pxBtnGridX;
	y += pxBtnGridY + ys;

	w = pxBtnGridX;

	xs = w / 4;

	for (i=8; i<21; i++)
	{
		drawButton( painter, i, x, y, w, h );

		x += (w + xs);
	}
	
	drawButton( painter, 21, x, y, w*2, h );

	// Row 3
	x  = pxBtnGridX / 2;
	y += pxBtnGridY + ys;

	w = pxBtnGridX;

	xs = w / 4;

	for (i=22; i<35; i++)
	{
		drawButton( painter, i, x, y, w, h );
		
		x += (w + xs);
	}

	x += (xs);

	drawButton( painter, 35, x, y, w*3, h );

	// Row 4
	x  = pxBtnGridX;
	y += pxBtnGridY + ys;

	w = pxBtnGridX;

	xs = w / 4;
	x -= xs;

	for (i=36; i<49; i++)
	{
		drawButton( painter, i, x, y, w, h );
		
		x += (w + xs);
	}

	drawButton( painter, 49, x, y, w*2, h );

	// Row 5
	x  = pxBtnGridX / 2;
	y += pxBtnGridY + ys;

	w = pxBtnGridX;

	xs = w / 4;
	//x -= xs;

	drawButton( painter, 50, x, y, w*2, h );

	x += ((w*2) + xs);

	for (i=51; i<62; i++)
	{
		drawButton( painter, i, x, y, w, h );
		
		x += (w + xs);
	}

	drawButton( painter, 62, x, y, w*2, h );

	// Row 6
	x  = (pxBtnGridX * 5) / 2;
	y += pxBtnGridY + ys;

	w = pxBtnGridX;

	xs = w / 4;
	x += (2*xs);

	drawButton( painter, 63, x, y, w*2, h );

	x += ((w*2) + xs);

	drawButton( painter, 64, x, y, (w*8) + (xs*7), h );

	// Row 7
	xs =  pxBtnGridX / 4;
	x  = (pxBtnGridX+xs) * 17;
	y  = (pxBtnGridY * 3) / 2;

	w = pxBtnGridX;

	xs = w / 4;
	//x += (2*xs);
	y += (2*ys);

	for (i=65; i<68; i++)
	{
		drawButton( painter, i, x, y, w, h );
		
		x += (w + xs);
	}

	// Arrow Keys
	x  = (pxBtnGridX+xs) * 17 + (pxBtnGridX)/2 + xs;
	y += pxBtnGridY + ys;

	drawButton( painter, 68, x, y, w*2, h );

	x  = (pxBtnGridX+xs) * 17 - (pxBtnGridX)/2 + (xs/2);
	y += pxBtnGridY + ys;

	drawButton( painter, 69, x, y, w*2, h );

	x += ((w*2) + xs);

	drawButton( painter, 70, x, y, w*2, h );

	x  = (pxBtnGridX+xs) * 17 + (pxBtnGridX)/2 + xs;
	y += pxBtnGridY + ys;

	drawButton( painter, 71, x, y, w*2, h );
}
//*********************************************************************************
//----------------------------------------------------------------------------
//--- Family Keyboard Config Dialog
//----------------------------------------------------------------------------
FKBConfigDialog::FKBConfigDialog(QWidget *parent)
	: QDialog(parent)
{
	QVBoxLayout *mainVbox;
	QHBoxLayout *hbox;
	QPushButton *closeButton;
	QTreeWidgetItem *item;
	QMenuBar    *menuBar;
	QSettings    settings;
	char stmp[64];

	setWindowTitle( "Family Keyboard" );

	mainVbox = new QVBoxLayout();
	setLayout( mainVbox );

	menuBar  = buildMenuBar();
	mainVbox->setMenuBar( menuBar );

	keyboard = new FamilyKeyboardWidget();

	mainVbox->addWidget( keyboard );

	keyTree = new QTreeWidget();

	keyTree->setColumnCount(2);
	keyTree->setSelectionMode( QAbstractItemView::SingleSelection );
	keyTree->setAlternatingRowColors(true);

	item = new QTreeWidgetItem();
	item->setText(0, QString::fromStdString("FKB Key"));
	item->setText(1, QString::fromStdString("SDL Binding"));
	item->setTextAlignment(0, Qt::AlignLeft);
	item->setTextAlignment(1, Qt::AlignLeft);

	keyTree->setHeaderItem(item);

	keyTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

	for (int i=0; i<FAMILYKEYBOARD_NUM_BUTTONS; i++)
	{
		int j = i*2;

		item = new QTreeWidgetItem();

		if ( strcmp( keyNames[j], keyNames[j+1] ) == 0 )
		{
			sprintf( stmp, " %s     ", keyNames[j] );
		}
		else
		{
			sprintf( stmp, " %s  -  %s   ", keyNames[j], keyNames[j+1] );
		}

		item->setText(0, tr(stmp) );

		item->setTextAlignment(0, Qt::AlignLeft);
		item->setTextAlignment(1, Qt::AlignLeft);

		keyTree->addTopLevelItem(item);
	}
	updateBindingList();

	mainVbox->addWidget( keyTree );

	hbox = new QHBoxLayout();

	mainVbox->addLayout( hbox );

	statLbl = new QLabel();
	fkbEnaBtn = new QPushButton( tr("Enable") );
	closeButton = new QPushButton( tr("Close") );
	closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	closeButton->setAutoDefault(false);

	hbox->addWidget(fkbEnaBtn, 1);
	hbox->addWidget(statLbl, 3);
	hbox->addStretch(5);
	hbox->addWidget( closeButton, 1);

	connect( keyTree, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(keyTreeItemActivated(QTreeWidgetItem*,int) ) );

	keyTree->setMinimumHeight(0);
	keyTree->setMaximumHeight(0);

	keyTreeHgtAnimation = new QPropertyAnimation( keyTree, "maximumHeight", this);
	keyTreeHgtAnimation->setDuration(500);
	keyTreeHgtAnimation->setStartValue(0);
	keyTreeHgtAnimation->setEndValue(512);
	keyTreeHgtAnimation->setEasingCurve( QEasingCurve::InOutCirc );

	connect( keyTreeHgtAnimation, SIGNAL(valueChanged(const QVariant &)), this, SLOT(keyTreeHeightChange(const QVariant &)));
	connect( keyTreeHgtAnimation, SIGNAL(finished(void)), this, SLOT(keyTreeResizeDone(void)) );

	connect(closeButton  , SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));
	connect(fkbEnaBtn    , SIGNAL(clicked(void)), this, SLOT(toggleFamilyKeyboardEnable(void)) );

	updateTimer = new QTimer(this);

	connect(updateTimer, &QTimer::timeout, this, &FKBConfigDialog::updatePeriodic);

	updateTimer->start(500); // 2hz

	updateStatusLabel();

	// Restore Window Geometry
	restoreGeometry(settings.value("familyKeyboard/geometry").toByteArray());
}
//----------------------------------------------------------------------------
FKBConfigDialog::~FKBConfigDialog(void)
{
	QSettings  settings;

	fkbWin = NULL;

	updateTimer->stop();

	// Save Window Geometry
	settings.setValue("familyKeyboard/geometry", saveGeometry());

	SaveCurrentMapping();
}
//----------------------------------------------------------------------------
void FKBConfigDialog::SaveCurrentMapping(void)
{
	std::string prefix;

	// FamilyKeyBoard
	prefix = "SDL.Input.FamilyKeyBoard.";
	g_config->setOption(prefix + "DeviceType", DefaultFamilyKeyBoardDevice);
	g_config->setOption(prefix + "DeviceNum", 0);
	for(unsigned int j = 0; j < FAMILYKEYBOARD_NUM_BUTTONS; j++)
	{
		g_config->setOption(prefix + FamilyKeyBoardNames[j], fkbmap[j].ButtonNum );
	}
	g_config->save();
}
//----------------------------------------------------------------------------
void FKBConfigDialog::resetDefaultMapping(void)
{
	std::string prefix, device;
	int type = 0, devnum = 0;

	// FamilyKeyBoard
	prefix = "SDL.Input.FamilyKeyBoard.";
	g_config->setOption(prefix + "DeviceType", DefaultFamilyKeyBoardDevice);
	g_config->setOption(prefix + "DeviceNum", 0);

	g_config->getOption(prefix + "DeviceType", &device);

	if (device.find("Keyboard") != std::string::npos)
	{
		type = BUTTC_KEYBOARD;
	}
	else if (device.find("Joystick") != std::string::npos)
	{
		type = BUTTC_JOYSTICK;
	}
	else
	{
		type = 0;
	}
	g_config->getOption(prefix + "DeviceNum", &devnum);

	for(unsigned int j = 0; j < FAMILYKEYBOARD_NUM_BUTTONS; j++)
	{
		g_config->setOption(prefix + FamilyKeyBoardNames[j], DefaultFamilyKeyBoard[j] );

		fkbmap[j].ButtType = type;
		fkbmap[j].DeviceNum = devnum;
		fkbmap[j].ButtonNum = DefaultFamilyKeyBoard[j];
	}
	g_config->save();
}
//----------------------------------------------------------------------------
QMenuBar *FKBConfigDialog::buildMenuBar(void)
{
	QMenu       *fileMenu, *confMenu, *viewMenu;
	//QActionGroup *actGroup;
	QAction     *act;
	int useNativeMenuBar=0;

	QMenuBar *menuBar = new consoleMenuBar(this);

	// This is needed for menu bar to show up on MacOS
	g_config->getOption( "SDL.UseNativeMenuBar", &useNativeMenuBar );

	menuBar->setNativeMenuBar( useNativeMenuBar ? true : false );

	//-----------------------------------------------------------------------
	// Menu Start
	//-----------------------------------------------------------------------
	// File
	fileMenu = menuBar->addMenu(tr("File"));

	// File -> Load
	act = new QAction(tr("Load"), this);
	//act->setShortcut(QKeySequence::Open);
	act->setStatusTip(tr("Load Mapping"));
	connect(act, SIGNAL(triggered()), this, SLOT(mappingLoad(void)) );

	fileMenu->addAction(act);

	// File -> Save
	act = new QAction(tr("Save"), this);
	//act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Save Mapping"));
	connect(act, SIGNAL(triggered()), this, SLOT(mappingSave(void)) );

	fileMenu->addAction(act);

	// File -> Save As
	act = new QAction(tr("Save As"), this);
	//act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Save Mapping As"));
	connect(act, SIGNAL(triggered()), this, SLOT(mappingSaveAs(void)) );

	fileMenu->addAction(act);

	fileMenu->addSeparator();

	// File -> Close
	act = new QAction(tr("Close"), this);
	//act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Close Window"));
	connect(act, SIGNAL(triggered()), this, SLOT(closeWindow(void)) );

	fileMenu->addAction(act);

	// Config
	confMenu = menuBar->addMenu(tr("Config"));

	// Config -> Font
	act = new QAction(tr("Font"), this);
	//act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Choose Font"));
	connect(act, SIGNAL(triggered()), this, SLOT(openFontDialog(void)) );

	confMenu->addAction(act);

	confMenu->addSeparator();

	// Config -> Reset to Defaults
	act = new QAction(tr("Reset to Defaults"), this);
	//act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Reset to Defaults Mappings"));
	connect(act, SIGNAL(triggered()), this, SLOT(resetDefaultMapping(void)) );

	confMenu->addAction(act);

	// View
	viewMenu = menuBar->addMenu(tr("View"));

	// View -> Show Key Binding Tree
	act = new QAction(tr("Show Key Binding Tree"), this);
	act->setCheckable(true);
	act->setChecked(false);
	//act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Show Key Binding Tree"));
	connect(act, SIGNAL(triggered(bool)), this, SLOT(toggleTreeView(bool)) );

	viewMenu->addAction(act);

	// View -> Shrink Window to Minimum Size
	act = new QAction(tr("Shrink Window to Minimum Size"), this);
	//act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Shrink Window to Minimum Size"));
	connect(act, SIGNAL(triggered(void)), this, SLOT(keyTreeResizeDone(void)) );

	viewMenu->addAction(act);

	return menuBar;
}
//*********************************************************************************
void FKBConfigDialog::openFontDialog(void)
{
	bool ok = false;

	QFont selFont = QFontDialog::getFont( &ok, keyboard->QWidget::font(), this, tr("Select Font"), QFontDialog::MonospacedFonts );

	if ( ok )
	{
		keyboard->setFont( selFont );
		keyboard->update();

		//printf("Font Changed to: '%s'\n", font.toString().toStdString().c_str() );

		g_config->setOption("SDL.FamilyKeyboardFont", selFont.toString().toStdString().c_str() );

		QTimer::singleShot( 100, this, SLOT(keyTreeResizeDone(void)) );
	}
}
//----------------------------------------------------------------------------
void FKBConfigDialog::toggleTreeView(bool state)
{
	if ( state )
	{
		keyTreeHgtAnimation->setStartValue(0);
		keyTreeHgtAnimation->setEndValue(512);
		keyTreeHgtAnimation->start();
	}
	else
	{
		keyTree->setMinimumHeight( 0 );
		keyTreeHgtAnimation->setStartValue( keyTree->height() );
		keyTreeHgtAnimation->setEndValue(0);
		keyTreeHgtAnimation->start();
	}
}
//----------------------------------------------------
void FKBConfigDialog::keyTreeHeightChange(const QVariant &value)
{
	int val = value.toInt();

	if ( val > 256 )
	{
		keyTree->setMinimumHeight( 256 );
	}
	else
	{
		keyTree->setMinimumHeight( val );
	}

	resize( minimumSizeHint() );
}
//----------------------------------------------------
void FKBConfigDialog::keyTreeResizeDone(void)
{
	resize( minimumSizeHint() );
}
//----------------------------------------------------------------------------
void FKBConfigDialog::updateBindingList(void)
{
	char keyNameStr[128];

	for (int i=0; i<FAMILYKEYBOARD_NUM_BUTTONS; i++)
	{
		QTreeWidgetItem *item = keyTree->topLevelItem(i);

		//item->setText(0, tr(keyNames[i*2]));

		if (fkbmap[i].ButtType == BUTTC_KEYBOARD)
		{
			snprintf(keyNameStr, sizeof(keyNameStr), "%s",
					 SDL_GetKeyName(fkbmap[i].ButtonNum));
		}
		else
		{
			strcpy(keyNameStr, ButtonName(&fkbmap[i]));
		}
		item->setText(1, tr(keyNameStr));
	}
}
//----------------------------------------------------------------------------
void FKBConfigDialog::updatePeriodic(void)
{
	bool tmpFkbActv = isFamilyKeyboardActv();

	if ( tmpFkbActv != fkbActv )
	{
		fkbActv = tmpFkbActv;

		updateStatusLabel();
	}
}
//----------------------------------------------------------------------------
void FKBConfigDialog::updateStatusLabel(void)
{
	if ( fkbActv )
	{
		statLbl->setText( tr("Family Keyboard is Enabled") );
		fkbEnaBtn->setText( tr("Disable") );
	}
	else
	{
		statLbl->setText( tr("Family Keyboard is Disabled") );
		fkbEnaBtn->setText( tr("Enable") );
	}
}
//----------------------------------------------------------------------------
void FKBConfigDialog::toggleFamilyKeyboardEnable(void)
{
	int curNesInput[3], usrNesInput[3];

	getInputSelection(0, &curNesInput[0], &usrNesInput[0]);
	getInputSelection(1, &curNesInput[1], &usrNesInput[1]);
	getInputSelection(2, &curNesInput[2], &usrNesInput[2]);

	if ( curNesInput[2] != SIFC_FKB )
	{
		ESI port[2];
		ESIFC fcexp;
		int fourscore = false, microphone = false;

		curNesInput[2] = SIFC_FKB;

		g_config->getOption("SDL.FourScore", &fourscore);

		microphone = replaceP2StartWithMicrophone;

		port[0] = (ESI)curNesInput[0];
		port[1] = (ESI)curNesInput[1];
		fcexp = (ESIFC)curNesInput[2];

		FCEUD_SetInput(fourscore, microphone, port[0], port[1], fcexp);

		if ( !isFamilyKeyboardActv() )
		{
			toggleFamilyKeyboardFunc();
		}
	}
	else
	{
		toggleFamilyKeyboardFunc();
	}
	updateStatusLabel();
}
//----------------------------------------------------------------------------
void FKBConfigDialog::closeEvent(QCloseEvent *event)
{
	printf("FKB Config Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void FKBConfigDialog::closeWindow(void)
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void FKBConfigDialog::keyTreeItemActivated(QTreeWidgetItem *item, int column)
{
	int itemIndex = keyTree->indexOfTopLevelItem( item );

	if ( itemIndex < 0 )
	{
		return;
	}
	FKBKeyMapDialog *mapDialog = new FKBKeyMapDialog( itemIndex, this );

	mapDialog->show();
	mapDialog->enterButtonLoop();

	updateBindingList();
}
//----------------------------------------------------------------------------
int FKBConfigDialog::getButtonIndexFromName( const char *buttonName )
{
	for (int j=0; j<FamilyKeyboardWidget::NUM_KEYS; j++)
	{
		if ( strcmp( buttonName, FamilyKeyBoardNames[j] ) == 0 )
		{
			return j;
		}
	}
	return -1;
}
//----------------------------------------------------------------------------
int FKBConfigDialog::convText2ButtConfig( const char *txt, ButtConfig *bmap )
{
	int devIdx = -1;

	bmap->ButtType  = -1;
	bmap->DeviceNum = -1;
	bmap->ButtonNum = -1;

	if (txt[0] == 0 )
	{
		return 0;
	}

	if (txt[0] == 'k')
	{
		SDL_Keycode key;

		bmap->ButtType = BUTTC_KEYBOARD;
		bmap->DeviceNum = -1;

		key = SDL_GetKeyFromName(&txt[1]);

		if (key != SDLK_UNKNOWN)
		{
			bmap->ButtonNum = key;
		}
		else
		{
			bmap->ButtonNum = -1;
		}
	}
	else if ((txt[0] == 'b') && isdigit(txt[1]))
	{
		bmap->ButtType = BUTTC_JOYSTICK;
		bmap->DeviceNum = devIdx;
		bmap->ButtonNum = atoi(&txt[1]);
	}
	else if ((txt[0] == 'h') && isdigit(txt[1]) &&
			 (txt[2] == '.') && isdigit(txt[3]))
	{
		int hatIdx, hatVal;

		hatIdx = txt[1] - '0';
		hatVal = atoi(&txt[3]);

		bmap->ButtType = BUTTC_JOYSTICK;
		bmap->DeviceNum = devIdx;
		bmap->ButtonNum = 0x2000 | ((hatIdx & 0x1F) << 8) | (hatVal & 0xFF);
	}
	else if ((txt[0] == 'a') || (txt[1] == 'a'))
	{
		int l = 0, axisIdx = 0, axisSign = 0;

		l = 0;
		if (txt[l] == '-')
		{
			axisSign = 1;
			l++;
		}
		else if (txt[l] == '+')
		{
			axisSign = 0;
			l++;
		}

		if (txt[l] == 'a')
		{
			l++;
		}
		if (isdigit(txt[l]))
		{
			axisIdx = atoi(&txt[l]);

			while (isdigit(txt[l]))
				l++;
		}
		if (txt[l] == '-')
		{
			axisSign = 1;
			l++;
		}
		else if (txt[l] == '+')
		{
			axisSign = 0;
			l++;
		}
		bmap->ButtType = BUTTC_JOYSTICK;
		bmap->DeviceNum = devIdx;
		bmap->ButtonNum = 0x8000 | (axisSign ? 0x4000 : 0) | (axisIdx & 0xFF);
	}

	return 0;
}
//----------------------------------------------------------------------------
void FKBConfigDialog::mappingLoad(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	char dir[512];
	QFileDialog  dialog(this, tr("Load Family Keyboard Mapping File") );
	QList<QUrl> urls;
	QDir d;

	const QStringList filters(
	{ "FKB Config Files (*.txt *.TXT)",
           "Any files (*)"
         });

	urls << QUrl::fromLocalFile( QDir::rootPath() );
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());
	urls << QUrl::fromLocalFile( QDir( FCEUI_GetBaseDirectory() ).absolutePath() );

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilters( filters );

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Load") );

	strcpy( dir, FCEUI_GetBaseDirectory() );
	strcat( dir, "/input/FamilyKeyboard");

	dialog.setDirectory( tr(dir) );

	// Check config option to use native file dialog or not
	g_config->getOption ("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);
	dialog.setSidebarUrls(urls);

	ret = dialog.exec();

	if ( ret )
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if ( fileList.size() > 0 )
		{
			filename = fileList[0];
		}
	}

	if ( filename.isNull() )
	{
	   return;
	}
	//qDebug() << "selected file path : " << filename.toUtf8();

	mappingLoad( filename.toStdString().c_str() );

	return;
}
//----------------------------------------------------------------------------
void FKBConfigDialog::mappingLoad(const char *filepath)
{
	int i=0,j=0, idx;
	FILE *fp;
	char line[256];
	char id[128];
	char val[128];

	fp = fopen( filepath, "r" );

	if ( fp == NULL )
	{
		printf("Error: Failed to open file '%s' for reading\n", filepath );
		return;
	}

	while (fgets(line, sizeof(line), fp) != 0)
	{
		i = 0;
		while (line[i] != 0)
		{
			if (line[i] == '#')
			{
				line[i] = 0;
				break;
			}
			i++;
		}

		i = 0;
		while (isspace(line[i])) i++;

		if ( line[i] == 0 )
		{
			continue;
		}

		j=0;
		while ((line[i] != 0) && (line[i] != '\n') && (line[i] != '=') && !isspace(line[i]) )
		{
			id[j] = line[i];
			i++;
			j++;
		}
		id[j] = 0;

		while (isspace(line[i])) i++;

		if ( line[i] != '=' )
		{
			printf("Error: Invalid Line\n");
			continue;
		}
		i++;

		while (isspace(line[i])) i++;

		j=0;
		while ((line[i] != 0) && (line[i] != '\n') && !isspace(line[i]) )
		{
			if ( line[i] == '\\' )
			{  // next character should be interpretted literally
				i++;
			}
			val[j] = line[i];
			i++;
			j++;
		}
		val[j] = 0;

		idx = getButtonIndexFromName( id );

		if ( idx < 0 )
		{
			continue;
		}

		convText2ButtConfig( val, &fkbmap[idx] );
	}

	fclose(fp);
}
//----------------------------------------------------------------------------
void FKBConfigDialog::mappingSave(void)
{
	const char *guid = "keyboard";
	const char *baseDir = FCEUI_GetBaseDirectory();
	std::string path;
	FILE *fp;
	char stmp[64];
	QDir dir;

	path = std::string(baseDir) + std::string("/input/FamilyKeyboard/");

	dir.mkpath(QString::fromStdString(path));

	if ( saveFileName.size() == 0 )
	{
		mappingSaveAs();
		return;
	}
	path = saveFileName;

	fp = fopen( path.c_str(), "w" );

	if ( fp == NULL )
	{
		printf("Error: Failed to open file '%s' for writing\n", path.c_str() );
		return;
	}

	fprintf( fp, "# Family Keyboard Button Mapping File for Fceux (Qt version)\n");
	fprintf( fp, "GUID=%s\n", guid );

	for (unsigned int i=0; i<FamilyKeyboardWidget::NUM_KEYS; i++)
	{
		if ( fkbmap[i].ButtType == BUTTC_KEYBOARD)
		{
			int j=0,k=0; const char *keyName;

			keyName = SDL_GetKeyName(fkbmap[i].ButtonNum);

			stmp[k] = 'k'; k++;

			// Write keyname in with necessary escape characters.
			while ( keyName[j] != 0 )
			{
				if ( (keyName[j] == '\\') || (keyName[j] == ',') )
				{
					stmp[k] = '\\'; k++;
				}
				stmp[k] = keyName[j]; k++; j++;
			}
			stmp[k] = 0;

			//sprintf(stmp, "k%s", SDL_GetKeyName(bmap[c][i].ButtonNum));
		}
		else
		{
			if (fkbmap[i].ButtonNum & 0x2000)
			{
				/* Hat "button" */
				sprintf(stmp, "h%i.%i",
						(fkbmap[i].ButtonNum >> 8) & 0x1F, fkbmap[i].ButtonNum & 0xFF);
			}
			else if (fkbmap[i].ButtonNum & 0x8000)
			{
				/* Axis "button" */
				sprintf(stmp, "%ca%i",
						(fkbmap[i].ButtonNum & 0x4000) ? '-' : '+', fkbmap[i].ButtonNum & 0x3FFF);
			}
			else
			{
				/* Button */
				sprintf(stmp, "b%i", fkbmap[i].ButtonNum);
			}
		}
		fprintf( fp, "%s=%s\n", FamilyKeyBoardNames[i], stmp );

	}

	fclose(fp);
}
//----------------------------------------------------------------------------
void FKBConfigDialog::mappingSaveAs(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	const char *base;
	QFileDialog  dialog(this, tr("Save Mapping To File") );
	QList<QUrl> urls;
	QDir d;

	base = FCEUI_GetBaseDirectory();

	urls << QUrl::fromLocalFile( QDir::rootPath() );
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());

	if ( base )
	{
		urls << QUrl::fromLocalFile( QDir( base ).absolutePath() );

		d.setPath( QString(base) + "/input/FamilyKeyboard");
		d.mkpath( QString(base) + "/input/FamilyKeyboard");

		if ( d.exists() )
		{
			urls << QUrl::fromLocalFile( d.absolutePath() );
		}
	}

	dialog.setFileMode(QFileDialog::AnyFile);

	dialog.setNameFilter(tr("FKB Config Files (*.txt *.TXT) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Save") );
	dialog.setDefaultSuffix( tr(".txt") );

	if ( last.size() == 0 )
	{
		if ( base )
		{
			last = std::string(base) + "/input/FamilyKeyboard";
			dialog.setDirectory( last.c_str() );
		}
	}

	// Check config option to use native file dialog or not
	g_config->getOption ("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);
	dialog.setSidebarUrls(urls);

	ret = dialog.exec();

	if ( ret )
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if ( fileList.size() > 0 )
		{
			filename = fileList[0];
		}
	}

	if ( filename.isNull() )
	{
	   return;
	}

	saveFileName = filename.toStdString();

	mappingSave();
}
//----------------------------------------------------------------------------
//*********************************************************************************
//******  Key Mapping Window
//*********************************************************************************
FKBKeyMapDialog::FKBKeyMapDialog( int idx, QWidget *parent )
	: QDialog( parent )
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QPushButton *cancelButton;
	QLabel      *lbl;
	char keyMapName[64];
	char stmp[128];

	setWindowTitle( tr("Family Keyboard Key Mapping") );
	setModal(true);

	keyIdx = idx;
	buttonConfigStatus = 0;

	mainLayout = new QVBoxLayout();
	hbox       = new QHBoxLayout();

	setLayout( mainLayout );

	sprintf( stmp, "Press a key to set new physical mapping for the '%s' Key", keyNames[idx*2] );

	msgLbl = new QLabel( tr(stmp) );

	if (fkbmap[ keyIdx ].ButtType == BUTTC_KEYBOARD)
	{
		snprintf(keyMapName, sizeof(keyMapName), "%s",
				 SDL_GetKeyName(fkbmap[ keyIdx ].ButtonNum));
	}
	else
	{
		strcpy(keyMapName, ButtonName(&fkbmap[ keyIdx ]));
	}

	lbl       = new QLabel( tr("Current Mapping is:") );
	curMapLbl = new QLabel( tr(keyMapName) );

	hbox->addWidget( lbl );
	hbox->addWidget( curMapLbl );

	cancelButton = new QPushButton( tr("Cancel") );
	cancelButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
	cancelButton->setAutoDefault(false);

	mainLayout->addWidget( msgLbl );
	mainLayout->addLayout( hbox );

	hbox       = new QHBoxLayout();
	mainLayout->addLayout( hbox );
	hbox->addWidget( cancelButton, 1 );
	hbox->addStretch( 4 );

	connect( cancelButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)) );
}
//*********************************************************************************
FKBKeyMapDialog::~FKBKeyMapDialog(void)
{
	buttonConfigStatus = 0;
}
//*********************************************************************************
void FKBKeyMapDialog::enterButtonLoop(void)
{
	buttonConfigStatus = 2;

	ButtonConfigBegin();

	DWaitButton(NULL, &fkbmap[ keyIdx ], &buttonConfigStatus);

	ButtonConfigEnd();

	buttonConfigStatus = 1;

	done(0);
	deleteLater();
}
//*********************************************************************************
void FKBKeyMapDialog::closeEvent(QCloseEvent *event)
{
	printf("FKB Key Map Close Window Event\n");
	buttonConfigStatus = 0;
	done(0);
	deleteLater();
	event->accept();
}
//*********************************************************************************
void FKBKeyMapDialog::closeWindow(void)
{
	printf("Close Window\n");
	buttonConfigStatus = 0;
	done(0);
	deleteLater();
}
//*********************************************************************************
void FKBKeyMapDialog::keyPressEvent(QKeyEvent *event)
{
	//printf("Key Press: 0x%x \n", event->key() );
	pushKeyEvent( event, 1 );

	event->accept();
}
//*********************************************************************************
void FKBKeyMapDialog::keyReleaseEvent(QKeyEvent *event)
{
	//printf("Key Release: 0x%x \n", event->key() );
	pushKeyEvent( event, 0 );

	event->accept();
}
//*********************************************************************************
