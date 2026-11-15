// GamePadFileBrowser.h
//
// Simple file browser dialog designed to be operated with a gamepad.
// Gamepad button changes reported by GamePadEventHub are translated into
// keyboard events (see GamePadNavigator) that are delivered to the file list. The file list is the
// only widget that accepts keyboard focus, so navigation can not get lost
// in other controls.

#pragma once

#include <QDialog>
#include <QDir>
#include <QStringList>

#include "Qt/GamePadEvents.h"

class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QFileIconProvider;
class GamePadNavigator;

class GamePadFileBrowser : public QDialog
{
	Q_OBJECT

	public:
		GamePadFileBrowser(QWidget *parent = nullptr, const QString &caption = QString());
		~GamePadFileBrowser(void);

		// Filters use the QFileDialog format, e.g. "NES files (*.nes *.NES)"
		void setNameFilters(const QStringList &filters);

		// Directories listed in the places view
		void setPlaces(const QStringList &paths);

		void setDirectory(const QString &path);

		// Navigates to the directory of the given file and selects it
		void selectFile(const QString &path);

		QString selectedFile(void) const;

	protected:
		void keyPressEvent(QKeyEvent *event) override;
		void showEvent(QShowEvent *event) override;
		void hideEvent(QHideEvent *event) override;

	private slots:
		void onNavigatorButton(GamePadEventHub::Button button);
		void onItemActivated(QListWidgetItem *item);
		void onOpenClicked(void);

	private:
		enum EntryType { EntryParent = 0, EntryPlaces, EntryDir, EntryFile, EntryPlace };

		void populate(const QString &selectPath = QString());
		void showPlaces(void);
		void goToParent(void);
		void cycleFilter(int delta);
		void updateFilterLabel(void);
		void addEntry(const QString &text, EntryType type, const QString &path);

		QLabel *pathLabel;
		QLabel *filterLabel;
		QListWidget *list;
		QPushButton *openButton;
		QPushButton *cancelButton;
		QFileIconProvider *iconProvider;

		QDir currentDir;
		bool placesMode;
		QStringList places;
		QStringList filterTitles;
		QList<QStringList> filterPatterns;
		int filterIdx;
		QString chosenFile;

		GamePadNavigator *navigator;
};
