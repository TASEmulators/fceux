#include "drivers/common/input.h"
#include "utils/bitflags.h"


bool btncfg_::IsAxisButton(uint8 which) const {
	return (ButtType[which] == BT_JOYSTICK && FL_TEST(ButtonNum[which], ISAXIS_MASK) != 0);
}

bool btncfg_::IsAxisNegative(uint8 which) const {
	assert(ButtType[which] == BT_JOYSTICK);
	return (FL_TEST(ButtonNum[which], ISAXISNEG_MASK) != 0);
}

uint8 btncfg_::GetAxisIdx(uint8 which) const {
	assert(ButtType[which] == BT_JOYSTICK);
	return FL_TEST(ButtonNum[which], AXISIDX_MASK);
}

bool btncfg_::IsPovButton(uint8 which) const {
	return (ButtType[which] == BT_JOYSTICK && FL_TEST(ButtonNum[which], ISPOVBTN_MASK) != 0);
}

uint8 btncfg_::GetPovDir(uint8 which) const {
	assert(ButtType[which] == BT_JOYSTICK);
	return FL_TEST(ButtonNum[which], POVDIR_MASK);
}

uint8 btncfg_::GetPovController(uint8 which) const {
	assert(ButtType[which] == BT_JOYSTICK);
	return (FL_TEST(ButtonNum[which], POVCONTROLLER_MASK) >> 4);
}

uint8 btncfg_::GetJoyButton(uint8 which) const {
	assert(ButtType[which] == BT_JOYSTICK);
	return FL_TEST(ButtonNum[which], JOYBTN_MASK);
}

uint16 btncfg_::GetScanCode(uint8 which) const {
	assert(ButtType[which] == BT_KEYBOARD);
	return ButtonNum[which];
}

void btncfg_::AssignScanCode(uint8 which, uint16 scode) {
	assert(ButtType[which] == BT_KEYBOARD);
	ButtonNum[which] = scode;
}

void btncfg_::AssignJoyButton(uint8 which, uint16 btnnum) {
	assert(ButtType[which] == BT_JOYSTICK);
	ButtonNum[which] = btnnum;
}
