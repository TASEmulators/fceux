// AviRiffViewer.h
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
#include <QMenuBar>
#include <QAction>

#include "Qt/avi/gwavi.h"

class AviRiffTreeItem : public QTreeWidgetItem
{
	//Q_OBJECT

	public:
		AviRiffTreeItem(int type, long long int fpos, const char *fourcc, size_t size, QTreeWidgetItem *parent = nullptr);
		~AviRiffTreeItem(void);

	private:
		int type;
		char fourcc[8];
		size_t size;
		long long int fpos;
};

class AviRiffTree : public QTreeWidget
{
	Q_OBJECT

public:
	AviRiffTree(QWidget *parent = 0);
	~AviRiffTree(void);

protected:
};

class AviRiffViewerDialog : public QDialog
{
	Q_OBJECT

public:
	AviRiffViewerDialog(QWidget *parent = 0);
	~AviRiffViewerDialog(void);

	int riffWalkCallback( int type, long long int fpos, const char *fourcc, size_t size );

protected:
	void closeEvent(QCloseEvent *event);

	QMenuBar *buildMenuBar(void);

	int  openFile( const char *filepath );
	int  closeFile(void);

	gwavi_t     *avi;
	AviRiffTree *riffTree;
	std::list <AviRiffTreeItem*> itemStack;

private:
public slots:
	void closeWindow(void);
private slots:
	void openAviFileDialog(void);
	//void ItemActivated(QTreeWidgetItem *item, int column);
};
