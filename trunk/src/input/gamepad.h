#define GPAD_COUNT 4 // max number of gamepads
#define GPAD_NUMKEYS 10 // number of CONFIGURABLE keys per gamepad
	// as an input device gamepad still have only 8 'ports' (10
	// minus turbo A and B, delivered through normal A and B buttons)

// gamepad state bitmasks
#define GPAD_A 0x1
#define GPAD_B 0x2
#define GPAD_SELECT 0x4
#define GPAD_START 0x8
#define GPAD_UP 0x10
#define GPAD_DOWN 0x20
#define GPAD_LEFT 0x40
#define GPAD_RIGHT 0x80
