// RetroAchievementsConf.h
//
// Account settings and login dialog for RetroAchievements.
//
#pragma once

#ifdef FCEUX_HAS_RETRO_ACHIEVEMENTS

#include <QDialog>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

class RetroAchievementsDialog_t : public QDialog
{
	Q_OBJECT

	public:
		// Shows the dialog, or raises it when it is already open.
		static void open(QWidget *parent);

	private:
		RetroAchievementsDialog_t(QWidget *parent);
		~RetroAchievementsDialog_t(void);

		QCheckBox   *enableCbox;
		QCheckBox   *soundCbox;
		QLineEdit   *userEdit;
		QLineEdit   *passwordEdit;
		QPushButton *loginButton;
		QPushButton *logoutButton;
		QLabel      *statusLabel;
		QLabel      *avatarLabel;

		void updateControls(void);
		void updateAvatar(void);

	private slots:
		void enableToggled(bool checked);
		void soundToggled(bool checked);
		void loginClicked(void);
		void logoutClicked(void);
		void loginStateChanged(int state, const QString &userName);
		void loginFailed(const QString &message);
		void imageReady(const QString &url);
};

#endif // FCEUX_HAS_RETRO_ACHIEVEMENTS
