// GamePadNavigator.h
//
// Translates gamepad button changes from GamePadEventHub into keyboard events
// for a modal window, so that stock Qt widgets can be operated with a pad.
// While running, it also pumps SDL from its own timer, because the main
// update timer is blocked when a dialog is opened from inside it
// (e.g. a gamepad hotkey), and it releases the NES controller input.

#pragma once

#include <QObject>
#include <QPointer>
#include <QTimer>

#include "Qt/GamePadEvents.h"

class QWidget;

class GamePadNavigator : public QObject
{
	Q_OBJECT

	public:
		// Input is only handled while 'window' is visible and active.
		// Key events are posted to 'keyTarget'.
		GamePadNavigator(QWidget *window, QWidget *keyTarget);
		~GamePadNavigator(void);

		void start(void);
		void stop(void);

		// True while any navigator is running. The NES controller input is
		// suppressed during that time.
		static bool isActive(void);

	signals:
		// Buttons that are not translated into key events
		void buttonPressed(GamePadEventHub::Button button);

	private slots:
		void onGamePadButton(int devIdx, GamePadEventHub::Button button, bool pressed);
		void onRepeatTimeout(void);

	private:
		void sendKey(int key);

		QPointer<QWidget> window;
		QPointer<QWidget> keyTarget;
		QTimer pumpTimer;
		QTimer repeatTimer;
		GamePadEventHub::Button repeatButton;
		int repeatKey;
		bool running;
};
