// boardctl.h
//
// Registry that lets a cartridge board expose user-adjustable controls
// (currently single-bit DIP switches) to a driver UI. Boards register their
// controls on init; the registry is cleared when the ROM is closed.
//
// Not thread safe: callers must hold the emulation lock (or be on the
// emulation thread) when registering, clearing, reading or writing switches.

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct BoardDipSwitch
{
	uint8_t *addr;      // byte holding the switch state
	uint8_t bit;        // bit index within the byte, 0-7
	std::string name;
	// Optional. Used instead of writing *addr directly when a change has to go
	// through another path, such as the movie/netplay command queue.
	std::function<void(bool)> setter;

	bool get(void) const { return ((*addr) >> bit) & 1; }
	void set(bool v) const
	{
		if (setter)
		{
			setter(v);
			return;
		}
		*addr = (*addr & ~(1u << bit)) | (static_cast<uint8_t>(v) << bit);
	}
};

// Registers a single bit. Duplicate (addr, bit) registrations are ignored.
void FCEU_BoardRegisterDipSwitch(uint8_t *addr, uint8_t bit, const char *name,
                                 std::function<void(bool)> setter = nullptr);

// Registers bits 0..numBits-1 of *addr, named "<prefix> <switch number>" (1-based).
void FCEU_BoardRegisterDipSwitchBits(uint8_t *addr, uint8_t numBits, const char *prefix);

// Removes all registered controls. Called on ROM close and before ROM load.
void FCEU_BoardClearControls(void);

const std::vector<BoardDipSwitch>& FCEU_BoardGetDipSwitches(void);
