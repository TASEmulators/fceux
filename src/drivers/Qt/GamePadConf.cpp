// GamePadConf.cpp
//
#include "Qt/GamePadConf.h"
#include "Qt/main.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"

//----------------------------------------------------
GamePadConfDialog_t::GamePadConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QHBoxLayout *hbox1, *hbox2;
	QGridLayout *grid;
	QCheckBox *efs_chkbox, *udlr_chkbox;
   QGroupBox *frame;
   QPushButton *loadDefaultButton;
   QPushButton *clearAllButton;
   QPushButton *closebutton;
   QPushButton *clearButton[GAMEPAD_NUM_BUTTONS];

   portNum = 0;
   configNo = 0;
   buttonConfigStatus = 1;

   setWindowTitle("GamePad Config");

	hbox1 = new QHBoxLayout();
	hbox2 = new QHBoxLayout();

	QLabel *label = new QLabel(tr("Port:"));
	portSel = new QComboBox();
	hbox1->addWidget( label );
	hbox1->addWidget( portSel );

   portSel->addItem( tr("1"), 0 );
   portSel->addItem( tr("2"), 1 );
   portSel->addItem( tr("3"), 2 );
   portSel->addItem( tr("4"), 3 );

	efs_chkbox  = new QCheckBox("Enable Four Score");
	udlr_chkbox = new QCheckBox("Allow Up+Down/Left+Right");

	int fourScore;
	g_config->getOption("SDL.FourScore", &fourScore);
	efs_chkbox->setChecked( fourScore );

	int opposite_dirs;
	g_config->getOption("SDL.Input.EnableOppositeDirectionals", &opposite_dirs);
	udlr_chkbox->setChecked( opposite_dirs );

   frame = new QGroupBox(tr("Buttons:"));
   grid  = new QGridLayout();

   grid-> setHorizontalSpacing(50);

   //frame->setFrameStyle( QFrame::Box );
   frame->setLayout( grid );

   for (int i=0; i<GAMEPAD_NUM_BUTTONS; i++)
   {
      char text[64];
	   QLabel *buttonName;

      sprintf( text, "%s:", GamePadNames[i] );

	   //hbox2 = new QHBoxLayout();

      //hbox2->setAlignment(Qt::AlignCenter);

	   buttonName     = new QLabel(tr(text));
	   keyName[i]     = new QLabel();
      button[i]      = new GamePadConfigButton_t(i);
      clearButton[i] = new QPushButton("Clear");

      grid->addWidget( buttonName    , i, 0, Qt::AlignCenter );
      grid->addWidget( keyName[i]    , i, 1, Qt::AlignCenter );
      grid->addWidget( button[i]     , i, 2, Qt::AlignCenter );
      grid->addWidget( clearButton[i], i, 3, Qt::AlignCenter );
   }
	updateCntrlrDpy();

   loadDefaultButton  = new QPushButton(tr("Load Defaults"));
   clearAllButton     = new QPushButton(tr("Clear All"));
   closebutton        = new QPushButton(tr("Close"));

	hbox2->addWidget( loadDefaultButton );
	hbox2->addWidget( clearAllButton    );
	hbox2->addWidget( closebutton       );

   connect(button[0], SIGNAL(clicked()), this, SLOT(changeButton0(void)) );
   connect(button[1], SIGNAL(clicked()), this, SLOT(changeButton1(void)) );
   connect(button[2], SIGNAL(clicked()), this, SLOT(changeButton2(void)) );
   connect(button[3], SIGNAL(clicked()), this, SLOT(changeButton3(void)) );
   connect(button[4], SIGNAL(clicked()), this, SLOT(changeButton4(void)) );
   connect(button[5], SIGNAL(clicked()), this, SLOT(changeButton5(void)) );
   connect(button[6], SIGNAL(clicked()), this, SLOT(changeButton6(void)) );
   connect(button[7], SIGNAL(clicked()), this, SLOT(changeButton7(void)) );
   connect(button[8], SIGNAL(clicked()), this, SLOT(changeButton8(void)) );
   connect(button[9], SIGNAL(clicked()), this, SLOT(changeButton9(void)) );

	connect(clearButton[0], SIGNAL(clicked()), this, SLOT(clearButton0(void)) );
   connect(clearButton[1], SIGNAL(clicked()), this, SLOT(clearButton1(void)) );
   connect(clearButton[2], SIGNAL(clicked()), this, SLOT(clearButton2(void)) );
   connect(clearButton[3], SIGNAL(clicked()), this, SLOT(clearButton3(void)) );
   connect(clearButton[4], SIGNAL(clicked()), this, SLOT(clearButton4(void)) );
   connect(clearButton[5], SIGNAL(clicked()), this, SLOT(clearButton5(void)) );
   connect(clearButton[6], SIGNAL(clicked()), this, SLOT(clearButton6(void)) );
   connect(clearButton[7], SIGNAL(clicked()), this, SLOT(clearButton7(void)) );
   connect(clearButton[8], SIGNAL(clicked()), this, SLOT(clearButton8(void)) );
   connect(clearButton[9], SIGNAL(clicked()), this, SLOT(clearButton9(void)) );

   connect(loadDefaultButton, SIGNAL(clicked()), this, SLOT(loadDefaults(void)) );
   connect(clearAllButton   , SIGNAL(clicked()), this, SLOT(clearAllCallback(void)) );
   connect(closebutton      , SIGNAL(clicked()), this, SLOT(closeWindow(void)) );

   connect(portSel    , SIGNAL(activated(int)), this, SLOT(controllerSelect(int)) );
   connect(efs_chkbox , SIGNAL(stateChanged(int)), this, SLOT(ena4score(int)) );
   connect(udlr_chkbox, SIGNAL(stateChanged(int)), this, SLOT(oppDirEna(int)) );

	QVBoxLayout *mainLayout = new QVBoxLayout();

	mainLayout->addLayout( hbox1 );
	mainLayout->addWidget( efs_chkbox );
	mainLayout->addWidget( udlr_chkbox );
	mainLayout->addWidget( frame );
	mainLayout->addLayout( hbox2 );

	setLayout( mainLayout );

}

//----------------------------------------------------
GamePadConfDialog_t::~GamePadConfDialog_t(void)
{
   buttonConfigStatus = 0;
}
void GamePadConfDialog_t::keyPressEvent(QKeyEvent *event)
{
   //printf("GamePad Window Key Press: 0x%x \n", event->key() );
	pushKeyEvent( event, 1 );
}

void GamePadConfDialog_t::keyReleaseEvent(QKeyEvent *event)
{
   //printf("GamePad Window Key Release: 0x%x \n", event->key() );
	pushKeyEvent( event, 0 );
}
//----------------------------------------------------
void GamePadConfDialog_t::updateCntrlrDpy(void)
{
   char keyNameStr[128];

	for (int i=0; i<GAMEPAD_NUM_BUTTONS; i++)
	{
		if (GamePadConfig[portNum][i].ButtType[configNo] == BUTTC_KEYBOARD)
		{
			snprintf( keyNameStr, sizeof (keyNameStr), "%s",
				  SDL_GetKeyName (GamePadConfig[portNum][i].ButtonNum[configNo]));
		}
		else
		{
			strcpy( keyNameStr, ButtonName( &GamePadConfig[portNum][i], configNo ) );
		}
		keyName[i]->setText( tr(keyNameStr) );
	}
}
//----------------------------------------------------
void GamePadConfDialog_t::controllerSelect(int index)
{
	//printf("Port Number:%i \n", index);
	portNum = index;
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
   char buf[256];
   std::string prefix;
   const char *keyNameStr;

   if ( buttonConfigStatus == 2 )
   {
      buttonConfigStatus = 0;
      return;
   }
   buttonConfigStatus = 2;

   ButtonConfigBegin ();

   button[x]->setText("Waiting" );

   snprintf (buf, sizeof(buf)-1, "SDL.Input.GamePad.%d.", padNo);
	prefix = buf;
	DWaitButton (NULL, &GamePadConfig[padNo][x], configNo, &buttonConfigStatus );

   g_config->setOption (prefix + GamePadNames[x],
			     GamePadConfig[padNo][x].ButtonNum[configNo]);

   if (GamePadConfig[padNo][x].ButtType[configNo] == BUTTC_KEYBOARD)
	{
		g_config->setOption (prefix + "DeviceType", "Keyboard");
	}
	else if (GamePadConfig[padNo][x].ButtType[configNo] == BUTTC_JOYSTICK)
	{
		g_config->setOption (prefix + "DeviceType", "Joystick");
	}
	else
	{
		g_config->setOption (prefix + "DeviceType", "Unknown");
	}
	g_config->setOption (prefix + "DeviceNum",
			     GamePadConfig[padNo][x].DeviceNum[configNo]);

   keyNameStr = ButtonName( &GamePadConfig[padNo][x], configNo );

   keyName[x]->setText( keyNameStr );
   button[x]->setText("Change");

   ButtonConfigEnd ();

   buttonConfigStatus = 1;
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton( int padNo, int x )
{
   char buf[256];
   std::string prefix;

	GamePadConfig[padNo][x].ButtonNum[configNo] = -1;

   keyName[x]->setText("");

   snprintf (buf, sizeof(buf)-1, "SDL.Input.GamePad.%d.", padNo);
	prefix = buf;

   g_config->setOption (prefix + GamePadNames[x],
			     GamePadConfig[padNo][x].ButtonNum[configNo]);

}
//----------------------------------------------------
void GamePadConfDialog_t::closeEvent(QCloseEvent *event)
{
   //printf("GamePad Close Window Event\n");
   buttonConfigStatus = 0;
   done(0);
   event->accept();
}
//----------------------------------------------------
void GamePadConfDialog_t::closeWindow(void)
{
   //printf("Close Window\n");
   buttonConfigStatus = 0;
   done(0);
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton0(void)
{
   changeButton( portNum, 0 );
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton1(void)
{
   changeButton( portNum, 1 );
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton2(void)
{
   changeButton( portNum, 2 );
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton3(void)
{
   changeButton( portNum, 3 );
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton4(void)
{
   changeButton( portNum, 4 );
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton5(void)
{
   changeButton( portNum, 5 );
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton6(void)
{
   changeButton( portNum, 6 );
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton7(void)
{
   changeButton( portNum, 7 );
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton8(void)
{
   changeButton( portNum, 8 );
}
//----------------------------------------------------
void GamePadConfDialog_t::changeButton9(void)
{
   changeButton( portNum, 9 );
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton0(void)
{
   clearButton( portNum, 0 );
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton1(void)
{
   clearButton( portNum, 1 );
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton2(void)
{
   clearButton( portNum, 2 );
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton3(void)
{
   clearButton( portNum, 3 );
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton4(void)
{
   clearButton( portNum, 4 );
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton5(void)
{
   clearButton( portNum, 5 );
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton6(void)
{
   clearButton( portNum, 6 );
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton7(void)
{
   clearButton( portNum, 7 );
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton8(void)
{
   clearButton( portNum, 8 );
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton9(void)
{
   clearButton( portNum, 9 );
}
//----------------------------------------------------
void GamePadConfDialog_t::clearAllCallback(void)
{
	for (int i=0; i<GAMEPAD_NUM_BUTTONS; i++)
	{
   	clearButton( portNum, i );
	}
}
//----------------------------------------------------
void GamePadConfDialog_t::loadDefaults(void)
{
   char buf[256];
   std::string prefix;

	if ( portNum > 0 )
	{
		clearAllCallback();
		return;
	}

   snprintf (buf, sizeof(buf)-1, "SDL.Input.GamePad.%d.", portNum);
	prefix = buf;

	for (int x=0; x<GAMEPAD_NUM_BUTTONS; x++)
	{
		GamePadConfig[portNum][x].ButtType[configNo]  = BUTTC_KEYBOARD;
		GamePadConfig[portNum][x].DeviceNum[configNo] = 0;
		GamePadConfig[portNum][x].ButtonNum[configNo] = DefaultGamePad[portNum][x];
		GamePadConfig[portNum][x].NumC = 1;

		g_config->setOption (prefix + GamePadNames[x],
			     GamePadConfig[portNum][x].ButtonNum[configNo]);

	   if (GamePadConfig[portNum][x].ButtType[configNo] == BUTTC_KEYBOARD)
		{
			g_config->setOption (prefix + "DeviceType", "Keyboard");
		}
		else if (GamePadConfig[portNum][x].ButtType[configNo] == BUTTC_JOYSTICK)
		{
			g_config->setOption (prefix + "DeviceType", "Joystick");
		}
		else
		{
			g_config->setOption (prefix + "DeviceType", "Unknown");
		}
		g_config->setOption (prefix + "DeviceNum",
				     GamePadConfig[portNum][x].DeviceNum[configNo]);
	}
	updateCntrlrDpy();
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
	pushKeyEvent( event, 1 );
}

void GamePadConfigButton_t::keyReleaseEvent(QKeyEvent *event)
{
   //printf("GamePad Button Key Release: 0x%x \n", event->key() );
	pushKeyEvent( event, 0 );
}
//----------------------------------------------------
