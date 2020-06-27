// GamePadConf.cpp
//
#include "GamePadConf.h"
#include "main.h"
#include "input.h"
#include "config.h"
#include "keyscan.h"
#include "fceuWrapper.h"

//----------------------------------------------------
GamePadConfDialog_t::GamePadConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QHBoxLayout *hbox1;
	QGridLayout *grid;
	QCheckBox *efs_chkbox, *udlr_chkbox;
   QGroupBox *frame;
   QPushButton *closebutton;

   portNum = 0;
   buttonConfigStatus = 1;

   setWindowTitle("GamePad Config");

	hbox1 = new QHBoxLayout();

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

   for (int i=0; i<10; i++)
   {
      char text[64];
      const char *keyNameStr;
	   QLabel *buttonName;

      sprintf( text, "%s:", GamePadNames[i] );

	   //hbox2 = new QHBoxLayout();

      //hbox2->setAlignment(Qt::AlignCenter);

      keyNameStr = ButtonName( &GamePadConfig[portNum][i], portNum );

	   buttonName = new QLabel(tr(text));
	   keyName[i] = new QLabel(tr(keyNameStr));
      button[i]  = new GamePadConfigButton_t(i);

      grid->addWidget( buttonName, i, 0, Qt::AlignCenter );
      grid->addWidget( keyName[i], i, 1, Qt::AlignCenter );
      grid->addWidget( button[i] , i, 2, Qt::AlignCenter );
   }
   closebutton     = new QPushButton(tr("Close"));

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
   connect(closebutton, SIGNAL(clicked()), this, SLOT(closeWindow(void)) );

   connect(efs_chkbox , SIGNAL(stateChanged(int)), this, SLOT(ena4score(int)) );
   connect(udlr_chkbox, SIGNAL(stateChanged(int)), this, SLOT(oppDirEna(int)) );

	QVBoxLayout *mainLayout = new QVBoxLayout();

	mainLayout->addLayout( hbox1 );
	mainLayout->addWidget( efs_chkbox );
	mainLayout->addWidget( udlr_chkbox );
	mainLayout->addWidget( frame );
	mainLayout->addWidget( closebutton, Qt::AlignRight );

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
   int configNo = 0;
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

   if (GamePadConfig[padNo][x].ButtType[0] == BUTTC_KEYBOARD)
	{
		g_config->setOption (prefix + "DeviceType", "Keyboard");
	}
	else if (GamePadConfig[padNo][x].ButtType[0] == BUTTC_JOYSTICK)
	{
		g_config->setOption (prefix + "DeviceType", "Joystick");
	}
	else
	{
		g_config->setOption (prefix + "DeviceType", "Unknown");
	}
	g_config->setOption (prefix + "DeviceNum",
			     GamePadConfig[padNo][x].DeviceNum[configNo]);

   keyNameStr = ButtonName( &GamePadConfig[padNo][x], padNo );

   keyName[x]->setText( keyNameStr );
   button[x]->setText("Change");

   ButtonConfigEnd ();

   buttonConfigStatus = 1;
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
