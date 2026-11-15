// GamePadMenuBrowser.h
//
// Dialog that lists the entries of a QMenuBar, and its submenus, so they can
// be operated with a gamepad. It reads the live QAction tree, so it never needs
// to be kept in sync with the real menu, and it does not modify it.
// The chosen action is not triggered by the dialog; the caller triggers it
// after the dialog has closed.

#pragma once

#include <QDialog>
#include <QList>
#include <QPointer>
#include <QString>

#include "Qt/GamePadEvents.h"

class QAction;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QMenu;
class QMenuBar;
class QPushButton;
class GamePadNavigator;

class GamePadMenuBrowser : public QDialog
{
	Q_OBJECT

	public:
		GamePadMenuBrowser(QWidget *parent, QMenuBar *menuBar);

		// The leaf action that was chosen, or nullptr
		QAction *selectedAction(void) const;

	protected:
		void keyPressEvent(QKeyEvent *event) override;
		void showEvent(QShowEvent *event) override;
		void hideEvent(QHideEvent *event) override;

	private slots:
		void onNavigatorButton(GamePadEventHub::Button button);
		void onItemActivated(QListWidgetItem *item);

	private:
		struct Level
		{
			QPointer<QMenu> menu;
			QString title;
			int row;
		};

		void populate(int selectRow = -1);
		void enterSubmenu(QAction *action);
		void goBack(void);
		QString plainText(const QAction *action) const;
		QString actionText(const QAction *action) const;

		QMenuBar *menuBar;
		QLabel *pathLabel;
		QLabel *hintLabel;
		QListWidget *list;
		QPushButton *closeButton;
		GamePadNavigator *navigator;

		QList<Level> stack;
		QList<QPointer<QAction>> entries;
		QPointer<QAction> chosen;
};
