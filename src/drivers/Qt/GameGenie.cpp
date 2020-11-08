// GameGenie.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <QHeaderView>
#include <QCloseEvent>

#include "../../types.h"
#include "../../fceu.h"
#include "../../cart.h"
#include "../../cheat.h"
#include "../../debug.h"
#include "../../driver.h"

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/HexEditor.h"
#include "Qt/GameGenie.h"

static const char *GameGenieLetters = "APZLGITYEOXUKSVN";

class fceuGGCodeValidtor : public QValidator
{ 
   public:
   	fceuGGCodeValidtor( QObject *parent)
			: QValidator(parent)
		{

		}

		QValidator::State validate(QString &input, int &pos) const
		{
		   int i,j, ok;
		   //printf("Validate: %i '%s'\n", input.size(), input.toStdString().c_str() );
		
		   if ( input.size() == 0 )
		   {
		      return QValidator::Acceptable;
		   }
			input = input.toUpper();
		   std::string s = input.toStdString();
		   i=0; 

			while ( s[i] != 0 )
			{
				j=0; ok=0;
				while ( GameGenieLetters[j] != 0 )
				{
					if ( s[i] == GameGenieLetters[j] )
					{
						ok = 1; break;
					}
					j++;
				}

				if ( !ok )
				{
		   		return QValidator::Invalid;
				}
				i++;
			}
		
		   return QValidator::Acceptable;
		}

	private:
};
//----------------------------------------------------------------------------
GameGenieDialog_t::GameGenieDialog_t(QWidget *parent)
	: QDialog( parent )
{
	int charWidth;
	QVBoxLayout *mainLayout, *vbox1, *vbox;
	QHBoxLayout *hbox1, *hbox;
	QTreeWidgetItem *item;
	QGroupBox *frame;
	QFont font;
	fceuGGCodeValidtor *ggCodeValidator;

	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );

	QFontMetrics fm(font);

#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
    charWidth = fm.horizontalAdvance(QLatin1Char('2'));
#else
    charWidth = fm.width(QLatin1Char('2'));
#endif

	setWindowTitle("Game Genie Encoder/Decoder Tool");

	mainLayout = new QVBoxLayout();
	vbox  = new QVBoxLayout();
	vbox1 = new QVBoxLayout();
	hbox1 = new QHBoxLayout();

	frame = new QGroupBox( tr("Address/Compare/Value") );
	mainLayout->addLayout( hbox1 );
	frame->setLayout( vbox );

	hbox1->addWidget( frame );
	hbox1->addLayout( vbox1 );
	addr = new QLineEdit();
	cmp  = new QLineEdit();
	val  = new QLineEdit();

	hbox = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( new QLabel( tr("Address:") ), 0, Qt::AlignRight );
	hbox->addWidget( addr, 0, Qt::AlignLeft );

	hbox = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( new QLabel( tr("Compare:") ), 0, Qt::AlignRight );
	hbox->addWidget( cmp, 0, Qt::AlignLeft );

	hbox = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( new QLabel( tr("Value:") ), 0, Qt::AlignRight );
	hbox->addWidget( val, 0, Qt::AlignLeft );

	frame = new QGroupBox( tr("Game Genie Code") );
	vbox  = new QVBoxLayout();
	vbox1->addWidget( frame );
	frame->setLayout( vbox );
	
	ggCode = new QLineEdit();
	vbox->addWidget( ggCode );

	addCheatBtn = new QPushButton( tr("Add To Cheat List") );
	vbox1->addWidget( addCheatBtn );

	tree = new QTreeWidget();

	tree->setColumnCount(1);

	item = new QTreeWidgetItem();
	item->setText( 0, QString::fromStdString( "Possible Affected ROM File Addresses" ) );
	item->setTextAlignment( 0, Qt::AlignLeft);

	tree->setHeaderItem( item );

	tree->header()->setSectionResizeMode( QHeaderView::ResizeToContents );

	mainLayout->addWidget( tree );

	setLayout( mainLayout );

	addrValidator = new fceuHexIntValidtor( 0, 0xFFFF, this );
	cmpValidator  = new fceuHexIntValidtor( 0, 0x00FF, this );
	valValidator  = new fceuHexIntValidtor( 0, 0x00FF, this );

	ggCodeValidator = new fceuGGCodeValidtor( this );

	addr->setValidator( addrValidator );
	cmp->setValidator( cmpValidator );
	val->setValidator( valValidator );
	ggCode->setValidator( ggCodeValidator );

	addr->setMaxLength( 4 );
	cmp->setMaxLength( 2 );
	val->setMaxLength( 2 );
	ggCode->setMaxLength( 8 );

	addr->setMinimumWidth( 6 * charWidth );
	cmp->setMinimumWidth( 4 * charWidth );
	val->setMinimumWidth( 4 * charWidth );
	addr->setMaximumWidth( 6 * charWidth );
	cmp->setMaximumWidth( 4 * charWidth );
	val->setMaximumWidth( 4 * charWidth );

	addr->setAlignment(Qt::AlignCenter);
	cmp->setAlignment(Qt::AlignCenter);
	val->setAlignment(Qt::AlignCenter);

	addr->setFont( font );
	cmp->setFont( font );
	val->setFont( font );
	ggCode->setFont( font );

	connect( addCheatBtn, SIGNAL(clicked(void)), this, SLOT(addCheatClicked(void)));

	connect( addr  , SIGNAL(textEdited(const QString &)), this, SLOT(addrChanged(const QString &)));
	connect( cmp   , SIGNAL(textEdited(const QString &)), this, SLOT(cmpChanged(const QString &)));
	connect( val   , SIGNAL(textEdited(const QString &)), this, SLOT(valChanged(const QString &)));
	connect( ggCode, SIGNAL(textEdited(const QString &)), this, SLOT(ggChanged(const QString &)));

	connect( tree, SIGNAL(itemActivated(QTreeWidgetItem*, int)), this, SLOT(romAddrDoubleClicked(QTreeWidgetItem*, int)) );

	addCheatBtn->setEnabled( false );
}
//----------------------------------------------------------------------------
GameGenieDialog_t::~GameGenieDialog_t(void)
{
	printf("Destroy Game Genie Window\n");
}
//----------------------------------------------------------------------------
void GameGenieDialog_t::closeEvent(QCloseEvent *event)
{
   printf("Game Genie Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------------------------------
void GameGenieDialog_t::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void GameGenieDialog_t::addCheatClicked(void)
{
	int a = -1, v = -1, c = -1;
	std::string name;

	name = ggCode->text().toStdString();

	if ( addr->text().size() > 0 )
	{
		a = strtol( addr->text().toStdString().c_str(), NULL, 16 );
	}
	if ( val->text().size() > 0 )
	{
		v = strtol( val->text().toStdString().c_str(), NULL, 16 );
	}
	if ( cmp->text().size() > 0 )
	{
		c = strtol( cmp->text().toStdString().c_str(), NULL, 16 );
	}

	fceuWrapperLock();
	FCEUI_AddCheat( name.c_str(), a, v, c, 1 );
	fceuWrapperUnLock();

}
//----------------------------------------------------------------------------
void GameGenieDialog_t::romAddrDoubleClicked(QTreeWidgetItem *item, int column)
{
	int addr;

	addr = strtol( item->text(0).toStdString().c_str(), NULL, 16 );

	printf("ROM Addr: %06X \n", addr );

	hexEditorOpenFromDebugger( QHexEdit::MODE_NES_ROM, addr );
}
//----------------------------------------------------------------------------
void GameGenieDialog_t::addrChanged(const QString &s)
{
	int a, v, c = -1;
	char gg[12];

	a = strtol(           s.toStdString().c_str(), NULL, 16 );
	v = strtol( val->text().toStdString().c_str(), NULL, 16 );

	if ( cmp->text().size() > 0 )
	{
		c = strtol( cmp->text().toStdString().c_str(), NULL, 16 );
	}

	EncodeGG( gg, a, v, c );

	ggCode->setText( tr(gg) );

	ListGGAddresses();
}
//----------------------------------------------------------------------------
void GameGenieDialog_t::cmpChanged(const QString &s)
{
	int a, v, c = -1;
	char gg[12];

	a = strtol( addr->text().toStdString().c_str(), NULL, 16 );
	v = strtol(  val->text().toStdString().c_str(), NULL, 16 );

	if ( s.size() > 0 )
	{
		c = strtol( s.toStdString().c_str(), NULL, 16 );
	}

	EncodeGG( gg, a, v, c );

	ggCode->setText( tr(gg) );

	ListGGAddresses();
}
//----------------------------------------------------------------------------
void GameGenieDialog_t::valChanged(const QString &s)
{
	int a, v, c = -1;
	char gg[12];

	a = strtol( addr->text().toStdString().c_str(), NULL, 16 );
	v = strtol(            s.toStdString().c_str(), NULL, 16 );

	if ( cmp->text().size() > 0 )
	{
		c = strtol( cmp->text().toStdString().c_str(), NULL, 16 );
	}
	EncodeGG( gg, a, v, c );

	ggCode->setText( tr(gg) );

	ListGGAddresses();
}
//----------------------------------------------------------------------------
void GameGenieDialog_t::ggChanged(const QString &s)
{
	int a = -1, c = -1, v = -1;
	char gg[12];
	char stmp[32];

	memset( gg, 0, sizeof(gg) );

	strncpy( gg, ggCode->text().toStdString().c_str(), 8 );

	FCEUI_DecodeGG( gg, &a, &v, &c);

	if ( a >= 0 )
	{
		sprintf( stmp, "%04X", a );

		addr->setText( tr(stmp) );
	}
	else
	{
		addr->clear();
	}

	if ( v >= 0 )
	{
		sprintf( stmp, "%02X", v );

		val->setText( tr(stmp) );
	}
	else
	{
		val->clear();
	}

	if ( c >= 0 )
	{
		sprintf( stmp, "%02X", c );

		cmp->setText( tr(stmp) );
	}
	else
	{
		cmp->clear();
	}
	
	ListGGAddresses();
}
//----------------------------------------------------------------------------
//The code in this function is a modified version
//of Chris Covell's work - I'd just like to point that out
void EncodeGG(char *str, int a, int v, int c)
{
	uint8 num[8];
	int i;
	
	a&=0x7fff;

	num[0]=(v&7)+((v>>4)&8);
	num[1]=((v>>4)&7)+((a>>4)&8);
	num[2]=((a>>4)&7);
	num[3]=(a>>12)+(a&8);
	num[4]=(a&7)+((a>>8)&8);
	num[5]=((a>>8)&7);

	if (c == -1){
		num[5]+=v&8;
		for(i = 0;i < 6;i++)str[i] = GameGenieLetters[num[i]];
		str[6] = 0;
	} else {
		num[2]+=8;
		num[5]+=c&8;
		num[6]=(c&7)+((c>>4)&8);
		num[7]=((c>>4)&7)+(v&8);
		for(i = 0;i < 8;i++)str[i] = GameGenieLetters[num[i]];
		str[8] = 0;
	}
	return;
}
//----------------------------------------------------------------------------
void GameGenieDialog_t::ListGGAddresses(void)
{
	int i; //mbg merge 7/18/06 changed from int
	int a = -1; int v = -1; int c = -1;
	QTreeWidgetItem *item;
	char str[32];
	bool addCheatEmable;

	if ( addr->text().size() > 0 )
	{
		a = strtol( addr->text().toStdString().c_str(), NULL, 16 );
	}
	if ( val->text().size() > 0 )
	{
		v = strtol( val->text().toStdString().c_str(), NULL, 16 );
	}
	if ( cmp->text().size() > 0 )
	{
		c = strtol( cmp->text().toStdString().c_str(), NULL, 16 );
	}
	// also enable/disable the add GG button here
	addCheatEmable = (a >= 0) && ( (ggCode->text().size() == 6) || (ggCode->text().size() == 8) );

	addCheatBtn->setEnabled( addCheatEmable );
	
	tree->clear();

	if (a != -1 && v != -1)
	{
		for (i = 0; i < PRGsize[0]; i += 0x2000)
		{
			if (c == -1 || PRGptr[0][i + (a & 0x1FFF)] == c)
			{
				item = new QTreeWidgetItem();

				sprintf(str, "%06X", i + (a & 0x1FFF) + 0x10);

				//printf("Added ROM ADDR: %s\n", str );
				item->setText( 0, tr(str) );

				item->setTextAlignment( 0, Qt::AlignCenter);
				tree->addTopLevelItem( item );
			}
		}
	}
	tree->viewport()->update();
}
//----------------------------------------------------------------------------
