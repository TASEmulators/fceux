// GamePadConf.cpp
//
#include <QDir>
#include <QInputDialog>
#include <QMessageBox>
#include <QScrollArea>

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
	std::string  guid;
	std::string  profile;

	struct {

		char needsSave;

	} btn[GAMEPAD_NUM_BUTTONS];

	GamePadConfigLocalData_t(void)
	{
		for (int i=0; i<GAMEPAD_NUM_BUTTONS; i++)
		{
			btn[i].needsSave = 0;
		}
	}

};

static GamePadConfigLocalData_t lcl[GAMEPAD_NUM_DEVICES];

static GamePadConfDialog_t *gamePadConfWin = NULL;

//----------------------------------------------------
int openGamePadConfWindow( QWidget *parent )
{
	if ( gamePadConfWin != NULL )
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
	if ( gamePadConfWin != NULL )
	{
   	gamePadConfWin->closeWindow();
	}
	return 0;
}
//----------------------------------------------------
GamePadConfDialog_t::GamePadConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QWidget *mainWidget;
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox, *hbox1, *hbox2, *hbox3, *hbox4, *hbox5;
	QVBoxLayout *vbox, *vbox1, *vbox2;
	QGridLayout *grid;
	QCheckBox *udlr_chkbox;
   QGroupBox *frame1, *frame2;
	QLabel *label;
   QPushButton *newProfileButton;
   QPushButton *saveProfileButton;
   QPushButton *applyProfileButton;
   QPushButton *removeProfileButton;
   QPushButton *clearAllButton;
   QPushButton *closebutton;
   QPushButton *clearButton[GAMEPAD_NUM_BUTTONS];
	QScrollArea *scroll;
	QStyle      *style;
	std::string prefix;
	char stmp[256];

	style = this->style();

	gamePadConfWin = this;

	// Ensure that joysticks are enabled, no harm calling init again.
	InitJoysticks();

	scroll = new QScrollArea(this);
	mainWidget = new QWidget();

   portNum = 0;
   buttonConfigStatus = 1;

   inputTimer  = new QTimer( this );

   connect( inputTimer, &QTimer::timeout, this, &GamePadConfDialog_t::updatePeriodic );

   setWindowTitle( tr("GamePad Config") );

	hbox1 = new QHBoxLayout();
	hbox2 = new QHBoxLayout();
	hbox3 = new QHBoxLayout();
	hbox4 = new QHBoxLayout();
	hbox5 = new QHBoxLayout();

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
				sprintf( stmp, "%i: %s", i, js->getName() );
   			devSel->addItem( tr(stmp), i );
			}
		}
	}
	for (int i=0; i<devSel->count(); i++)
	{
		if ( devSel->itemData(i).toInt() == GamePad[portNum].getDeviceIndex() )
		{
			devSel->setCurrentIndex( i );
		}
	}

	label = new QLabel(tr("GUID:"));
	guidLbl = new QLabel();

	hbox3->addWidget( label );
	hbox3->addWidget( guidLbl );

	guidLbl->setText( GamePad[portNum].getGUID() );

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
	applyProfileButton->setIcon( style->standardIcon( QStyle::SP_DialogApplyButton ) );
   hbox->addWidget( applyProfileButton );

   saveProfileButton = new QPushButton( tr("Save") );
	saveProfileButton->setWhatsThis(tr("Stores Current Active Map to the Selected Profile"));
	saveProfileButton->setIcon( style->standardIcon( QStyle::SP_DialogSaveButton ) );
   hbox->addWidget( saveProfileButton );

	hbox = new QHBoxLayout();
	vbox->addLayout( hbox );

   newProfileButton   = new QPushButton( tr("New") );
	newProfileButton->setWhatsThis(tr("Create a New Map Profile"));
	newProfileButton->setIcon( style->standardIcon( QStyle::SP_FileIcon ) );
   hbox->addWidget( newProfileButton );

   removeProfileButton   = new QPushButton( tr("Delete") );
	removeProfileButton->setWhatsThis(tr("Deletes the Selected Map Profile"));
	removeProfileButton->setIcon( style->standardIcon( QStyle::SP_TrashIcon ) );
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

   clearAllButton     = new QPushButton(tr("Clear All"));
   closebutton        = new QPushButton(tr("Close"));

	clearAllButton->setIcon( style->standardIcon( QStyle::SP_LineEditClearButton ) );
	closebutton->setIcon( style->standardIcon( QStyle::SP_DialogCloseButton ) );

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

   connect(clearAllButton   , SIGNAL(clicked()), this, SLOT(clearAllCallback(void)) );
   connect(closebutton      , SIGNAL(clicked()), this, SLOT(closeWindow(void)) );

   connect(portSel    , SIGNAL(activated(int)), this, SLOT(portSelect(int)) );
   connect(devSel     , SIGNAL(activated(int)), this, SLOT(deviceSelect(int)) );
   connect(efs_chkbox , SIGNAL(stateChanged(int)), this, SLOT(ena4score(int)) );
   connect(udlr_chkbox, SIGNAL(stateChanged(int)), this, SLOT(oppDirEna(int)) );

	mainLayout = new QVBoxLayout();
	vbox1      = new QVBoxLayout();
	vbox2      = new QVBoxLayout();

	hbox5->addWidget( efs_chkbox );
	hbox5->addWidget( udlr_chkbox );

	vbox1->addLayout( hbox1 );
	vbox1->addLayout( hbox2 );
	vbox1->addLayout( hbox3 );
	vbox1->addWidget( frame1);
	vbox1->addLayout( hbox5 );

	vbox2->addWidget( frame2 );
	vbox2->addLayout( hbox4 );

	mainLayout->addLayout( vbox1 );
	mainLayout->addLayout( vbox2 );

	mainWidget->setLayout( mainLayout );
	mainWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

	scroll->setWidget( mainWidget );
	scroll->setWidgetResizable(true);
	scroll->setSizeAdjustPolicy( QAbstractScrollArea::AdjustToContents );
	scroll->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	scroll->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	
	QHBoxLayout *dialogLayout = new QHBoxLayout();

	dialogLayout->addWidget( scroll );

	setLayout( dialogLayout );

   inputTimer->start( 33 ); // 30hz

	for (int i=0; i<GAMEPAD_NUM_DEVICES; i++)
	{
		sprintf( stmp, "SDL.Input.GamePad.%i.", i );
		prefix = stmp;

		g_config->getOption(prefix + "Profile", &lcl[i].profile );

		lcl[i].guid.assign( GamePad[i].getGUID() );
	}

   loadMapList();
}

//----------------------------------------------------
GamePadConfDialog_t::~GamePadConfDialog_t(void)
{
   inputTimer->stop();
   buttonConfigStatus = 0;
	gamePadConfWin = NULL;

	printf("GamePad Window Deleted\n");
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
	std::string prefix, mapName;
   int index, devIdx;
   jsDev_t *js;
	size_t n=0;
	char stmp[256];

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

	sprintf( stmp, "SDL.Input.GamePad.%u.", portNum );
	prefix = stmp;

	g_config->getOption(prefix + "Profile", &mapName );

   mapSel->clear();
   mapSel->addItem( tr("default"), 0 ); n=1;

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

		if ( mapName.compare( fileName ) == 0 )
		{
			mapSel->setCurrentIndex(n);
		}
		n++;
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

		//if ( lcl[portNum].btn[i].needsSave )
		//{
      //	keyName[i]->setStyleSheet("color: red;");
		//}
		//else
		//{
      //	keyName[i]->setStyleSheet("color: black;");
		//}
	}
}
//----------------------------------------------------
void GamePadConfDialog_t::portSelect(int index)
{
	//printf("Port Number:%i \n", index);
	portNum = index;
	updateCntrlrDpy();

	for (int i=0; i<devSel->count(); i++)
	{
		if ( devSel->itemData(i).toInt() == GamePad[portNum].getDeviceIndex() )
		{
			devSel->setCurrentIndex( i );
		}
	}
	guidLbl->setText( GamePad[portNum].getGUID() );

   loadMapList();
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

	lcl[portNum].guid.assign( GamePad[portNum].getGUID() );
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

   if ( buttonConfigStatus == 2 )
   {
      buttonConfigStatus = 0;
      return;
   }
   buttonConfigStatus = 2;

   ButtonConfigBegin ();

   button[x]->setText("Waiting" );

	DWaitButton (NULL, &GamePad[padNo].bmap[x], &buttonConfigStatus );

   keyNameStr = ButtonName( &GamePad[padNo].bmap[x] );

   keyName[x]->setText( keyNameStr );
   button[x]->setText("Change");
	lcl[padNo].btn[x].needsSave = 1;

   ButtonConfigEnd ();

   buttonConfigStatus = 1;
}
//----------------------------------------------------
void GamePadConfDialog_t::clearButton( int padNo, int x )
{
	GamePad[padNo].bmap[x].ButtonNum = -1;

   keyName[x]->setText("");

	lcl[padNo].btn[x].needsSave = 1;
}
//----------------------------------------------------
void GamePadConfDialog_t::closeEvent(QCloseEvent *event)
{
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
	promptToSave();

   printf("Close Window\n");
   buttonConfigStatus = 0;
   done(0);
	deleteLater();
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
void GamePadConfDialog_t::saveConfig(void)
{
	int i;
	char stmp[256];
	std::string prefix, mapName;

	sprintf( stmp, "SDL.Input.GamePad.%u.", portNum );
	prefix = stmp;

   mapName = mapSel->currentText().toStdString();

	g_config->setOption(prefix + "DeviceGUID", GamePad[portNum].getGUID() );
	g_config->setOption(prefix + "Profile"   , mapName.c_str()            );

	for (i=0; i<GAMEPAD_NUM_BUTTONS; i++)
	{
		lcl[portNum].btn[i].needsSave = 0;
	}
	g_config->save();
}
//----------------------------------------------------
void GamePadConfDialog_t::createNewProfile( const char *name )
{
	char stmp[256];
   //printf("Creating: %s \n", name );

   GamePad[portNum].createProfile(name);

   mapSel->addItem( tr(name) );

	mapSel->setCurrentIndex( mapSel->count() - 1 );
	saveConfig();

   sprintf( stmp, "Mapping Created: %s/%s \n", GamePad[portNum].getGUID(), name );
   mapMsg->setText( tr(stmp) );
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
		saveConfig();

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
		saveConfig();

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
void GamePadConfDialog_t::promptToSave(void)
{
	int i,j,n;
	std::string msg;
	QMessageBox msgBox(this);
	char saveRequired = 0;
	char padNeedsSave[GAMEPAD_NUM_DEVICES];
	char stmp[256];

	n=0;
	for (i=0; i<GAMEPAD_NUM_DEVICES; i++)
	{
		padNeedsSave[i] = 0;

		for (j=0; j<GAMEPAD_NUM_BUTTONS; j++)
		{
			if ( lcl[i].btn[j].needsSave )
			{
				padNeedsSave[i] = 1;
				saveRequired = 1; 
				n++;
				break;
			}
		}
	}

	if ( !saveRequired )
	{
		return;
	}
	sprintf( stmp, "Warning: Gamepad mappings have not been saved for port%c ", (n > 1) ? 's':' ');

	msg.assign( stmp );

	j=n;
	for (i=0; i<GAMEPAD_NUM_DEVICES; i++)
	{
		if ( padNeedsSave[i] )
		{
			sprintf( stmp, "%i", i+1 );

			msg.append(stmp);

			j--;

			if ( j > 1 )
			{
				msg.append(", ");
			} 
			else if ( j == 1 )
			{
				msg.append(" and ");
			}
		}
	}
	msg.append(".");

	msgBox.setIcon( QMessageBox::Warning );
	msgBox.setText( tr(msg.c_str()) );

	msgBox.show();
	//msgBox.resize( 512, 128 );
	msgBox.exec();
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

		if ( lcl[portNum].btn[i].needsSave )
		{
      	keyName[i]->setStyleSheet("color: red;");
		}
		else
		{
      	keyName[i]->setStyleSheet("color: black;");
		}
   }

	int fourScore;
	g_config->getOption("SDL.FourScore", &fourScore);
	if ( fourScore != efs_chkbox->isChecked() )
	{
		efs_chkbox->setChecked( fourScore );
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
