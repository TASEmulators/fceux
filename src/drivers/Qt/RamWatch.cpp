// HotKeyConf.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <string>

#include <SDL.h>
#include <QMenuBar>
#include <QAction>
#include <QHeaderView>
#include <QCloseEvent>
#include <QGroupBox>

#include "../../types.h"
#include "../../fceu.h"
#include "../../cheat.h"
#include "../../debug.h"

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/RamWatch.h"

//----------------------------------------------------------------------------
RamWatchDialog_t::RamWatchDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QMenuBar *menuBar;
	QHBoxLayout *mainLayout;
	QVBoxLayout *vbox, *vbox1;
	QTreeWidgetItem *item;
	QMenu *fileMenu, *watchMenu;
	QAction *menuAct;
	QGroupBox *frame;
	int useNativeMenuBar;

	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );

	QFontMetrics fm(font);

#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
    fontCharWidth = 2 * fm.horizontalAdvance(QLatin1Char('2'));
#else
    fontCharWidth = 2 * fm.width(QLatin1Char('2'));
#endif

	setWindowTitle("RAM Watch");

	resize( 512, 512 );

	menuBar = new QMenuBar(this);

	// This is needed for menu bar to show up on MacOS
	g_config->getOption( "SDL.UseNativeMenuBar", &useNativeMenuBar );

	menuBar->setNativeMenuBar( useNativeMenuBar ? true : false );

	//-----------------------------------------------------------------------
	// Menu 
	//-----------------------------------------------------------------------
	// File
   fileMenu = menuBar->addMenu(tr("File"));

	// File -> New List
	menuAct = new QAction(tr("New List"), this);
   menuAct->setShortcut( QKeySequence(tr("Ctrl+N")) );
   menuAct->setStatusTip(tr("New List"));
   //connect(menuAct, SIGNAL(triggered()), this, SLOT(newListCB(void)) );

   fileMenu->addAction(menuAct);

	// File -> Open 
	menuAct = new QAction(tr("Open"), this);
   menuAct->setShortcut( QKeySequence(tr("Ctrl+O")) );
   menuAct->setStatusTip(tr("Open Watch File"));
   //connect(menuAct, SIGNAL(triggered()), this, SLOT(newListCB(void)) );

   fileMenu->addAction(menuAct);

	// File -> Save 
	menuAct = new QAction(tr("Save"), this);
   menuAct->setShortcut( QKeySequence(tr("Ctrl+S")) );
   menuAct->setStatusTip(tr("Save Watch File"));
   //connect(menuAct, SIGNAL(triggered()), this, SLOT(newListCB(void)) );

   fileMenu->addAction(menuAct);

	// File -> Save As
	menuAct = new QAction(tr("Save As"), this);
   menuAct->setShortcut( QKeySequence(tr("Ctrl+Shift+S")) );
   menuAct->setStatusTip(tr("Save As Watch File"));
   //connect(menuAct, SIGNAL(triggered()), this, SLOT(newListCB(void)) );

   fileMenu->addAction(menuAct);

	// File -> Append
	menuAct = new QAction(tr("Append"), this);
   //menuAct->setShortcut( QKeySequence(tr("Ctrl+A")) );
   menuAct->setStatusTip(tr("Append to Watch File"));
   //connect(menuAct, SIGNAL(triggered()), this, SLOT(newListCB(void)) );

   fileMenu->addAction(menuAct);

	fileMenu->addSeparator();

	// File -> Append
	menuAct = new QAction(tr("Close"), this);
   menuAct->setShortcut( QKeySequence(tr("Alt+F4")) );
   menuAct->setStatusTip(tr("Close Window"));
   connect(menuAct, SIGNAL(triggered()), this, SLOT(closeWindow(void)) );

   fileMenu->addAction(menuAct);

	// Watch
   watchMenu = menuBar->addMenu(tr("Watch"));

	// Watch -> New Watch
	menuAct = new QAction(tr("New Watch"), this);
   menuAct->setShortcut( QKeySequence(tr("N")) );
   menuAct->setStatusTip(tr("New Watch"));
   //connect(menuAct, SIGNAL(triggered()), this, SLOT(newListCB(void)) );

   watchMenu->addAction(menuAct);

	// Watch -> Edit Watch
	menuAct = new QAction(tr("Edit Watch"), this);
   menuAct->setShortcut( QKeySequence(tr("E")) );
   menuAct->setStatusTip(tr("Edit Watch"));
   //connect(menuAct, SIGNAL(triggered()), this, SLOT(newListCB(void)) );

   watchMenu->addAction(menuAct);

	// Watch -> Remove Watch
	menuAct = new QAction(tr("Remove Watch"), this);
   menuAct->setShortcut( QKeySequence(tr("R")) );
   menuAct->setStatusTip(tr("Remove Watch"));
   //connect(menuAct, SIGNAL(triggered()), this, SLOT(newListCB(void)) );

   watchMenu->addAction(menuAct);

	// Watch -> Duplicate Watch
	menuAct = new QAction(tr("Duplicate Watch"), this);
   menuAct->setShortcut( QKeySequence(tr("A")) );
   menuAct->setStatusTip(tr("Duplicate Watch"));
   //connect(menuAct, SIGNAL(triggered()), this, SLOT(newListCB(void)) );

   watchMenu->addAction(menuAct);

	// Watch -> Add Separator
	menuAct = new QAction(tr("Add Separator"), this);
   menuAct->setShortcut( QKeySequence(tr("S")) );
   menuAct->setStatusTip(tr("Add Separator"));
   //connect(menuAct, SIGNAL(triggered()), this, SLOT(newListCB(void)) );

   watchMenu->addAction(menuAct);

	watchMenu->addSeparator();

	// Watch -> Move Up
	menuAct = new QAction(tr("Move Up"), this);
   menuAct->setShortcut( QKeySequence(tr("U")) );
   menuAct->setStatusTip(tr("Move Up"));
   //connect(menuAct, SIGNAL(triggered()), this, SLOT(newListCB(void)) );

   watchMenu->addAction(menuAct);

	// Watch -> Move Down
	menuAct = new QAction(tr("Move Down"), this);
   menuAct->setShortcut( QKeySequence(tr("D")) );
   menuAct->setStatusTip(tr("Move Down"));
   //connect(menuAct, SIGNAL(triggered()), this, SLOT(newListCB(void)) );

   watchMenu->addAction(menuAct);

	//-----------------------------------------------------------------------
	// End Menu
	//-----------------------------------------------------------------------

	mainLayout = new QHBoxLayout();

	tree = new QTreeWidget();

	tree->setColumnCount(3);

	item = new QTreeWidgetItem();
	item->setText( 0, QString::fromStdString( "Address" ) );
	item->setText( 1, QString::fromStdString( "Value" ) );
	item->setText( 2, QString::fromStdString( "Notes" ) );
	item->setTextAlignment( 0, Qt::AlignLeft);
	item->setTextAlignment( 1, Qt::AlignLeft);
	item->setTextAlignment( 2, Qt::AlignLeft);

	tree->setHeaderItem( item );

	tree->header()->setSectionResizeMode( QHeaderView::ResizeToContents );

	vbox1 = new QVBoxLayout();
	vbox  = new QVBoxLayout();
	frame = new QGroupBox( tr("Watches") );
	vbox1->addWidget( frame );
	frame->setLayout( vbox  );

	up_btn = new QPushButton( tr("Up") );
	vbox->addWidget( up_btn );

	down_btn = new QPushButton( tr("Down") );
	vbox->addWidget( down_btn );

	edit_btn = new QPushButton( tr("Edit") );
	vbox->addWidget( edit_btn );

	del_btn = new QPushButton( tr("Remove") );
	vbox->addWidget( del_btn );

	new_btn = new QPushButton( tr("New") );
	vbox->addWidget( new_btn );

	dup_btn = new QPushButton( tr("Duplicate") );
	vbox->addWidget( dup_btn );

	sep_btn = new QPushButton( tr("Separator") );
	vbox->addWidget( sep_btn );

	mainLayout->addWidget( tree  );
	mainLayout->addLayout( vbox1 );
	mainLayout->setMenuBar( menuBar );

	cht_btn = new QPushButton( tr("Add Cheat") );
	vbox1->addWidget( cht_btn );

	setLayout( mainLayout );
}
//----------------------------------------------------------------------------
RamWatchDialog_t::~RamWatchDialog_t(void)
{
	printf("Destroy RAM Watch Config Window\n");
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::closeEvent(QCloseEvent *event)
{
   printf("RAM Watch Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::updateRamWatchDisplay(void)
{
	int idx=0;
	QTreeWidgetItem *item;
	std::list < ramWatch_t * >::iterator it;
	char addrStr[32], valStr1[16], valStr2[16];
	ramWatch_t *rw;

	for (it = ramWatchList.ls.begin (); it != ramWatchList.ls.end (); it++)
	{
		rw = *it;

		item = tree->topLevelItem(idx);

		if ( item == NULL )
		{
			item = new QTreeWidgetItem();

			tree->addTopLevelItem( item );
		}
		sprintf (addrStr, "0x%04X", rw->addr);

		rw->updateMem ();

		if (rw->size == 2)
		{
			if (rw->type)
			{
				sprintf (valStr1, "%6u", rw->val.u16);
			}
			else
			{
				sprintf (valStr1, "%6i", rw->val.i16);
			}
			sprintf (valStr2, "0x%04X", rw->val.u16);
		}
		else
		{
			if (rw->type)
			{
				sprintf (valStr1, "%6u", rw->val.u8);
			}
			else
			{
				sprintf (valStr1, "%6i", rw->val.i8);
			}
			sprintf (valStr2, "0x%02X", rw->val.u8);
		}

		item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren );

		//item->setFont(font);
		item->setText( 0, tr(addrStr) );
		item->setText( 1, tr(valStr1) );
		item->setText( 2, tr(rw->name.c_str())  );
   
		item->setTextAlignment( 0, Qt::AlignLeft);
		item->setTextAlignment( 1, Qt::AlignLeft);
		item->setTextAlignment( 2, Qt::AlignLeft);

		idx++;
	}
}
//----------------------------------------------------------------------------
void ramWatch_t::updateMem (void)
{
	if (size == 1)
	{
		val.u8 = GetMem (addr);
	}
	else if (size == 2)
	{
		val.u16 = GetMem (addr) | (GetMem (addr + 1) << 8);
	}
	else
	{
		val.u8 = GetMem (addr);
	}
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::saveWatchFile (const char *filename)
{
	int i;
	FILE *fp;
	const char *c;
	std::list < ramWatch_t * >::iterator it;
	ramWatch_t *rw;

	fp = fopen (filename, "w");

	if (fp == NULL)
	{
		printf ("Error: Failed to open file: %s\n", filename);
		return;
	}

	for (it = ramWatchList.ls.begin (); it != ramWatchList.ls.end (); it++)
	{
		rw = *it;

		c = rw->name.c_str ();

		fprintf (fp, "0x%04x   %c%i   ", rw->addr, rw->type ? 'U' : 'S',
			 rw->size);

		i = 0;
		fprintf (fp, "\"");
		while (c[i])
		{
			if (c[i] == '"')
			{
				fprintf (fp, "\\%c", c[i]);
			}
			else
			{
				fprintf (fp, "%c", c[i]);
			}
			i++;
		}
		fprintf (fp, "\"\n");
	}
	fclose (fp);

}
//----------------------------------------------------------------------------
void RamWatchDialog_t::loadWatchFile (const char *filename)
{
	FILE *fp;
	int i, j, a, t, s, literal;
	char line[512], stmp[512];
	ramWatch_t *rw;

	fp = fopen (filename, "r");

	if (fp == NULL)
	{
		printf ("Error: Failed to open file: %s\n", filename);
		return;
	}

	while (fgets (line, sizeof (line) - 1, fp) != NULL)
	{
		a = -1;
		t = -1;
		s = -1;
		// Check for Comments
		i = 0;
		while (line[i] != 0)
		{
			if (literal)
			{
				literal = 0;
			}
			else
			{
				if (line[i] == '#')
				{
					line[i] = 0;
					break;
				}
				else if (line[i] == '\\')
				{
					literal = 1;
				}
			}
			i++;
		}

		i = 0;
		j = 0;
		while (isspace (line[i])) i++;

		if ((line[i] == '0') && (tolower (line[i + 1]) == 'x'))
		{
			stmp[j] = '0';
			j++;
			i++;
			stmp[j] = 'x';
			j++;
			i++;

			while (isxdigit (line[i]))
			{
				stmp[j] = line[i];
				i++;
				j++;
			}
		}
		else
		{
			while (isxdigit (line[i]))
			{
				stmp[j] = line[i];
				i++;
				j++;
			}
		}
		stmp[j] = 0;

		if (j == 0) continue;

		a = strtol (stmp, NULL, 0);

		while (isspace (line[i])) i++;

		t = line[i];
		i++;
		s = line[i];
		i++;

		if ((t != 'U') && (t != 'S'))
		{
			printf ("Error: Invalid RAM Watch Byte Type: %c", t);
			continue;
		}
		if (!isdigit (s))
		{
			printf ("Error: Invalid RAM Watch Byte Size: %c", s);
			continue;
		}
		s = s - '0';

		if ((s != 1) && (s != 2) && (s != 4))
		{
			printf ("Error: Invalid RAM Watch Byte Size: %i", s);
			continue;
		}

		while (isspace (line[i])) i++;

		if (line[i] == '"')
		{
			i++;
			j = 0;
			literal = 0;
			while ((line[i] != 0))
			{
				if (literal)
				{
					literal = 0;
				}
				else
				{
					if (line[i] == '"')
					{
						break;
					}
					else if (line[i] == '\\')
					{
						literal = 1;
					}
				}
				if (!literal)
				{
					stmp[j] = line[i];
					j++;
				}
				i++;
			}
			stmp[j] = 0;
		}
		rw = new ramWatch_t;

		rw->addr = a;
		rw->type = (t == 'U') ? 1 : 0;
		rw->size = s;
		rw->name.assign (stmp);

		ramWatchList.ls.push_back (rw);
	}

	fclose (fp);

	//showAllRamWatchResults (1);
}
//----------------------------------------------------------------------------
