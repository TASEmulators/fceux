// HotKeyConf.h
//

#pragma once

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

#include "Qt/main.h"

class HotKeyConfSetDialog_t : public QDialog
{
		Q_OBJECT

public:
	HotKeyConfSetDialog_t( int hkIndex, int discardNum, QTreeWidgetItem *itemIn = 0, QWidget *parent = 0);
	~HotKeyConfSetDialog_t(void);

protected:
	void closeEvent(QCloseEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
	void assignHotkey(QKeyEvent *event);

	QLineEdit *keySeqText;
	QTreeWidgetItem *item;

	int  idx;
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
	void hotKeyActivated(QTreeWidgetItem *item, int column);
	void hotKeyDoubleClicked(QTreeWidgetItem *item, int column);
};
