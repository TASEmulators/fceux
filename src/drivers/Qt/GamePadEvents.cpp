// GamePadEvents.cpp

#include <cstdlib>

#include "Qt/GamePadEvents.h"
#include "Qt/dface.h"

namespace
{
	constexpr int AXIS_PRESS_THRESHOLD   = 16384;
	constexpr int AXIS_RELEASE_THRESHOLD = 8192;

	constexpr uint32_t bit(GamePadEventHub::Button b)
	{
		return 1u << static_cast<int>(b);
	}

	// Generic button order assumed for joysticks that are not SDL game controllers
	const GamePadEventHub::Button rawButtonMap[] =
	{
		GamePadEventHub::A,  GamePadEventHub::B,  GamePadEventHub::X,     GamePadEventHub::Y,
		GamePadEventHub::L1, GamePadEventHub::R1, GamePadEventHub::Back,  GamePadEventHub::Start
	};

	bool axisActive(int value, bool wasActive)
	{
		return wasActive ? (value >= AXIS_RELEASE_THRESHOLD) : (value >= AXIS_PRESS_THRESHOLD);
	}
}

GamePadEventHub::GamePadEventHub(QObject *parent)
	: QObject(parent)
{
}

GamePadEventHub *GamePadEventHub::instance(void)
{
	static GamePadEventHub *hub = new GamePadEventHub();

	return hub;
}

void GamePadEventHub::setSourceMask(int devIdx, Source src, uint32_t newMask)
{
	DeviceState &st = state[devIdx];

	uint32_t oldEffective = st.mask[SrcButtons] | st.mask[SrcDpad] | st.mask[SrcAxes];

	st.mask[src] = newMask;

	uint32_t newEffective = st.mask[SrcButtons] | st.mask[SrcDpad] | st.mask[SrcAxes];
	uint32_t changed = oldEffective ^ newEffective;

	for (int i = 0; i < NumButtons; i++)
	{
		if (changed & (1u << i))
		{
			emit buttonChanged(devIdx, static_cast<Button>(i), (newEffective >> i) & 1);
		}
	}
}

void GamePadEventHub::updateAxes(int devIdx)
{
	DeviceState &st = state[devIdx];
	const uint32_t prev = st.mask[SrcAxes];
	uint32_t m = 0;

	m |= axisActive(-st.stickX, prev & bit(Left))  ? bit(Left)  : 0;
	m |= axisActive( st.stickX, prev & bit(Right)) ? bit(Right) : 0;
	m |= axisActive(-st.stickY, prev & bit(Up))    ? bit(Up)    : 0;
	m |= axisActive( st.stickY, prev & bit(Down))  ? bit(Down)  : 0;
	m |= axisActive( st.trigL,  prev & bit(L2))    ? bit(L2)    : 0;
	m |= axisActive( st.trigR,  prev & bit(R2))    ? bit(R2)    : 0;

	setSourceMask(devIdx, SrcAxes, m);
}

void GamePadEventHub::pump(void)
{
	static const struct { Uint32 min; Uint32 max; } ranges[] =
	{
		{ SDL_JOYAXISMOTION,       SDL_JOYBUTTONUP },
		{ SDL_CONTROLLERAXISMOTION, SDL_CONTROLLERBUTTONUP }
	};
	SDL_Event events[16];

	SDL_PumpEvents();

	for (const auto &r : ranges)
	{
		int n;

		while ((n = SDL_PeepEvents(events, 16, SDL_GETEVENT, r.min, r.max)) > 0)
		{
			for (int i = 0; i < n; i++)
			{
				processEvent(events[i]);
			}
		}
	}
}

void GamePadEventHub::resetDevice(int devIdx)
{
	if ((devIdx < 0) || (devIdx >= MAX_JOYSTICKS))
	{
		return;
	}

	for (int s = 0; s < NumSources; s++)
	{
		setSourceMask(devIdx, static_cast<Source>(s), 0);
	}
	state[devIdx] = DeviceState();
}

void GamePadEventHub::processEvent(const SDL_Event &event)
{
	int devIdx = -1;

	switch (event.type)
	{
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
		case SDL_JOYHATMOTION:
		case SDL_JOYAXISMOTION:
		{
			SDL_JoystickID id = (event.type == SDL_JOYHATMOTION)  ? event.jhat.which  :
			                    (event.type == SDL_JOYAXISMOTION) ? event.jaxis.which :
			                                                        event.jbutton.which;
			devIdx = FindJoystickByInstanceID(id);

			// Game controllers are handled through the SDL_CONTROLLER* events
			jsDev_t *dev = getJoystickDevice(devIdx);

			if ((dev == nullptr) || dev->isGameController())
			{
				return;
			}
			break;
		}
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			devIdx = FindJoystickByInstanceID(event.cbutton.which);
			break;
		case SDL_CONTROLLERAXISMOTION:
			devIdx = FindJoystickByInstanceID(event.caxis.which);
			break;
		default:
			return;
	}

	if ((devIdx < 0) || (devIdx >= MAX_JOYSTICKS))
	{
		return;
	}
	DeviceState &st = state[devIdx];

	switch (event.type)
	{
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
		{
			const int idx = event.jbutton.button;

			if ((idx >= 0) && (idx < static_cast<int>(sizeof(rawButtonMap) / sizeof(rawButtonMap[0]))))
			{
				uint32_t m = st.mask[SrcButtons];

				if (event.type == SDL_JOYBUTTONDOWN)
					m |= bit(rawButtonMap[idx]);
				else
					m &= ~bit(rawButtonMap[idx]);

				setSourceMask(devIdx, SrcButtons, m);
			}
			break;
		}
		case SDL_JOYHATMOTION:
		{
			if (event.jhat.hat != 0)
			{
				break;
			}
			uint32_t m = 0;

			m |= (event.jhat.value & SDL_HAT_UP)    ? bit(Up)    : 0;
			m |= (event.jhat.value & SDL_HAT_DOWN)  ? bit(Down)  : 0;
			m |= (event.jhat.value & SDL_HAT_LEFT)  ? bit(Left)  : 0;
			m |= (event.jhat.value & SDL_HAT_RIGHT) ? bit(Right) : 0;

			setSourceMask(devIdx, SrcDpad, m);
			break;
		}
		case SDL_JOYAXISMOTION:
		{
			if (event.jaxis.axis == 0)
			{
				st.stickX = event.jaxis.value;
			}
			else if (event.jaxis.axis == 1)
			{
				st.stickY = event.jaxis.value;
			}
			else
			{
				break;
			}
			updateAxes(devIdx);
			break;
		}
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
		{
			const bool down = (event.type == SDL_CONTROLLERBUTTONDOWN);
			Source src = SrcButtons;
			Button btn;

			switch (event.cbutton.button)
			{
				case SDL_CONTROLLER_BUTTON_A:             btn = A;     break;
				case SDL_CONTROLLER_BUTTON_B:             btn = B;     break;
				case SDL_CONTROLLER_BUTTON_X:             btn = X;     break;
				case SDL_CONTROLLER_BUTTON_Y:             btn = Y;     break;
				case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  btn = L1;    break;
				case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: btn = R1;    break;
				case SDL_CONTROLLER_BUTTON_START:         btn = Start; break;
				case SDL_CONTROLLER_BUTTON_BACK:          btn = Back;  break;
				case SDL_CONTROLLER_BUTTON_DPAD_UP:       btn = Up;    src = SrcDpad; break;
				case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     btn = Down;  src = SrcDpad; break;
				case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     btn = Left;  src = SrcDpad; break;
				case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    btn = Right; src = SrcDpad; break;
				default:
					return;
			}
			uint32_t m = st.mask[src];

			if (down)
				m |= bit(btn);
			else
				m &= ~bit(btn);

			setSourceMask(devIdx, src, m);
			break;
		}
		case SDL_CONTROLLERAXISMOTION:
		{
			switch (event.caxis.axis)
			{
				case SDL_CONTROLLER_AXIS_LEFTX:        st.stickX = event.caxis.value; break;
				case SDL_CONTROLLER_AXIS_LEFTY:        st.stickY = event.caxis.value; break;
				case SDL_CONTROLLER_AXIS_TRIGGERLEFT:  st.trigL  = event.caxis.value; break;
				case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: st.trigR  = event.caxis.value; break;
				default:
					return;
			}
			updateAxes(devIdx);
			break;
		}
		default:
			break;
	}
}
