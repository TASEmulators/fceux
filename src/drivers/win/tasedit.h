#include "movie.h"

#define FRAMES_TOO_FAR 60
#define NUM_JOYPADS 4
#define NUM_JOYPAD_BUTTONS 8
#define GREENZONE_DEFAULT_CAPACITY 50000
#define GREENZONE_MIN_CAPACITY 1
#define GREENZONE_MAX_CAPACITY 200000	// maybe even more
#define PAUSEFRAME_BLINKING_PERIOD 100
// listview column names
#define COLUMN_ARROW 0
#define COLUMN_FRAMENUM	 1
#define COLUMN_JOYPAD1_A 2
#define COLUMN_JOYPAD1_B 3
#define COLUMN_JOYPAD1_S 4
#define COLUMN_JOYPAD1_T 5
#define COLUMN_JOYPAD1_U 6
#define COLUMN_JOYPAD1_D 7
#define COLUMN_JOYPAD1_L 8
#define COLUMN_JOYPAD1_R 9
#define COLUMN_JOYPAD2_A 10
#define COLUMN_JOYPAD2_B 11
#define COLUMN_JOYPAD2_S 12
#define COLUMN_JOYPAD2_T 13
#define COLUMN_JOYPAD2_U 14
#define COLUMN_JOYPAD2_D 15
#define COLUMN_JOYPAD2_L 16
#define COLUMN_JOYPAD2_R 17
#define COLUMN_JOYPAD3_A 18
#define COLUMN_JOYPAD3_B 19
#define COLUMN_JOYPAD3_S 20
#define COLUMN_JOYPAD3_T 21
#define COLUMN_JOYPAD3_U 22
#define COLUMN_JOYPAD3_D 23
#define COLUMN_JOYPAD3_L 24
#define COLUMN_JOYPAD3_R 25
#define COLUMN_JOYPAD4_A 26
#define COLUMN_JOYPAD4_B 27
#define COLUMN_JOYPAD4_S 28
#define COLUMN_JOYPAD4_T 29
#define COLUMN_JOYPAD4_U 30
#define COLUMN_JOYPAD4_D 31
#define COLUMN_JOYPAD4_L 32
#define COLUMN_JOYPAD4_R 33
#define COLUMN_FRAMENUM2 34

// listview colors
#define NORMAL_FRAMENUM_COLOR 0xFFFFFF
#define CUR_FRAMENUM_COLOR 0xFCF1CE
#define GREENZONE_FRAMENUM_COLOR 0xDCFFDC
#define LAG_FRAMENUM_COLOR 0xDAD9FE
#define NORMAL_INPUT_COLOR1 0xF0F0F0
#define CUR_INPUT_COLOR1 0xF7E9B2
#define GREENZONE_INPUT_COLOR1 0xC3FFC3
#define LAG_INPUT_COLOR1 0xCCC8EE
#define NORMAL_INPUT_COLOR2 0xDEDEDE
#define CUR_INPUT_COLOR2 0xE4D8A8
#define GREENZONE_INPUT_COLOR2 0xAEE2AE
#define LAG_INPUT_COLOR2 0xB8B3E2

// -----------------------------
void EnterTasEdit();
void ExitTasEdit();
void UpdateTasEdit();
void InvalidateGreenZone(int after);
bool JumpToFrame(int index);
int FindBeginningOfGreenZone(int starting_index);
void FollowPlayback();
void ClearSelection();
void AddFourscoreColumns();
void RemoveFourscoreColumns();
void RedrawTasedit();
void RedrawList();
void RedrawRow(int index);
void RedrawTweakCount();