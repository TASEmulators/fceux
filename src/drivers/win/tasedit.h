#include "movie.h"

#define NUM_JOYPADS			4
#define NUM_JOYPAD_BUTTONS	8
#define COLUMN_ARROW		0
#define COLUMN_FRAMENUM		1
#define COLUMN_JOYPAD1_A	2
#define COLUMN_JOYPAD1_B	3
#define COLUMN_JOYPAD1_S	4
#define COLUMN_JOYPAD1_T	5
#define COLUMN_JOYPAD1_U	6
#define COLUMN_JOYPAD1_D	7
#define COLUMN_JOYPAD1_L	8
#define COLUMN_JOYPAD1_R	9
#define COLUMN_JOYPAD2_A	10
#define COLUMN_JOYPAD2_B	11
#define COLUMN_JOYPAD2_S	12
#define COLUMN_JOYPAD2_T	13
#define COLUMN_JOYPAD2_U	14
#define COLUMN_JOYPAD2_D	15
#define COLUMN_JOYPAD2_L	16
#define COLUMN_JOYPAD2_R	17
#define COLUMN_JOYPAD3_A	18
#define COLUMN_JOYPAD3_B	19
#define COLUMN_JOYPAD3_S	20
#define COLUMN_JOYPAD3_T	21
#define COLUMN_JOYPAD3_U	22
#define COLUMN_JOYPAD3_D	23
#define COLUMN_JOYPAD3_L	24
#define COLUMN_JOYPAD3_R	25
#define COLUMN_JOYPAD4_A	26
#define COLUMN_JOYPAD4_B	27
#define COLUMN_JOYPAD4_S	28
#define COLUMN_JOYPAD4_T	29
#define COLUMN_JOYPAD4_U	30
#define COLUMN_JOYPAD4_D	31
#define COLUMN_JOYPAD4_L	32
#define COLUMN_JOYPAD4_R	33
#define COLUMN_FRAMENUM2	34

void DoTasEdit();
void UpdateTasEdit();
void CreateProject(MovieData data);
void InvalidateGreenZone(int after);
