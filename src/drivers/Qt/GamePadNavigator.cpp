// GamePadNavigator.cpp

#include <QApplication>
#include <QKeyEvent>
#include <QWidget>

#include "Qt/GamePadNavigator.h"
#include "Qt/input.h"

namespace
{
	constexpr int REPEAT_DELAY_MS    = 400;
	constexpr int REPEAT_INTERVAL_MS = 80;
	constexpr int PUMP_INTERVAL_MS   = 10;

	int activeCount = 0;

	// Returns the key a button is translated into, or 0 if the button is unmapped
	int buttonToKey(GamePadEventHub::Button button)
	{
		switch (button)
		{
			case GamePadEventHub::Up:    return Qt::Key_Up;
			case GamePadEventHub::Down:  return Qt::Key_Down;
			case GamePadEventHub::A:
			case GamePadEventHub::Start: return Qt::Key_Return;
			case GamePadEventHub::B:     return Qt::Key_Escape;
			case GamePadEventHub::L2:    return Qt::Key_PageUp;
			case GamePadEventHub::R2:    return Qt::Key_PageDown;
			default:                     return 0;
		}
	}

	bool isRepeatable(GamePadEventHub::Button button)
	{
		return (button == GamePadEventHub::Up)   || (button == GamePadEventHub::Down) ||
		       (button == GamePadEventHub::L2)   || (button == GamePadEventHub::R2);
	}
}

GamePadNavigator::GamePadNavigator(QWidget *windowIn, QWidget *keyTargetIn)
	: QObject(windowIn),
	  window(windowIn),
	  keyTarget(keyTargetIn),
	  repeatButton(GamePadEventHub::NumButtons),
	  repeatKey(0),
	  running(false)
{
	pumpTimer.setInterval(PUMP_INTERVAL_MS);
	pumpTimer.setTimerType(Qt::PreciseTimer);

	connect(&pumpTimer, &QTimer::timeout, GamePadEventHub::instance(), &GamePadEventHub::pump);
	connect(&repeatTimer, &QTimer::timeout, this, &GamePadNavigator::onRepeatTimeout);
	connect(GamePadEventHub::instance(), &GamePadEventHub::buttonChanged,
		this, &GamePadNavigator::onGamePadButton);
}

GamePadNavigator::~GamePadNavigator(void)
{
	stop();
}

bool GamePadNavigator::isActive(void)
{
	return activeCount > 0;
}

void GamePadNavigator::start(void)
{
	if (running)
	{
		return;
	}
	running = true;
	activeCount++;

	// The emulator thread keeps reading the last controller state while
	// the main update timer is blocked
	clearGamePadInput();

	pumpTimer.start();
}

void GamePadNavigator::stop(void)
{
	if (!running)
	{
		return;
	}
	running = false;
	activeCount--;

	pumpTimer.stop();
	repeatTimer.stop();
	repeatButton = GamePadEventHub::NumButtons;
}

void GamePadNavigator::sendKey(int key)
{
	if (keyTarget.isNull())
	{
		return;
	}
	QCoreApplication::postEvent(keyTarget, new QKeyEvent(QEvent::KeyPress, key, Qt::NoModifier));
	QCoreApplication::postEvent(keyTarget, new QKeyEvent(QEvent::KeyRelease, key, Qt::NoModifier));
}

void GamePadNavigator::onGamePadButton(int devIdx, GamePadEventHub::Button button, bool pressed)
{
	(void)devIdx;

	if (!pressed)
	{
		if (button == repeatButton)
		{
			repeatTimer.stop();
			repeatButton = GamePadEventHub::NumButtons;
		}
		return;
	}

	// Pad input is also reported while another window has focus
	if (!running || window.isNull() || !window->isVisible() || !window->isActiveWindow())
	{
		return;
	}

	const int key = buttonToKey(button);

	if (key == 0)
	{
		emit buttonPressed(button);
		return;
	}
	sendKey(key);

	if (isRepeatable(button))
	{
		repeatButton = button;
		repeatKey = key;
		repeatTimer.start(REPEAT_DELAY_MS);
	}
}

void GamePadNavigator::onRepeatTimeout(void)
{
	if (!running || (repeatButton == GamePadEventHub::NumButtons))
	{
		repeatTimer.stop();
		return;
	}
	repeatTimer.setInterval(REPEAT_INTERVAL_MS);

	sendKey(repeatKey);
}
