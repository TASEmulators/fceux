// RamWatch.h
//

#pragma once

#include <list>
#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>
#include <QTreeView>
#include <QTreeWidget>

#include "Qt/main.h"

struct ramWatch_t
{
	std::string name;
	int addr;
	int type;
	int size;
	int isSep;

	union
	{
		int8_t i8;
		uint8_t u8;
		int16_t i16;
		uint16_t u16;
		int32_t i32;
		uint32_t u32;
	} val;

	  ramWatch_t (void)
	{
		addr = 0;
		type = 's';
		size = 0;
		isSep = 0;
		val.u32 = 0;
	};

	void updateMem (void);
};

struct ramWatchList_t
{

	std::list <ramWatch_t*> ls;

	ramWatchList_t (void)
	{

	}

	 ~ramWatchList_t (void)
	{
		ramWatch_t *rw;

		while (!ls.empty ())
		{
			rw = ls.front ();

			delete rw;

			ls.pop_front ();
		}
	}

	size_t size (void)
	{
		return ls.size ();
	};

	void clear(void)
	{
		ramWatch_t *rw;

		while (!ls.empty ())
		{
			rw = ls.front ();

			delete rw;

			ls.pop_front ();
		}
	}

	void add_entry (const char *name, int addr, int type, int size, int isSep = 0)
	{
		ramWatch_t *rw = new ramWatch_t;

		rw->name.assign (name);
		rw->addr  = addr;
		rw->type  = type;
		rw->size  = size;
		rw->isSep = isSep;
		ls.push_back (rw);
	}

	void updateMemoryValues (void)
	{
		ramWatch_t *rw;
		std::list < ramWatch_t * >::iterator it;

		for (it = ls.begin (); it != ls.end (); it++)
		{
			rw = *it;

			rw->updateMem ();
		}
	}

	ramWatch_t *getIndex (size_t idx)
	{
		size_t i = 0;
		std::list < ramWatch_t * >::iterator it;

		for (it = ls.begin (); it != ls.end (); it++)
		{
			if (i == idx)
			{
				return *it;
			}
			i++;
		}
		return NULL;
	}

	int deleteIndex (size_t idx)
	{
		size_t i = 0;
		std::list < ramWatch_t * >::iterator it;

		for (it = ls.begin (); it != ls.end (); it++)
		{
			if (i == idx)
			{
				delete *it;
				ls.erase (it);
				return 0;
			}
			i++;
		}
		return -1;
	}

	int moveIndexUp(size_t idx)
	{
		size_t i = 0;
		std::list < ramWatch_t * >::iterator it, lp;
		
		lp = ls.begin();

		for (it = ls.begin (); it != ls.end (); it++)
		{
			if (i == idx)
			{
				ls.insert( lp, *it );
				ls.erase (it);
				return 0;
			}
			lp = it;

			i++;
		}
		return -1;
	}

	int moveIndexDown(size_t idx)
	{
		size_t i = 0;
		std::list < ramWatch_t * >::iterator it, next;
		
		for (it = ls.begin (); it != ls.end (); it++)
		{
			if (i == idx)
			{
				next = it; next++;

				if ( next != ls.end() )
				{
					ls.insert( it, *next );
					ls.erase (next);
				}
				return 0;
			}
			i++;
		}
		return -1;
	}
};

class RamWatchDialog_t : public QDialog
{
   Q_OBJECT

	public:
		RamWatchDialog_t(QWidget *parent = 0);
		~RamWatchDialog_t(void);

	protected:
		void closeEvent(QCloseEvent *event);
		void loadWatchFile (const char *filename, int append = 0);
		void saveWatchFile (const char *filename, int append = 0);

		QFont        font;
		QTreeWidget *tree;
		QPushButton *up_btn;
		QPushButton *down_btn;
		QPushButton *edit_btn;
		QPushButton *del_btn;
		QPushButton *new_btn;
		QPushButton *dup_btn;
		QPushButton *sep_btn;
		QPushButton *cht_btn;
		QTimer      *updateTimer;

		std::string  saveFileName;

		//ramWatchList_t ramWatchList;

		int  fontCharWidth;

	private:
		void updateRamWatchDisplay(void);
		void openWatchEditWindow( ramWatch_t *rw = NULL, int mode = 0);

   public slots:
      void closeWindow(void);
	private slots:
		void newListCB(void);
		void openListCB(void);
		void saveListCB(void);
		void saveListAs(void);
		void appendListCB(void);
		void periodicUpdate(void);
		void addCheatClicked(void);
		void newWatchClicked(void);
		void sepWatchClicked(void);
		void dupWatchClicked(void);
		void editWatchClicked(void);
		void removeWatchClicked(void);
		void moveWatchUpClicked(void);
		void moveWatchDownClicked(void);
		void watchClicked( QTreeWidgetItem *item, int column);

};

extern ramWatchList_t ramWatchList;

void openRamWatchWindow( QWidget *parent, int force = 0 );
