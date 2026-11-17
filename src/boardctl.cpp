// boardctl.cpp
//
#include "boardctl.h"

static std::vector<BoardDipSwitch> dipSwitches;

void FCEU_BoardRegisterDipSwitch(uint8_t *addr, uint8_t bit, const char *name,
                                 std::function<void(bool)> setter)
{
	if (addr == nullptr || bit > 7)
	{
		return;
	}
	for (const BoardDipSwitch &sw : dipSwitches)
	{
		if (sw.addr == addr && sw.bit == bit)
		{
			return;
		}
	}
	dipSwitches.push_back({addr, bit, name ? name : "", std::move(setter)});
}

void FCEU_BoardRegisterDipSwitchBits(uint8_t *addr, uint8_t numBits, const char *prefix)
{
	for (uint8_t i = 0; i < numBits && i < 8; i++)
	{
		FCEU_BoardRegisterDipSwitch(addr, i, (std::string(prefix) + " " + std::to_string(i + 1)).c_str());
	}
}

void FCEU_BoardClearControls(void)
{
	dipSwitches.clear();
}

const std::vector<BoardDipSwitch>& FCEU_BoardGetDipSwitches(void)
{
	return dipSwitches;
}
