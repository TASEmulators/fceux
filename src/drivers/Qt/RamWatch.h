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

	union
	{
		int8_t i8;
		uint8_t u8;
		int16_t i16;
		uint16_t u16;
	} val;

	  ramWatch_t (void)
	{
		addr = 0;
		type = 0;
		size = 0;
		val.u16 = 0;
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

	void add_entry (const char *name, int addr, int type, int size)
	{
		ramWatch_t *rw = new ramWatch_t;

		rw->name.assign (name);
		rw->addr = addr;
		rw->type = type;
		rw->size = size;
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
};

class RamWatchDialog_t : public QDialog
{
   Q_OBJECT

	public:
		RamWatchDialog_t(QWidget *parent = 0);
		~RamWatchDialog_t(void);

	protected:
		void closeEvent(QCloseEvent *event);
		void loadWatchFile (const char *filename);
		void saveWatchFile (const char *filename);

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

		ramWatchList_t ramWatchList;

		int  fontCharWidth;

	private:
		void updateRamWatchDisplay(void);
		void openWatchEditWindow( int idx = -1);

   public slots:
      void closeWindow(void);
	private slots:
		void newWatchClicked(void);

};
