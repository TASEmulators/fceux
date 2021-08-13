// HotKeyConf.h
//

#pragma once

#include <string>
#include <list>
#include <map>

#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>
#include <QTreeView>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "Qt/main.h"

class HotKeyConfTreeItem_t : public QTreeWidgetItem
{
	//Q_OBJECT

	public:
		HotKeyConfTreeItem_t(int idx, QTreeWidgetItem *parent = nullptr);
		~HotKeyConfTreeItem_t(void);

		int  hkIdx;
};

class HotKeyConfSetDialog_t : public QDialog
{
	Q_OBJECT

public:
	HotKeyConfSetDialog_t( int discardNum, HotKeyConfTreeItem_t *itemIn = 0, QWidget *parent = 0);
	~HotKeyConfSetDialog_t(void);

protected:
	void closeEvent(QCloseEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
	void assignHotkey(QKeyEvent *event);
	void checkForConflicts(int hkIdx);

	QLineEdit *keySeqText;
	HotKeyConfTreeItem_t *item;

	int  discardCount;

public slots:
	void closeWindow(void);
	void cleanButtonCB(void);
	void okButtonCB(void);
};

class HotKeyConfTree_t : public QTreeWidget
{
	Q_OBJECT

public:
	HotKeyConfTree_t(QWidget *parent = 0);
	~HotKeyConfTree_t(void);

	void finalize(void);

	QTreeWidgetItem *addGroup( const char *group);
	QTreeWidgetItem *addGroup( std::string group);
	QTreeWidgetItem *findGroup( const char *group);
	QTreeWidgetItem *findGroup( std::string group);

	QTreeWidgetItem *addItem(const char *group, int i );

protected:
	std::map <std::string, QTreeWidgetItem*>  groupMap;

protected:
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
};

class HotKeyConfDialog_t : public QDialog
{
	Q_OBJECT

public:
	HotKeyConfDialog_t(QWidget *parent = 0);
	~HotKeyConfDialog_t(void);

protected:
	void closeEvent(QCloseEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);

	HotKeyConfTree_t *tree;

private:
public slots:
	void closeWindow(void);
private slots:
	void resetDefaultsCB(void);
	void hotKeyActivated(QTreeWidgetItem *item, int column);
	void hotKeyDoubleClicked(QTreeWidgetItem *item, int column);
};
