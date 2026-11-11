// RetroAchievementsList.h
//
// Window listing the achievements and leaderboards of the loaded game.
#pragma once

#ifdef FCEUX_HAS_RETRO_ACHIEVEMENTS

#include <QDialog>
#include <QHash>
#include <QLabel>
#include <QList>
#include <QTimer>
#include <QTreeWidget>

#include "Qt/RetroAchievements/RetroAchievements.h"

class RetroAchievementsListDialog_t : public QDialog
{
	Q_OBJECT

	public:
		// Shows the window, or raises it when it is already open.
		static void open(QWidget *parent);

	private:
		RetroAchievementsListDialog_t(QWidget *parent);
		~RetroAchievementsListDialog_t(void);

		void setItemImage(QTreeWidgetItem *item, const QString &url);

		QLabel *badgeLabel;
		QLabel *titleLabel;
		QLabel *summaryLabel;
		QLabel *presenceLabel;
		QTreeWidget *achTree;
		QTreeWidget *boardTree;
		QTreeWidget *entryTree;
		QLabel *entryStatus;
		QTimer refreshTimer;
		QTimer presenceTimer;
		QHash<QString, QList<QTreeWidgetItem*> > imageItems;
		QString gameBadgeUrl;
		unsigned int selectedBoard;

	private slots:
		void refresh(void);
		void updatePresence(void);
		void imageReady(const QString &url);
		void boardSelected(void);
		void entriesReady(unsigned int boardId, const QList<RaLeaderboardEntry> &entries);
};

#endif // FCEUX_HAS_RETRO_ACHIEVEMENTS
