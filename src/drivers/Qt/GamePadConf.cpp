// GamePadConf.cpp
//
#include <QDir>
#include <QInputDialog>

#include "Qt/GamePadConf.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/sdl-joystick.h"
#include "Qt/fceuWrapper.h"

//----------------------------------------------------
GamePadConfDialog_t::GamePadConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QHBoxLayout *hbox, *hbox1, *hbox2, *hbox3, *hbox4;
	QVBoxLayout *vbox;
	QGridLayout *grid;
	QCheckBox *efs_chkbox, *udlr_chkbox;
   QGroupBox *frame1, *frame2;
	QLabel *label;
   QPushButton *newProfileButton;
   QPushButton *saveProfileButton;
   QPushButton *applyProfileButton;
   QPushButton *removeProfileButton;
   QPushButton *loadDefaultButton;
   QPushButton *clearAllButton;
   QPushButton *closebutton;
   QPushButton *clearButton[GAMEPAD_NUM_BUTTONS];

	InitJoysticks();

   portNum = 0;
   buttonConfigStatus = 1;

   inputTimer  = new QTimer( this );

   connect( inputTimer, &QTimer::timeout, this, &GamePadConfDialog_t::updatePeriodic );

   setWindowTitle( tr("GamePad Config") );

	hbox1 = new QHBoxLayout();
	hbox2 = new QHBoxLayout();
	hbox3 = new QHBoxLayout();
	hbox4 = new QHBoxLayout();

	label = new QLabel(tr("Console Port:"));
	portSel = new QComboBox();
	hbox1->addWidget( label );
	hbox1->addWidget( portSel );

   portSel->addItem( tr("1"), 0 );
   portSel->addItem( tr("2"), 1 );
   portSel->addItem( tr("3"), 2 );
   portSel->addItem( tr("4"), 3 );

	label = new QLabel(tr("Device:"));
	devSel = new QComboBox();
	hbox2->addWidget( label );
	hbox2->addWidget( devSel );

   devSel->addItem( tr("Keyboard"), -1 );

	for (int i=0; i<MAX_JOYSTICKS; i++)
	{
		jsDev_t *js = getJoystickDevice( i );

		if ( js != NULL )
		{
			if ( js->isConnected() )
			{
				char stmp[128];
				sprintf( stmp, "%i: %s", i, js->getName() );
   			devSel->addItem( tr(stmp), i );
			}
		}
	}

	label = new QLabel(tr("GUID:"));
	guidLbl = new QLabel();

	hbox3->addWidget( label );
	hbox3->addWidget( guidLbl );

   frame1 = new QGroupBox(tr("Mapping Profile:"));
   //grid   = new QGridLayout();
   vbox   = new QVBoxLayout();

   //frame1->setLayout( grid );
   frame1->setLayout( vbox );

	hbox = new QHBoxLayout();
	vbox->addLayout( hbox );

	mapSel = new QComboBox();
   hbox->addWidget( mapSel );

   mapSel->setWhatsThis( tr("Combo box for selection of a saved button mapping profile for the selected device"));
   mapSel->addItem( tr("default"), 0 );

	hbox = new QHBoxLayout();
	vbox->addLayout( hbox );

   applyProfileButton = new QPushButton( tr("Load") );
	applyProfileButton->setWhatsThis(tr("Sets Current Active Map to the Selected Profile"));
   hbox->addWidget( applyProfileButton );

   saveProfileButton = new QPushButton( tr("Save") );
	saveProfileButton->setWhatsThis(tr("Stores Current Active Map to the Selected Profile"));
   hbox->addWidget( saveProfileButton );

	hbox = new QHBoxLayout();
	vbox->addLayout( hbox );

   newProfileButton   = new QPushButton( tr("New") );
	newProfileButton->setWhatsThis(tr("Create a New Map Profile"));
   hbox->addWidget( newProfileButton );

   removeProfileButton   = new QPushButton( tr("Delete") );
	removeProfileButton->setWhatsThis(tr("Deletes the Selected Map Profile"));
   hbox->addWidget( removeProfileButton );

   mapMsg = new QLabel();
   vbox->addWidget(mapMsg);

	efs_chkbox  = new QCheckBox( tr("Enable Four Score") );
	udlr_chkbox = new QCheckBox( tr("Allow Up+Down/Left+Right") );

	int fourScore;
	g_config->getOption("SDL.FourScore", &fourScore);
	efs_chkbox->setChecked( fourScore );

	int opposite_dirs;
	g_config->getOption("SDL.Input.EnableOppositeDirectionals", &opposite_dirs);
	udlr_chkbox->setChecked( opposite_dirs );

   frame2 = new QGroupBox(tr("Current Active Button Mappings:"));
   grid   = new QGridLayout();

   grid-> setHorizontalSpacing(50);

   frame2->setLayout( grid );

   for (int i=0; i<GAMEPAD_NUM_BUTTONS; i++)
   {
      char text[64];
	   QLabel *buttonName;

      sprintf( text, "%s:", GamePadNames[i] );

	   //hbox2 = new QHBoxLayout();

      //hbox2->setAlignment(Qt::AlignCenter);

	   buttonName     = new QLabel(tr(text));
	   keyName[i]     = new QLabel();
	   keyState[i]    = new QLabel( tr("F") );
	   label          = new QLabel( tr("State:") );
      button[i]      = new GamePadConfigButton_t(i);
      clearButton[i] = new QPushButton( tr("Clear") );

      grid->addWidget( buttonName    , i, 0, Qt::AlignCenter );
      grid->addWidget( keyName[i]    , i, 1, Qt::AlignCenter );
      grid->addWidget( label         , i, 2, Qt::AlignCenter );
      grid->addWidget( keyState[i]   , i, 3, Qt::AlignCenter );
      grid->addWidget( button[i]     , i, 4, Qt::AlignCenter );
      grid->addWidget( clearButton[i], i, 5, Qt::AlignCenter );
   }
	updateCntrlrDpy();

   loadDefaultButton  = new QPushButton(tr("Load Defaults"));
   clearAllButton     = new QPushButton(tr("Clear All"));
   closebutton        = new QPushButton(tr("Close"));

	hbox4->addWidget( loadDefaultButton );
	hbox4->addWidget( clearAllButton    );
	hbox4->addWidget( closebutton       );

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

   connect(newProfileButton   , SIGNAL(clicked()), this, SLOT(newProfileCallback(void)) );
   connect(applyProfileButton , SIGNAL(clicked()), this, SLOT(loadProfileCallback(void)) );
   connect(saveProfileButton  , SIGNAL(clicked()), this, SLOT(saveProfileCallback(void)) );
   connect(removeProfileButton, SIGNAL(clicked()), this, SLOT(deleteProfileCallback(void)) );

   connect(loadDefaultButton, SIGNAL(clicked()), this, SLOT(loadDefaults(void)) );
   connect(clearAllButton   , SIGNAL(clicked()), this, SLOT(clearAllCallback(void)) );
   connect(closebutton      , SIGNAL(clicked()), this, SLOT(closeWindow(void)) );

   connect(portSel    , SIGNAL(activated(int)), this, SLOT(portSelect(int)) );
   connect(devSel     , SIGNAL(activated(int)), this, SLOT(deviceSelect(int)) );
   connect(efs_chkbox , SIGNAL(stateChanged(int)), this, SLOT(ena4score(int)) );
   connect(udlr_chkbox, SIGNAL(stateChanged(int)), this, SLOT(oppDirEna(int)) );

	QVBoxLayout *mainLayout = new QVBoxLayout();

	mainLayout->addLayout( hbox1 );
	mainLayout->addLayout( hbox2 );
	mainLayout->addLayout( hbox3 );
	mainLayout->addWidget( frame1 );
	mainLayout->addWidget( efs_chkbox );
	mainLayout->addWidget( udlr_chkbox );
	mainLayout->addWidget( frame2 );
	mainLayout->addLayout( hbox4 );

	setLayout( mainLayout );

   inputTimer->start( 33 ); // 30hz

   loadMapList();
}

//----------------------------------------------------
GamePadConfDialog_t::~GamePadConfDialog_t(void)
{
   inputTimer->stop();
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
void GamePadConfDialog_t::loadMapList(void)
{
   QDir dir;
   QStringList filters, fileList;
   const char *baseDir = FCEUI_GetBaseDirectory();
   const char *guid;
   std::string path;
   int index, devIdx;
   jsDev_t *js;

	index  = devSel->currentIndex();
	devIdx = devSel->itemData(index).toInt();

   if ( devIdx < 0 )
   {
      guid = "keyboard";
   }
   else
   {
      js = getJoystickDevice( devIdx );

      guid = js->getGUID();
   }

   if ( guid == NULL )
   {
      return;
   }

   path = std::string(baseDir) + "/input/" + std::string(guid);

   dir.setPath( QString::fromStdString(path) );

   filters << "*.txt";
   dir.setNameFilters(filters);
   
   fileList = dir.entryList( filters, QDir::Files, QDir::NoSort );

   mapSel->clear();
   mapSel->addItem( tr("default"), 0 );

   for (size_t i=0; i < fileList.size(); i++)
   {
      size_t suffixIdx;
      std::string fileName = fileList[i].toStdString();

      suffixIdx = fileName.find_last_of('.');

      fileName.erase( suffixIdx );

      //printf("File: %s \n", fileName.c_str() );
      //

      if ( fileName.compare("default") == 0 ) continue;

      mapSel->addItem( tr(fileName.c_str()), (int)i+1 );
   }
}
//----------------------------------------------------
void GamePadConfDialog_t::updateCntrlrDpy(void)
{
   char keyNameStr[128];

	for (int i=0; i<GAMEPAD_NUM_BUTTONS; i++)
	{
		if (GamePad[portNum].bmap[i].ButtType == BUTTC_KEYBOARD)
		{
			snprintf( keyNameStr, sizeof (keyNameStr), "%s",
				  SDL_GetKeyName (GamePad[portNum].bmap[i].ButtonNum));
		}
		else
		{
			strcpy( keyNameStr, ButtonName( &GamePad[portNum].bmap[i] ) );
		}
		keyName[i]->setText( tr(keyNameStr) );
	}
}
//----------------------------------------------------
void GamePadConfDialog_t::portSelect(int index)
{
	//printf("Port Number:%i \n", index);
	portNum = index;
	updateCntrlrDpy();
}
//----------------------------------------------------
void GamePadConfDialog_t::deviceSelect(int index)
{
	jsDev_t *js;
	int devIdx = devSel->itemData(index).toInt();

	js = getJoystickDevice( devIdx );

	if ( js != NULL )
	{
		if ( js->isConnected() )
		{
			guidLbl->setText( js->getGUID() );
		}
	}
	else
	{
		guidLbl->setText("");
	}
	GamePad[portNum].setDeviceIndex( devIdx );

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
	DWaitButton (NULL, &GamePad[padNo].bmap[x], &buttonConfigStatus );

   g_config->setOption (prefix + GamePadNames[x],
			     GamePad[padNo].bmap[x].ButtonNum);

   if (GamePad[padNo].bmap[x].ButtType == BUTTC_KEYBOARD)
	{
		g_config->setOption (prefix + "DeviceType", "Keyboard");
	}
	else if (GamePad[padNo].bmap[x].ButtType == BUTTC_JOYSTICK)
	{
		g_config->setOption (prefix + "DeviceType", "Joystick");
	}
	else
	{
		g_config->setOption (prefix + "DeviceType", "Unknown");
	}
	g_config->setOption (prefix + "DeviceNum",
			     GamePad[padNo].bmap[x].DeviceNum);

   keyNameStr = ButtonName( &GamePad[padNo].bmap[x] );

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

	GamePad[padNo].bmap[x].ButtonNum = -1;

   keyName[x]->setText("");

   snprintf (buf, sizeof(buf)-1, "SDL.Input.GamePad.%d.", padNo);
	prefix = buf;

   g_config->setOption (prefix + GamePadNames[x],
			     GamePad[padNo].bmap[x].ButtonNum);

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
	int index, devIdx;
   char buf[256];
   std::string prefix;

	index  = devSel->currentIndex();
	devIdx = devSel->itemData(index).toInt();

	//printf("Selected Device:%i : %i \n", index, devIdx );

	if ( devIdx == -1 )
	{
   	snprintf (buf, sizeof(buf)-1, "SDL.Input.GamePad.%d.", portNum);
		prefix = buf;

		for (int x=0; x<GAMEPAD_NUM_BUTTONS; x++)
		{
			GamePad[portNum].bmap[x].ButtType  = BUTTC_KEYBOARD;
			GamePad[portNum].bmap[x].DeviceNum = 0;
			GamePad[portNum].bmap[x].ButtonNum = DefaultGamePad[portNum][x];

			g_config->setOption (prefix + GamePadNames[x],
				     GamePad[portNum].bmap[x].ButtonNum);

		   if (GamePad[portNum].bmap[x].ButtType == BUTTC_KEYBOARD)
			{
				g_config->setOption (prefix + "DeviceType", "Keyboard");
			}
			else if (GamePad[portNum].bmap[x].ButtType == BUTTC_JOYSTICK)
			{
				g_config->setOption (prefix + "DeviceType", "Joystick");
			}
			else
			{
				g_config->setOption (prefix + "DeviceType", "Unknown");
			}
			g_config->setOption (prefix + "DeviceNum",
					     GamePad[portNum].bmap[x].DeviceNum);
		}
	}
	else
	{
		GamePad[portNum].setDeviceIndex( devIdx );
		GamePad[portNum].loadDefaults();
	}
	updateCntrlrDpy();
}
//----------------------------------------------------
void GamePadConfDialog_t::createNewProfile( const char *name )
{
   printf("Creating: %s \n", name );

   GamePad[portNum].createProfile(name);

   mapSel->addItem( tr(name) );
}
//----------------------------------------------------
void GamePadConfDialog_t::newProfileCallback(void)
{
   int ret;
   QInputDialog dialog(this);

   dialog.setWindowTitle( tr("New Profile") );
   dialog.setLabelText( tr("Specify New Profile Name") );
   dialog.setOkButtonText( tr("Create") );

   dialog.show();
   ret = dialog.exec();

   if ( QDialog::Accepted == ret )
   {
      createNewProfile( dialog.textValue().toStdString().c_str() );
   }
}
//----------------------------------------------------
void GamePadConfDialog_t::loadProfileCallback(void)
{
   char stmp[256];
   int index, devIdx, ret;
   std::string mapName;

	index  = devSel->currentIndex();
	devIdx = devSel->itemData(index).toInt();

   mapName = mapSel->currentText().toStdString();

	GamePad[portNum].setDeviceIndex( devIdx );

   if ( mapName.compare("default") == 0 )
   {
      ret =GamePad[portNum].loadDefaults();
   }
   else
   {
      ret = GamePad[portNum].loadProfile( mapName.c_str() );
   }
   if ( ret == 0 )
   {
      sprintf( stmp, "Mapping Loaded: %s/%s \n", GamePad[portNum].getGUID(), mapName.c_str() );
   }
   else
   {
      sprintf( stmp, "Error: Failed to Load Mapping: %s/%s \n", GamePad[portNum].getGUID(), mapName.c_str() );
   }
   mapMsg->setText( tr(stmp) );

	updateCntrlrDpy();
}
//----------------------------------------------------
void GamePadConfDialog_t::saveProfileCallback(void)
{
   int ret;
   std::string mapName;
   char stmp[256];

   mapName = mapSel->currentText().toStdString();

   ret = GamePad[portNum].saveCurrentMapToFile( mapName.c_str() );

   if ( ret == 0 )
   {
      sprintf( stmp, "Mapping Saved: %s/%s \n", GamePad[portNum].getGUID(), mapName.c_str() );
   }
   else
   {
      sprintf( stmp, "Error: Failed to Save Mapping: %s \n", mapName.c_str() );
   }
   mapMsg->setText( tr(stmp) );

}
//----------------------------------------------------
void GamePadConfDialog_t::deleteProfileCallback(void)
{
   int ret;
   std::string mapName;
   char stmp[256];

   mapName = mapSel->currentText().toStdString();

   ret = GamePad[portNum].deleteMapping( mapName.c_str() );

   if ( ret == 0 )
   {
      sprintf( stmp, "Mapping Deleted: %s/%s \n", GamePad[portNum].getGUID(), mapName.c_str() );
   }
   else
   {
      sprintf( stmp, "Error: Failed to Delete Mapping: %s \n", mapName.c_str() );
   }
   mapMsg->setText( tr(stmp) );

   loadMapList();
}
//----------------------------------------------------
void GamePadConfDialog_t::updatePeriodic(void)
{
   for (int i=0; i<GAMEPAD_NUM_BUTTONS; i++)
   {
      const char *txt, *style;
      if ( GamePad[portNum].bmap[i].state )
      {
         txt = "  T  ";
         style = "background-color: green; color: white;";
      }
      else
      {
         txt = "  F  ";
         style = "background-color: red; color: white;";
      }
      keyState[i]->setText( tr(txt) );
      keyState[i]->setStyleSheet( style );
   }
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
