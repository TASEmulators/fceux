// SplashScreen.h
//
#pragma once

#include <QTimer>
#include <QPixmap>
#include <QCloseEvent>
#include <QSplashScreen>

class fceuSplashScreen : public QSplashScreen
{
	Q_OBJECT

	public:
		fceuSplashScreen(void);
		~fceuSplashScreen(void);

	protected:
		void closeEvent(QCloseEvent *event);

	private:
		QTimer  *updateTimer;
		int      alpha;

	private slots:
		void periodicUpdate(void);

};
