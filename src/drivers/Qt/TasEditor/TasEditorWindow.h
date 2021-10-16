// TasEditorWindow.h
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
#include <QCloseEvent>
#include <QMenu>
#include <QMenuBar>
#include <QAction>

class TasEditorWindow : public QDialog
{
	Q_OBJECT

	public:
		TasEditorWindow(QWidget *parent = 0);
		~TasEditorWindow(void);

	protected:
		void closeEvent(QCloseEvent *event);

		QMenuBar *buildMenuBar(void);
		QMenu    *recentMenu;
	private:

	public slots:
		void closeWindow(void);
	private slots:
};
