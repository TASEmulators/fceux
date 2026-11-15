// GamePadEvents.h
//
// Converts raw SDL joystick/game controller events into a small set of
// logical button change events that GUI widgets can listen to.

#pragma once

#include <cstdint>

#include <QObject>

#include "Qt/sdl.h"
#include "Qt/sdl-joystick.h"

class GamePadEventHub : public QObject
{
	Q_OBJECT

	public:
		// Logical buttons of a generic gamepad.
		enum Button
		{
			Up, Down, Left, Right,
			A, B, X, Y,
			L1, R1, L2, R2,
			Start, Back,
			NumButtons
		};

		static GamePadEventHub *instance(void);

		// Feed an SDL event. Events that are not joystick or game controller
		// button/hat/axis events are ignored.
		void processEvent(const SDL_Event &event);

		// Pumps SDL and feeds pending joystick button/hat/axis events to the hub.
		// Other events, such as device add/remove, are left in the SDL queue.
		// Allows a modal dialog to receive pad input while the main
		// update timer is blocked.
		void pump(void);

		// Releases every button held on the given device and clears its state.
		void resetDevice(int devIdx);

	signals:
		// Emitted once for each logical button state transition.
		void buttonChanged(int devIdx, GamePadEventHub::Button button, bool pressed);

	private:
		GamePadEventHub(QObject *parent = nullptr);

		enum Source { SrcButtons, SrcDpad, SrcAxes, NumSources };

		struct DeviceState
		{
			uint32_t mask[NumSources] = { 0, 0, 0 };
			int stickX = 0;
			int stickY = 0;
			int trigL = 0;
			int trigR = 0;
		};

		void setSourceMask(int devIdx, Source src, uint32_t newMask);
		void updateAxes(int devIdx);

		DeviceState state[MAX_JOYSTICKS];
};
