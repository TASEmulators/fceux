/**
 * qfox:
 * todo: memi() to fetch a 32bit number? is this desired?
 * todo: x memm(y) to fetch x registers starting at y, to the left, adding right nibble only in tens (04 05 03 == 453). a lot of games use this for score
 * todo: check for existence of argument (low priority)
 * todo: cleanup "static"s here and there and read up on what they mean in C, i think i've got them confused
 * todo: fix some run/stop issues

 delete new
 main.cpp contains blit-skipping: FCEUD_Update
 sprintf(TempArray,"%s\\fceu98.cfg",BaseDirectory); (main)
 **/

/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2006 Luke Gustafson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "common.h"
#include "../../fceu.h" //mbg merge 7/18/06 added
#include "basicbot.h"
#include "../../input.h" // qfox: fceu_botmode() fceu_setbotmode()

// Cleanup: static function declarations moved here
static void BotSyntaxError(int errorcode);
static void StopBasicBot();
static void StartBasicBot();
static int debug(int n);
static char * debugS(char * s);
static int error(int n);
static void FromGUI();
static void UpdateStatics();
static bool LoadBasicBotFile(char fn[]);
static bool LoadBasicBot();
static bool SaveBasicBotFile(char fn[]);
static bool SaveBasicBot();
static void PlayBest();

// v0.1.0 will be the last release by Luke, mine will start at v0.2.0 and
//	 go up each time something is released to the public, so you'll
//	 know your version is the latest or whatever. Version will be
//	 put in the project as well. A changelog will be kept.
static char BBversion[] = "0.3.4a";
// title
static char BBcaption[] = "Basic Bot v0.3.4a by qFox";
// save/load version
static int BBsaveload = 2;

int BasicBot_wndx=0, BasicBot_wndy=0;

static HWND hwndBasicBot = 0;					// GUI handle
static bool BotRunning = false;					// Is the bot computing or not?
static bool ResetStatsASAP = false;					// Do we need to reset the stats asap? (when pressing reset in gui)
static bool MovingOn = false;					// If movingon bool is set, the bot is feeding the app
												// with the best result of the current part. it will
												// not compute anything, just read input.
												// when it fed all available input, it will save to
												// current state and continue with the next part from there
static int MovingPointer = 0;					// Points to the next input to be sent

static uint32 rand1=1, rand2=0x1337EA75, rand3=0x0BADF00D;

#define BOT_FORMULAS 47							// Number of code fields in the array
char * Formula[BOT_FORMULAS];					// These hold BOT_FORMULAS formula's entered in the GUI:
int * Bytecode[BOT_FORMULAS];					// Byte code converted formulas
static int ByteCodeField;						// Working field
enum {
			CODE_1_A        = 0,				// Code fields
			CODE_1_B        = 1,
			CODE_1_SELECT   = 2,
			CODE_1_START    = 3,
			CODE_1_UP       = 4,
			CODE_1_DOWN     = 5,
			CODE_1_LEFT     = 6,
			CODE_1_RIGHT    = 7,
			CODE_2_A        = 8,
			CODE_2_B        = 9,
			CODE_2_SELECT   = 10,
			CODE_2_START    = 11,
			CODE_2_UP       = 12,
			CODE_2_DOWN     = 13,
			CODE_2_LEFT     = 14,
			CODE_2_RIGHT    = 15,

			CODE_OK         = 16,
			CODE_INVALID    = 17,
			CODE_ROLLBACK   = 18,
			CODE_MAXFRAMES  = 19,
			CODE_MAXATTEMPTS= 20,
			CODE_MAXPARTS   = 21,

			CODE_SCORE1     = 22,
			CODE_SCORE2     = 23,
			CODE_SCORE3     = 24,
			CODE_SCORE4     = 25,
			CODE_SCORE5     = 26,
			CODE_SCORE6     = 27,

			CODE_X			= 28,
			CODE_Y			= 29,
			CODE_Z			= 30,
			CODE_P			= 31,
			CODE_Q			= 32,

			CODE_EXTRA		= 33,
			CODE_INIT_ATTEMPT = 34,
			CODE_INIT_PART	= 35,

			// from here on down put inputs that
			// are not to be parsed, but visual only
			CODE_COMMENTS   = 36,
			CODE_LOG		= 37,
			CODE_TITLE1		= 38,
			CODE_TITLE2		= 39,
			CODE_TITLE3		= 40,
			CODE_TITLE4		= 41,
			CODE_TITLE5		= 42,
			CODE_TITLE6		= 43,
			CODE_ROMNAME	= 44,
			CODE_SKIPS		= 45,
			CODE_SLOW		= 46
			// BOT_FORMULAS needs to be CODE_SLOW+1 !
};

// array contains GUI id's for certain pieces of code
// these are in sync with the CODE_ ints above
int Labels[] = { 
			GUI_BOT_A_1,
			GUI_BOT_B_1,
			GUI_BOT_SELECT_1,
			GUI_BOT_START_1,
			GUI_BOT_UP_1,
			GUI_BOT_DOWN_1,
			GUI_BOT_LEFT_1,
			GUI_BOT_RIGHT_1,
			GUI_BOT_A_2,
			GUI_BOT_B_2,
			GUI_BOT_SELECT_2,
			GUI_BOT_START_2,
			GUI_BOT_UP_2,
			GUI_BOT_DOWN_2,
			GUI_BOT_LEFT_2,
			GUI_BOT_RIGHT_2,

			GUI_BOT_OK,
			GUI_BOT_INVALID,
			GUI_BOT_ROLLBACK,
			GUI_BOT_MAXFRAMES,
			GUI_BOT_MAXATTEMPTS,
			GUI_BOT_MAXPARTS,

			GUI_BOT_SCORE1,
			GUI_BOT_SCORE2,
			GUI_BOT_SCORE3,
			GUI_BOT_SCORE4,
			GUI_BOT_SCORE5,
			GUI_BOT_SCORE6,

			GUI_BOT_X,
			GUI_BOT_Y,
			GUI_BOT_Z,
			GUI_BOT_P,
			GUI_BOT_Q,

			GUI_BOT_EXTRA,
			GUI_BOT_INIT_ATTEMPT,
			GUI_BOT_INIT_PART,

			// from here on down put inputs that
			// are not to be parsed, but visual only
			GUI_BOT_COMMENTS,
			GUI_BOT_LOG,
			GUI_BOT_TITLE1,
			GUI_BOT_TITLE2,
			GUI_BOT_TITLE3,
			GUI_BOT_TITLE4,
			GUI_BOT_TITLE5,
			GUI_BOT_TITLE6,
			GUI_BOT_ROMNAME,
			GUI_BOT_SKIPS,
			GUI_BOT_SLOW
			};

// these are the commands
// they are actually the "bytecode" commands,
// used by the encoder and interpreter
const int 	BOT_BYTE_LB		= 48,
			BOT_BYTE_RB		= 1,
			BOT_BYTE_LIT	= 2,
			BOT_BYTE_COLON	= 3,
			BOT_BYTE_SCOLON = 49,
			BOT_BYTE_ADD	= 4,
			BOT_BYTE_SUB	= 5,
			BOT_BYTE_MUL	= 6,
			BOT_BYTE_DIV	= 7,
			BOT_BYTE_MOD	= 8,
			BOT_BYTE_AND	= 9,
			BOT_BYTE_OR		= 10,
			BOT_BYTE_XOR	= 11,
			BOT_BYTE_EQ		= 12,
			BOT_BYTE_GTEQ	= 13,
			BOT_BYTE_GT		= 14,
			BOT_BYTE_LTEQ	= 15,
			BOT_BYTE_LT		= 16,
			BOT_BYTE_UEQ	= 17,
			BOT_BYTE_IIF	= 18,
			BOT_BYTE_ATTEMPT= 19,
			BOT_BYTE_AC		= 20,
			BOT_BYTE_ABS	= 21,
			BOT_BYTE_BUTTON = 22,
			BOT_BYTE_C		= 23,
			BOT_BYTE_DOWN	= 24,
			BOT_BYTE_ECHO	= 25,
			BOT_BYTE_ERROR	= 53,
			BOT_BYTE_FRAME	= 26,
			BOT_BYTE_I		= 27,
			BOT_BYTE_INVALIDS=57,
			BOT_BYTE_J		= 28,
			BOT_BYTE_K		= 29,
			BOT_BYTE_LAST   = 50,
			BOT_BYTE_LEFT	= 30,
			BOT_BYTE_LOOP	= 31,
			BOT_BYTE_LSHIFT = 51,
			BOT_BYTE_MAX	= 54,
			BOT_BYTE_MEM	= 32,
			BOT_BYTE_MEMH	= 33,
			BOT_BYTE_MEML	= 34,
			BOT_BYTE_MEMW	= 35,
			BOT_BYTE_NEVER  = 55,
			BOT_BYTE_OKS    = 56,
			BOT_BYTE_RC		= 37,
			BOT_BYTE_RIGHT	= 36,
			BOT_BYTE_RSHIFT = 52,
			BOT_BYTE_SELECT = 39,
			BOT_BYTE_START	= 38,
			BOT_BYTE_SC		= 40,
			BOT_BYTE_SCORE1 = 58,
			BOT_BYTE_SCORE2 = 59,
			BOT_BYTE_SCORE3 = 60,
			BOT_BYTE_SCORE4 = 61,
			BOT_BYTE_SCORE5 = 62,
			BOT_BYTE_SCORE6 = 63,
			BOT_BYTE_STOP	= 41,
			BOT_BYTE_UP		= 42,
			BOT_BYTE_P		= 43,
			BOT_BYTE_Q		= 44,
			BOT_BYTE_X		= 45,
			BOT_BYTE_Y		= 46,
			BOT_BYTE_Z		= 47;

#define BOT_MAXCOUNTERS 256				// I reckon 256 counters is enough?
static int BotCounter[BOT_MAXCOUNTERS]; // The counters. All ints, maybe change this to
					        			//	another type? Long or something?

static bool EditPlayerOne = true;		// Which player is currently shown in the GUI?
static int BotFrame,					// Which frame is currently or last computed?
	   BotLoopVarI = 0,					// Used for loop(), "i" returns this value.
	   BotLoopVarJ = 0,					// Used for a loop() inside another loop(), "j" 
	   									//	returns this value.
	   BotLoopVarK = 0;					// Used for a loop() inside a already nested 
	   									//	loop(), "k" returns this value.

#define BOT_MAXSCORES 6					// Number of scores that is kept track of
static int BotAttempts, 				// Number of attempts tried so far
	   BotFrames, 						// Number of frames computed so far
	   TotalFrames,						// Total frames computed
	   TotalAttempts,					// Total attempts computed
	   Oks,								// Number of successfull attempts
	   Invalids,						// Number of invalidated attempts
	   Parts,							// Number of parts computed so far
	   BotBestScore[BOT_MAXSCORES]; 				// score, tie1, tie2, tie3, tie4, tie5
static bool NewAttempt;					// Tells code to reset certain run-specific info

static int LastButtonPressed;			// Used for lastbutton command (note that this 
										//	doesnt work for the very first frame of each attempt...)

static uint32 CurrentAttempt[BOT_MAXFRAMES];		// Contains all the frames of the current attempt.
static uint32 BestAttempt[BOT_MAXFRAMES]; 			// Contains all the frames of the best attempt so far.
static uint32 AttemptPointer;			// Points to the last frame of the current attempt.

//static bool ProcessCode = true;			// This boolean tells the code whether or not to process
										//	some code. This is mainly used in branching (iif's)
										//	to skip over the part of the code that is not to be
										//	executed. It still needs to be parsed because the
										//	pointer needs to be moved ahead.

char *BasicBotDir = 0;					// Last used load/save dir is put in this var

static bool EvaluateError = false;		// Indicates whether an error was thrown.
static int LastEvalError;         		// Last code thrown by the parser.

char * GlobalCurrentChar;         		// This little variable will hold the position of the
	                                    //	string we are currently evaluating. I'm pretty sure
	                                    //	Luke used a structure of two arguments, where the
	                                    //	second argument somehow points to the currentchar
	                                    //	pointer (although I can't find where this happens
	                                    //	anywhere in his source) to remember increments 
	                                    //	made in a recursive higher depth (since these
	                                    //	were local variables, going back a depth would mean
	                                    //	loosing the changes). I think he used the second
	                                    //	argument to alter the original pointer. Since there
	                                    //	is only one evaluation-job going on at any single
	                                    //	moment, I think it's easier (better legible) to use
	                                    //	a global variable. Hence, that's what this is for.

int GlobalCurrentCode;					// Basically the same as the above, but for parsing bytecode

bool UpdateFromGUI = false;				// When this flag is set, the next new attempt will reload
										//	the settings from gui to mem. note that this cannot set
										//	the settings for both players at once... maybe later.

static int X,Y,Z,P,Prand,Q,Qrand;       // Static variables (is this a contradiction?) and
                                        //	probabilities. XYZ remain the same, PQ return randomly a
                                        //	number in the range of 0-P or 0-Q. They can be used in
                                        //	the code like any other command ("5+X echo"). The button
                                        //	above their input fields will update these values only (!)
                                        //	The rand values are updated after each frame. This results
                                        //	in all the P and Q values being equal in every usage, but
                                        //	random for each frame.
                                        //	The values are only evaluated when reading them from the
                                        //	GUI (so when starting a new run or when refreshing). This
                                        //	means you can use any code here, but the static won't be
                                        //	updated every frame. This is the intention of the variable.

#define BOT_MAXPROB	1000				// The bot uses probabilities to determine whether to push a
										//	button or not. The odds can be between 0 (for never) and
										//	BOT_MAXPROB (for always/certain). This is also the value
										//	comparisons return when "true", because then it's "certain"
										//	This used to be (if it changed) 1000.

FILE * LogFile;							// The handle to the logfile

struct lastPart {
	int part;
	int attempt;
	int frames;
	int Tattempts;
	int Tframes;
	int counters[BOT_MAXCOUNTERS];
	int score;
	int tie1;
	int tie2;
	int tie3;
	int tie4;
	int tie5;
	int lastbutton;
};
lastPart * LastPart;
lastPart * BestPart; // temporarily save stuff here until current part is done. then replace lastpart by bestpart.
lastPart * NewPart(int part, int attempt, 
			   int frames, int score, int tie1, 
			   int tie2, int tie3, int tie4, 
			   int tie5, int lastbutton)
{
	lastPart * l = new lastPart();
	(*l).attempt = attempt;
	(*l).frames = frames;
	(*l).score = score;
	(*l).tie1 = tie1;
	(*l).tie2 = tie2;
	(*l).tie3 = tie3;
	(*l).tie4 = tie4;
	(*l).tie5 = tie5;
	(*l).lastbutton = lastbutton;
	for (int i=0; i<BOT_MAXCOUNTERS; ++i)
		(*l).counters[i] = BotCounter[i];

	return l;
}
static void SeedRandom(uint32 seed)
{
	rand1 = seed;
	if(rand1 == 0) rand1 = 1; // luke: I forget if this is necessary.
}

/**
 * Get the length of a textfield ("edit control") or textarea
 * It's more of a macro really.
 * Returns 0 when an error occurs, the length of current 
 * contents in any other case (check LastError in case of 0)
 */
unsigned int FieldLength(HWND winhandle,int controlid)
{
	HWND hwndItem = GetDlgItem(winhandle,controlid);
	return SendMessage(hwndItem,WM_GETTEXTLENGTH,0,0);
}
/**
 * Get the text from a freaking textfield/textarea
 * Will reserve new memory space. These strings need
 * to be "delete"d when done with them!!
 **/
char * GetText(HWND winhandle, int controlid)
{
	unsigned int count = FieldLength(hwndBasicBot,controlid);
	char *t = new char[count+1];
	GetDlgItemTextA(hwndBasicBot,controlid,t,count+1);
	return t;
}

/**
 * copies "len" chars or until a '\0' from the
 * old string to a new buffer and terminates 
 * this buffer with a '\0'
 **/
char * ToString(char * old, int len)
{
	char * temp = new char[len+1];
	for (int i=0; i<len; ++i) 
	{
		if (old[i] == 0) break;
		temp[i] = old[i];
	}
	temp[len] = '\0';
	return temp;
}
/**
 * Get the len of a string, which is the position
 * of the first '\0' from the start
 **/
unsigned int GetLength(char * str)
{
	char * was = str;
	while (*str != 0) ++str;
	return str-was;
}
/**
 * luke: Simulation-quality random numbers: taus88 algorithm
 **/
static unsigned int NextRandom(void)
{
	uint32 b;
	b = (((rand1 << 13) ^ rand1) >> 19);
	rand1 = (((rand1 & 4294967294U) << 12) ^ b);
	b = (((rand2 << 2) ^ rand2) >> 25);
	rand2 = (((rand2 & 4294967288U) << 4) ^ b);
	b = (((rand3 << 3) ^ rand3) >> 11);
	rand3 = (((rand3 & 4294967280U) << 17) ^ b);
	return (rand1 ^ rand2 ^ rand3);
}
/**
 * Get a random number, from 0 to n (inclusive...)
 */
static int GetRandom(int n)
{
	if (n == 0) return n; // Zero devision exception catch :)
	return (int) (NextRandom() % n);
}

/**
 * Print a string
 **/
static char * debugS(char * s)
{
	SetDlgItemTextA(hwndBasicBot,GUI_BOT_DEBUG,s);
	return s;
}

/**
 * Print a number
 **/
static int debug(int n)
{
     SetDlgItemInt(hwndBasicBot,GUI_BOT_DEBUG,n,TRUE);
	 return n;
}
/**
 * When an error occurs, call this method to print the errorcode
 **/
static int error(int n)
{
     SetDlgItemInt(hwndBasicBot,GUI_BOT_ERROR,n,TRUE);
	 return n;
}


/**
 * All syntax-errors lead here.
 **/
static void BotSyntaxError(int errorcode)
{
     EvaluateError = true;
     LastEvalError = errorcode;
     error(errorcode);
     StopBasicBot();
}

/**
 * Convert string code to byte code
 * This function is NOT called recursively!
 * - formula is the string to parse
 * - codefield is the field for which this code is meant (used for debugging)
 **/
static int * ToByteCode(char * formula, int codefield)
{
	// get the len of the formula
	int len = GetLength(formula);
	// create some new space with the len of the formula
	// its very unlikely that the bytecode will be any
	// longer then the formula, the only exception is a
	// formula with a single digit. for this reason
	// we'll add a 10 byte padding
	#define EXTRABYTES 30
	int * bytes = new int[len+EXTRABYTES];
	memset(bytes,0,sizeof(int)*(len+EXTRABYTES));
	int pointer = 0;

	// reset values
    EvaluateError = false;
    // set the global pointer to the string at the start of the string
    GlobalCurrentChar = formula;

    // first we want to quickly check whether the brackets are matching or not
    // same for ? and :, both are pairs and should not exist alone, also they
	// can not appear out of order (so no "?;:" ";?:" or something)
	// also, 
    char * quickscan = formula;
	// to print an errormsg
	char errormsg[200];
    // counters
    int iifs = 0, iifs2 = 0, bracks = 0;
    // until EOL
    while (*quickscan != 0 && !EvaluateError)
    {
    	switch(*quickscan)
    	{
			case 'l': if (*(quickscan+1)=='o'&&*(quickscan+2)=='o'&&*(quickscan+3)=='p') ++iifs2; break;
    		case '?': ++iifs; break;
    		case ':': 
				--iifs;
				++iifs2;
				if (iifs < 0) BotSyntaxError(1004);
				break;
			case ';': 
				--iifs2; 
				if (iifs2 < 0) BotSyntaxError(1004);
				break;
    		case '(': ++bracks; break;
    		case ')': 
				--bracks; 
				if (bracks < 0) BotSyntaxError(1004);
				break;
    		default: break;
    	}
    	++quickscan;
    }
    if (EvaluateError || iifs || bracks || iifs2)
	{
    	BotSyntaxError(1004);
		sprintf(errormsg,"[%d] ?:;() dont match!",codefield);
		debugS(errormsg);
		delete bytes;
		return NULL;
	}
    
	bool hexmode = false;
	bool negmode = false;

	// two stacks, used to branch properly
	// code adds current pos to the stack if one of the commands
	// is encountered. once a proper jump point is reached, put
	// the location back at the position that's on the top of the
	// stack (popping it off). for the iif, this means first 
	// pushing the qmarkstack, when reaching the colon pop the
	// qmarkstack and put the position there, then push the
	// colonstack. when the end is reached, pop the colonstack and
	// put the position there. this allows easy jumping. loop just
	// needs to jump over its argument when it has looped enough.
	int qmarkstack[100];
	int colonstack[100];
	// stack-counters
	int qpoint=-1,cpoint=-1;

	char * pointerStart = GlobalCurrentChar;

	while(*GlobalCurrentChar != 0 && !EvaluateError)
	{
		switch(*GlobalCurrentChar)
		{
		case 0:
			// stringterminator, end of code
			// no more input, so return
			break; 
		case '(':
			++GlobalCurrentChar;
			bytes[pointer++] = BOT_BYTE_LB;
			break;
		case ')':
			++GlobalCurrentChar;
			bytes[pointer++] = BOT_BYTE_RB;
			break;
		case 'x':
			++GlobalCurrentChar;
			hexmode = true;
			break;
		case '+':
			++GlobalCurrentChar;
			bytes[pointer++] = BOT_BYTE_ADD;
			break;
		case '-':
			++GlobalCurrentChar;
			bytes[pointer++] = BOT_BYTE_SUB;
			break;
		case '*':
			++GlobalCurrentChar;
			bytes[pointer++] = BOT_BYTE_MUL;
			break;
		case '/':
			++GlobalCurrentChar;
			bytes[pointer++] = BOT_BYTE_DIV;
			break;
		case '%':
			++GlobalCurrentChar;
			bytes[pointer++] = BOT_BYTE_MOD;
			break;
		case '&':
			++GlobalCurrentChar;
			bytes[pointer++] = BOT_BYTE_AND;
			break;
		case '|':
			++GlobalCurrentChar;
			bytes[pointer++] = BOT_BYTE_OR;
			break;
		case '^':
			++GlobalCurrentChar;
			bytes[pointer++] = BOT_BYTE_XOR;
			break;
		case '=':
			++GlobalCurrentChar;
			bytes[pointer++] = BOT_BYTE_EQ;
			break;
		case '>':
			if(*(GlobalCurrentChar + 1) == '=')
			{
                GlobalCurrentChar += 2;
				bytes[pointer++] = BOT_BYTE_GTEQ;
            }
			else if(*(GlobalCurrentChar + 1) == '>')
			{
				GlobalCurrentChar += 2;
				bytes[pointer++] = BOT_BYTE_RSHIFT;
			}
			else
			{
			    ++GlobalCurrentChar;
				bytes[pointer++] = BOT_BYTE_GT;
            }
			break;
		case '<':
			if(*(GlobalCurrentChar + 1) == '=') 
            {
                GlobalCurrentChar += 2;
				bytes[pointer++] = BOT_BYTE_LTEQ;
            }
			else if(*(GlobalCurrentChar + 1) == '<')
			{
				GlobalCurrentChar += 2;
				bytes[pointer++] = BOT_BYTE_LSHIFT;
			}
			else
			{
			    ++GlobalCurrentChar;
				bytes[pointer++] = BOT_BYTE_LT;
            }
			break;
		case '!':
			if(*(GlobalCurrentChar + 1) == '=')
			{
                GlobalCurrentChar += 2;
				bytes[pointer++] = BOT_BYTE_UEQ;
			}
			else
			{
				BotSyntaxError(1001); 	// "!" is not a valid command, but perhaps we could 
          						  		//	use it as a prefix, checking if the next is 1000
		          				  		//	(or 0) or not and inverting it. For now error it.
				sprintf(errormsg,"[%d] Unknown command (!...)",codefield);
				debugS(errormsg);
				delete bytes;
				return NULL;
			}
			break;
		case '?':
            ++GlobalCurrentChar;
			// the bytecode will look similar to this:
			//     xyz?1abcd:2efghi;3
			// 1 will contain the adres of 2 (+1), 2 contains the adres of 3
			// if true, it skips over 1, jumping to 3 when reaching :
			// else it just jumps to 2
			// this is basic branching using a stack :)
			bytes[pointer++] = BOT_BYTE_IIF;
			qmarkstack[++qpoint] = pointer++;
			break;
		case ':':
			{
			++GlobalCurrentChar;
			// colon encountered
			bytes[pointer++] = BOT_BYTE_COLON;
			// next position will contain jump to end of second statement
			colonstack[++cpoint] = pointer++;
			// put the number of positions after this target as jump target after the ?
			int qmarkpos = qmarkstack[qpoint--];
			bytes[qmarkpos] = pointer - qmarkpos;
			break;
			}
		case ';':
			{
			++GlobalCurrentChar;
			bytes[pointer++] = BOT_BYTE_SCOLON;
			// semicolon encountered
			// had to use the semicolon to simplify determining end of second arg...
			// next position is the target of the colon
			int colonpos = colonstack[cpoint--];
			bytes[colonpos] = pointer - colonpos;
			break;
			}
        case 10:
        case 13:
        case 9:
		case ' ':
            ++GlobalCurrentChar;
			break;
		case 'a':
			if(*(GlobalCurrentChar+1) == 't'
				&&*(GlobalCurrentChar+2) == 't'
				&&*(GlobalCurrentChar+3) == 'e'
				&&*(GlobalCurrentChar+4) == 'm'
				&&*(GlobalCurrentChar+5) == 'p'
				&&*(GlobalCurrentChar+6) == 't')
			{
				// attempt
                GlobalCurrentChar += 7;
				bytes[pointer++] = BOT_BYTE_ATTEMPT;
			}
			else if((*(GlobalCurrentChar+1) == 'd'
				&&*(GlobalCurrentChar+2) == 'd'
				&&*(GlobalCurrentChar+3) == 'c'
				&&*(GlobalCurrentChar+4) == 'o'
				&&*(GlobalCurrentChar+5) == 'u'
				&&*(GlobalCurrentChar+6) == 'n'
				&&*(GlobalCurrentChar+7) == 't'
				&&*(GlobalCurrentChar+8) == 'e'
				&&*(GlobalCurrentChar+9) == 'r'
				&&*(GlobalCurrentChar+10)== '(') ||
				(*(GlobalCurrentChar+1) == 'c'
				&&*(GlobalCurrentChar+2) == '('))
			{
                // addcounter() ac()
				GlobalCurrentChar += (*(GlobalCurrentChar+1) == 'd') ? 11 : 3;
				bytes[pointer++] = BOT_BYTE_AC;
			}
			else if(*(GlobalCurrentChar+1) == 'b'
				&&*(GlobalCurrentChar+2) == 's'
				&&*(GlobalCurrentChar+3) == '(')
			{
                // abs()
				GlobalCurrentChar += 4;
				bytes[pointer++] = BOT_BYTE_ABS;
			}
			else if(*(GlobalCurrentChar+1) == 'l'
				&&*(GlobalCurrentChar+2) == 'w'
				&&*(GlobalCurrentChar+3) == 'a'
				&&*(GlobalCurrentChar+4) == 'y'
				&&*(GlobalCurrentChar+5) == 's')
			{
                // always (=max)
				GlobalCurrentChar += 6;
				bytes[pointer++] = BOT_BYTE_MAX;
			}
			else
			{
                // a
				++GlobalCurrentChar;
				bytes[pointer++] = BOT_BYTE_LIT;
				bytes[pointer++] = 1;
			}
			break;
		case 'b':
			if(*(GlobalCurrentChar+1) == 'u'
				&&*(GlobalCurrentChar+2) == 't'
				&&*(GlobalCurrentChar+3) == 't'
				&&*(GlobalCurrentChar+4) == 'o'
				&&*(GlobalCurrentChar+5) == 'n')
			{
                // button
				GlobalCurrentChar += 6;
				bytes[pointer++] = BOT_BYTE_BUTTON;
			}
			else
			{
                // button B
				++GlobalCurrentChar;
				bytes[pointer++] = BOT_BYTE_LIT;
				bytes[pointer++] = 2;
			}
			break;
		case 'c':
			if((*(GlobalCurrentChar+1) == 'o'
				&& *(GlobalCurrentChar+2) == 'u'
				&& *(GlobalCurrentChar+3) == 'n'
				&& *(GlobalCurrentChar+4) == 't'
				&& *(GlobalCurrentChar+5) == 'e'
				&& *(GlobalCurrentChar+6) == 'r'
				&& *(GlobalCurrentChar+7) == '(') ||
				(*(GlobalCurrentChar+1) == '('))
			{
                // counter()
                GlobalCurrentChar += (*(GlobalCurrentChar+1) == 'o') ? 8 : 2;
				bytes[pointer++] = BOT_BYTE_C;
			}
			else
			{
				BotSyntaxError(1001);
				sprintf(errormsg,"[%d] Unknown Command (c...)",codefield);
				debugS(errormsg);
				delete bytes;
				return NULL;
			}
			break;
		case 'd':
			if(*(GlobalCurrentChar+1) == 'o'
				&&*(GlobalCurrentChar+2) == 'w'
				&&*(GlobalCurrentChar+3) == 'n')
			{
                // down
				GlobalCurrentChar += 4;
				bytes[pointer++] = BOT_BYTE_DOWN;
			}
			else
			{
				BotSyntaxError(1001);
				sprintf(errormsg,"[%d] Unknown Command (d...)",codefield);
				debugS(errormsg);
				delete bytes;
				return NULL;
			}
			break;
		case 'e':
			if(*(GlobalCurrentChar+1) == 'c'
				&&*(GlobalCurrentChar+2) == 'h'
				&&*(GlobalCurrentChar+3) == 'o')
			{
				GlobalCurrentChar += 4;
				bytes[pointer++] = BOT_BYTE_ECHO;
    			break;
			}
			if(*(GlobalCurrentChar+1) == 'r'
				&&*(GlobalCurrentChar+2) == 'r'
				&&*(GlobalCurrentChar+3) == 'o'
				&&*(GlobalCurrentChar+4) == 'r')
			{
				GlobalCurrentChar += 5;
				bytes[pointer++] = BOT_BYTE_ERROR;
    			break;
			}
			else
			{
				BotSyntaxError(1001);
				sprintf(errormsg,"[%d] Unknown Command (e...)",codefield);
				debugS(errormsg);
				delete bytes;
				return NULL;
			}
		case 'f':
			if(*(GlobalCurrentChar+1) == 'r'
				&& *(GlobalCurrentChar+2) == 'a'
				&& *(GlobalCurrentChar+3) == 'm'
				&& *(GlobalCurrentChar+4) == 'e')
			{
                // frame
				GlobalCurrentChar += 5;
				bytes[pointer++] = BOT_BYTE_FRAME;
			}
			else
			{
				BotSyntaxError(1001);
				sprintf(errormsg,"[%d] Unknown Command (f...)",codefield);
				debugS(errormsg);
				delete bytes;
				return NULL;
			}
			break;
		case 'i':
			if(*(GlobalCurrentChar+1) == 'n'
				&&*(GlobalCurrentChar+2) == 'v'
				&&*(GlobalCurrentChar+3) == 'a'
				&&*(GlobalCurrentChar+4) == 'l'
				&&*(GlobalCurrentChar+5) == 'i'
				&&*(GlobalCurrentChar+6) == 'd'
				&&*(GlobalCurrentChar+7) == 's')
			{
				// invalids
				GlobalCurrentChar += 8;
				bytes[pointer++] = BOT_BYTE_INVALIDS;
			}
			else
			{
				++GlobalCurrentChar;
				bytes[pointer++] = BOT_BYTE_I;
			}
			break;
		case 'j':
			++GlobalCurrentChar;
			bytes[pointer++] = BOT_BYTE_J;
			break;
		case 'k':
			++GlobalCurrentChar;
			bytes[pointer++] = BOT_BYTE_K;
			break;
		case 'l':
			if((*(GlobalCurrentChar+1) == 'b') ||
				 (*(GlobalCurrentChar+1) == 'a'
				&&*(GlobalCurrentChar+2) == 's'
				&&*(GlobalCurrentChar+3) == 't'
				&&*(GlobalCurrentChar+4) == 'b'
				&&*(GlobalCurrentChar+5) == 'u'
				&&*(GlobalCurrentChar+6) == 't'
				&&*(GlobalCurrentChar+7) == 't'
				&&*(GlobalCurrentChar+8) == 'o'
				&&*(GlobalCurrentChar+9) == 'n'))
			{
				// lastbutton lb
				GlobalCurrentChar += (*(GlobalCurrentChar+1) == 'b') ? 2 : 10;
				bytes[pointer++] = BOT_BYTE_LAST;
			}
			else if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 'f'
				&&*(GlobalCurrentChar+3) == 't')
			{
                // left
				GlobalCurrentChar += 4;
				bytes[pointer++] = BOT_BYTE_LEFT;
			}
			else if(*(GlobalCurrentChar+1) == 'o'
				&&*(GlobalCurrentChar+2) == 'o'
				&&*(GlobalCurrentChar+3) == 'p')
			{
                // loop
				// loop and iif share the same limiter ';'
				// and since you cant desync nesting them anyways, its
				// safe to use the colonstack to add the loops, because
				// if a loop is added while inside a colon-semicolon
				// statement, it needs to be closed before leaving this
				// block anyways. (make sure the code checks for this!)
				GlobalCurrentChar += 4;
				bytes[pointer++] = BOT_BYTE_LOOP;
				colonstack[++cpoint] = pointer++;
			}
			else
			{
				BotSyntaxError(1001);
				sprintf(errormsg,"[%d] Unknown Command (l...)",codefield);
				debugS(errormsg);
				delete bytes;
				return NULL;
			}
			break;
		case 'm':
			if(*(GlobalCurrentChar+1) == 'a'
				&&*(GlobalCurrentChar+2) == 'x')
			{
				// max
				GlobalCurrentChar += 3;
				bytes[pointer++] = BOT_BYTE_MAX;
			}
			else if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 'm'
				&&*(GlobalCurrentChar+3) == '(')
			{
                // mem()
                GlobalCurrentChar += 4;
				bytes[pointer++] = BOT_BYTE_MEM;
			}
			else if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 'm'
				&&*(GlobalCurrentChar+3) == 'h'
				&&*(GlobalCurrentChar+4) == '(')
			{
                // memh()
                GlobalCurrentChar += 5;
				bytes[pointer++] = BOT_BYTE_MEMH;
			}
			else if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 'm'
				&&*(GlobalCurrentChar+3) == 'l'
				&&*(GlobalCurrentChar+4) == '(')
			{
                // meml()
                GlobalCurrentChar += 5;
				bytes[pointer++] = BOT_BYTE_MEML;
			}
			else if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 'm'
				&&*(GlobalCurrentChar+3) == 'w'
				&&*(GlobalCurrentChar+4) == '(')
			{
                // memw()
                GlobalCurrentChar += 5;
				bytes[pointer++] = BOT_BYTE_MEMW;
			}
			else
			{
				BotSyntaxError(1001);
				sprintf(errormsg,"[%d] Unknown Command (m...)",codefield);
				debugS(errormsg);
				delete bytes;
				return NULL;
			}
			break;
		case 'n':
			if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 'v'
				&&*(GlobalCurrentChar+3) == 'e'
				&&*(GlobalCurrentChar+4) == 'r'||
				*(GlobalCurrentChar+1) == 'o')
			{
                // never no
                GlobalCurrentChar += (*(GlobalCurrentChar+1) == 'e') ? 5 : 2;
				bytes[pointer++] = BOT_BYTE_NEVER;
			}
			else
			{
				// this is a little sketchy, but the principle works.
				++GlobalCurrentChar;
				negmode = true;
			}
			break;
		case 'o':
			if(*(GlobalCurrentChar+1) == 'k'
				&&*(GlobalCurrentChar+2) == 's')
			{
                // oks
                GlobalCurrentChar += 3;
				bytes[pointer++] = BOT_BYTE_OKS;
			}
			else
			{
				BotSyntaxError(11001);
				sprintf(errormsg,"[%d] Unknown Command (o...)",codefield);
				debugS(errormsg);
				delete bytes;
				return NULL;
			}
			break;
		case 'r':
			if(*(GlobalCurrentChar+1) == 'i'
				&&*(GlobalCurrentChar+2) == 'g'
				&&*(GlobalCurrentChar+3) == 'h'
				&&*(GlobalCurrentChar+4) == 't')
			{
                // right
                GlobalCurrentChar += 5;
				bytes[pointer++] = BOT_BYTE_RIGHT;
			}
			else if((*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 's'
				&&*(GlobalCurrentChar+3) == 'e'
				&&*(GlobalCurrentChar+4) == 't'
				&&*(GlobalCurrentChar+5) == 'c'
				&&*(GlobalCurrentChar+6) == 'o'
				&&*(GlobalCurrentChar+7) == 'u'
				&&*(GlobalCurrentChar+8) == 'n'
				&&*(GlobalCurrentChar+9) == 't'
				&&*(GlobalCurrentChar+10) == 'e'
				&&*(GlobalCurrentChar+11) == 'r'
				&&*(GlobalCurrentChar+12) == '('
                )||(
                  *(GlobalCurrentChar+1) == 'c'
				&&*(GlobalCurrentChar+2) == '('))
			{
                // resetcounter() rc()
                GlobalCurrentChar += (*(GlobalCurrentChar+1) == 'e') ? 13 : 3;
				bytes[pointer++] = BOT_BYTE_RC;
			}
			else
			{
				BotSyntaxError(1001);
				sprintf(errormsg,"[%d] Unknown Command (r...)",codefield);
				debugS(errormsg);
				delete bytes;
				return NULL;
			}
			break;
		case 's':
			if(*(GlobalCurrentChar+1) == 't'
				&&*(GlobalCurrentChar+2) == 'a'
				&&*(GlobalCurrentChar+3) == 'r'
				&&*(GlobalCurrentChar+4) == 't')
			{
                // start
                GlobalCurrentChar += 5;
				bytes[pointer++] = BOT_BYTE_START;
			}
			else if(*(GlobalCurrentChar+1) == 'c'
				&&*(GlobalCurrentChar+2) == 'o'
				&&*(GlobalCurrentChar+3) == 'r'
				&&*(GlobalCurrentChar+4) == 'e'
				&&(*(GlobalCurrentChar+5) == '1' || 
				   *(GlobalCurrentChar+5) == '2' || 
				   *(GlobalCurrentChar+5) == '3' || 
				   *(GlobalCurrentChar+5) == '4' || 
				   *(GlobalCurrentChar+5) == '5' ||
				   *(GlobalCurrentChar+5) == '6'))
			{
                // score 1-6
				// returns the current max tie 1-6
				switch ((int)*(GlobalCurrentChar+5)) {
					case '1': bytes[pointer++] = BOT_BYTE_SCORE1; break;
					case '2': bytes[pointer++] = BOT_BYTE_SCORE2; break;
					case '3': bytes[pointer++] = BOT_BYTE_SCORE3; break;
					case '4': bytes[pointer++] = BOT_BYTE_SCORE4; break;
					case '5': bytes[pointer++] = BOT_BYTE_SCORE5; break;
					case '6': bytes[pointer++] = BOT_BYTE_SCORE6; break;
					default: 
						sprintf(errormsg,"[%d] Serious error here",codefield);
						debugS(errormsg);
				}
				GlobalCurrentChar += 6;
            }
			else if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 'l'
				&&*(GlobalCurrentChar+3) == 'e'
				&&*(GlobalCurrentChar+4) == 'c'
				&&*(GlobalCurrentChar+5) == 't')
			{
                // select
                GlobalCurrentChar += 6;
				bytes[pointer++] = BOT_BYTE_SELECT;
			}
			// why was there no short version for setcounter()? There is now... :)
			else if ((*(GlobalCurrentChar+1) == 'c') ||
				(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 't'
				&&*(GlobalCurrentChar+3) == 'c'
				&&*(GlobalCurrentChar+4) == 'o'
				&&*(GlobalCurrentChar+5) == 'u'
				&&*(GlobalCurrentChar+6) == 'n'
				&&*(GlobalCurrentChar+7) == 't'
				&&*(GlobalCurrentChar+8) == 'e'
				&&*(GlobalCurrentChar+9) == 'r'
				&&*(GlobalCurrentChar+10) == '('))
			{
                // setcounter() sc()
				GlobalCurrentChar += (*(GlobalCurrentChar+1) == 'c') ? 3 : 11;
				bytes[pointer++] = BOT_BYTE_SC;
			}
			// qfox: Added stop command. Should do exactly the same as pressing the "stop" button.
			//	 But probably won't yet :)
			else if(*(GlobalCurrentChar+1) == 't'
				&&*(GlobalCurrentChar+2) == 'o'
				&&*(GlobalCurrentChar+3) == 'p')
			{
                // stop
                GlobalCurrentChar += 4;
				bytes[pointer++] = BOT_BYTE_STOP;
            }
			else
			{
				BotSyntaxError(10001);
				sprintf(errormsg,"[%d] Unknown Command (s...)",codefield);
				debugS(errormsg);
				delete bytes;
				return NULL;
			}
			break;
		case 'u':
			if(*(GlobalCurrentChar+1) == 'p')
			{
                // up
                GlobalCurrentChar += 2;
				bytes[pointer++] = BOT_BYTE_UP;
			}
			else
			{
				BotSyntaxError(11001);
				sprintf(errormsg,"[%d] Unknown Command (u...)",codefield);
				debugS(errormsg);
				delete bytes;
				return NULL;
			}
			break;
		case 'v':
			if(*(GlobalCurrentChar+1) == 'a'
				&&*(GlobalCurrentChar+2) == 'l'
				&&*(GlobalCurrentChar+3) == 'u'
				&&*(GlobalCurrentChar+4) == 'e')
			{
				// value
				// "value" is completely transparent, more like a nop (or no-operation).
				//	could be used in a iif where there is no else, or to make it more
				//	clear how value is used internally...
				GlobalCurrentChar += 5;
			}
			else
			{
				BotSyntaxError(1001);
				sprintf(errormsg,"[%d] Unknown Command (v...)",codefield);
				debugS(errormsg);
				delete bytes;
				return NULL;
			}
			break;
		case 'y':
			if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 's')
			{
				// yes
				GlobalCurrentChar += 3;
				bytes[pointer++] = BOT_BYTE_MAX;
			}
			else
			{
				BotSyntaxError(1001);
				sprintf(errormsg,"[%d] Unknown Command (y...)",codefield);
				debugS(errormsg);
				delete bytes;
				return NULL;
			}
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			// Not quite sure how the original literals worked, but it doesn't work in my 
			//	 implementation so I had to rewrite it. When a digit is encountered it will
			//	 start reading character (starting at the first). "value" is multiplied by 10
			//	 and the next digit is added to it. When a non-digit char is encountered, the
			//	 code stops parsing this number and behaves like everywhere else. (returning
			//   it if ret is true, breaking else).
			//	 If hexmode, this is done below, taking note of the additional "numbers".
			if (!hexmode) 
			{
				// keep on reading input until no more numbers are encountered (0-9)
				int ahead = 0;
				int value = 0;
				while (*(GlobalCurrentChar+ahead) >= '0' && *(GlobalCurrentChar+ahead) <= '9') {
					if (negmode) value = (value*10)-(*(GlobalCurrentChar+ahead) - '0');
					else value = (value*10)+(*(GlobalCurrentChar+ahead) - '0');
					++ahead;
				}
				// I don't think ahead needs to be incremented before adding it to
				//	 currentchar, because the while increments it for _every_ character.
				GlobalCurrentChar += ahead;
				bytes[pointer++] = BOT_BYTE_LIT;
				bytes[pointer++] = value;
				negmode = false;
				break;
			}
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			// Hexmode does things a liiiitle differently. We need to multiply each number 
			//	 by 16 instead of 10, and if the character is ABCDEF we need to subtract 'A'
			//	 and add 10, instead of just subtracting '0'. Otherwise same principle.
			if(hexmode)
			{
				// Keep on reading input until no more numbers are encountered (0-9)
				int ahead = 0;
				int value = 0;
				// number-size check (int is max) -> do it when converting to bytecode
				while ((*(GlobalCurrentChar+ahead) >= '0' && *(GlobalCurrentChar+ahead) <= '9')
					|| (*(GlobalCurrentChar+ahead) >= 'A' && *(GlobalCurrentChar+ahead) <= 'F')) {
                    // next code is a little redundant, but it's either redundant or even harder to read.
					if (*(GlobalCurrentChar+ahead) >= '0' && *(GlobalCurrentChar+ahead) <= '9')
						if (negmode) value = (value*16)-(*(GlobalCurrentChar+ahead) - '0');
						else value = (value*16)+(*(GlobalCurrentChar+ahead) - '0');
					else
						if (negmode) value = (value*16)-((*(GlobalCurrentChar+ahead) - 'A')+10);
						else value = (value*16)+((*(GlobalCurrentChar+ahead) - 'A')+10);
					++ahead;
				}
				GlobalCurrentChar += ahead;
				bytes[pointer++] = BOT_BYTE_LIT;
				bytes[pointer++] = value;
				negmode = false;
				hexmode = false;
			}
			else
			{
				// If not in hexmode, we cant use these characters.
				BotSyntaxError(1003);
				sprintf(errormsg,"[%d] Not in hexmode!",codefield);
				debugS(errormsg);
				delete bytes;
				return NULL;
			}
			break;
		case 'P':
			++GlobalCurrentChar;
			bytes[pointer++] = BOT_BYTE_P;
			break;
		case 'Q':
			++GlobalCurrentChar;
			bytes[pointer++] = BOT_BYTE_Q;
			break;
		case 'X':
			++GlobalCurrentChar;
			bytes[pointer++] = BOT_BYTE_X;
			break;
		case 'Y':
			++GlobalCurrentChar;
			bytes[pointer++] = BOT_BYTE_Y;
			break;
		case 'Z':
			++GlobalCurrentChar;
			bytes[pointer++] = BOT_BYTE_Z;
			break;
		default:
			// Unknown characters should error out. You can use spaces.
			BotSyntaxError(12001);
			sprintf(errormsg,"[%d][%d] Unknown character... '%c'",codefield,int(GlobalCurrentChar-pointerStart),(*GlobalCurrentChar));
			debugS(errormsg);
			delete bytes;
			return NULL;
		}
	}
	return bytes;
}


/**
 * The bytecode interpreter
 */
static int Interpreter(bool single)
{
	int value = 0;
	int nextlit;
	while (Bytecode[ByteCodeField][GlobalCurrentCode] != 0)
	{
		switch (Bytecode[ByteCodeField][GlobalCurrentCode++])
		{
		case 0:
			break;
		case BOT_BYTE_LB:
			value = Interpreter(false);
			if (single) return value;
			break;
		case BOT_BYTE_RB:
			return value;
		case BOT_BYTE_LIT:
			value = Bytecode[ByteCodeField][GlobalCurrentCode++];
			if (single) return value;
			break;
		case BOT_BYTE_ADD:
			value += Interpreter(true);
			if (single) return value;
			break;
		case BOT_BYTE_SUB:
			value -= Interpreter(true);
			if (single) return value;
			break;
		case BOT_BYTE_MUL:
			value *= Interpreter(true);
			if (single) return value;
			break;
		case BOT_BYTE_DIV:
			{
				// we need to set up a zero-division-exception-trap here
			int zerotrap = Interpreter(true);
			if (value == 0 || zerotrap == 0)
				value = 0;
			else
				value /= zerotrap;
			if (single) return value;
			break;
			}
		case BOT_BYTE_MOD:
			{
			// we need to set up a zero-division-exception-trap here
			int zerotrap = Interpreter(true);
			if (value == 0 || zerotrap == 0)
				value = 0;
			else
				value %= zerotrap;
			if (single) return value;
			break;
			}
		case BOT_BYTE_AND:
			value &= Interpreter(true);
			if (single) return value;
			break;
		case BOT_BYTE_OR:
			value |= Interpreter(true);
			if (single) return value;
			break;
		case BOT_BYTE_XOR:
			value ^= Interpreter(true);
			if (single) return value;
			break;
		case BOT_BYTE_EQ:
			value = ((value == Interpreter(true)) ? BOT_MAXPROB : 0);
			if (single) return value;
			break;
		case BOT_BYTE_GTEQ:
			value = ((value >= Interpreter(true)) ? BOT_MAXPROB : 0);
			if (single) return value;
			break;
		case BOT_BYTE_GT:
			value = ((value > Interpreter(true)) ? BOT_MAXPROB : 0);
			if (single) return value;
			break;
		case BOT_BYTE_LTEQ:
			value = ((value <= Interpreter(true)) ? BOT_MAXPROB : 0);
			if (single) return value;
			break;
		case BOT_BYTE_LT:
			value = ((value < Interpreter(true)) ? BOT_MAXPROB : 0);
			if (single) return value;
			break;
		case BOT_BYTE_UEQ:
			value = ((value != Interpreter(true)) ? BOT_MAXPROB : 0);
			if (single) return value;
			break;
		case BOT_BYTE_IIF:
			if (value == 0)
				GlobalCurrentCode += Bytecode[ByteCodeField][GlobalCurrentCode];
			else
				GlobalCurrentCode += 1;
			value = Interpreter(false);
			break;
		case BOT_BYTE_COLON:
			// move the pointer beyond the "else" part
			GlobalCurrentCode += Bytecode[ByteCodeField][GlobalCurrentCode];
			return value;
		case BOT_BYTE_SCOLON:
			// limiter for iif and loop
			return value;
		case BOT_BYTE_ATTEMPT:
			value = BotAttempts;
			if (single) return value;
			break;
		case BOT_BYTE_AC:
			value = (BotCounter[(Interpreter(false) & 0x7FFFFFFF) % BOT_MAXCOUNTERS] += value);
			if (single) return value;
			break;
		case BOT_BYTE_ABS:
			value = Interpreter(false);
			if (value < 0) 
				value = (0 - value);
			if (single) return value;
			break;
		case BOT_BYTE_BUTTON:
			if (value == 0) value = BotInput[1];
			else value &= BotInput[1];
			if (single) return value;
			break;
		case BOT_BYTE_C:
			value = BotCounter[(Interpreter(false) & 0x7FFFFFFF) % BOT_MAXCOUNTERS];
			if (single) return value;
			break;
		case BOT_BYTE_DOWN:
			value = 32;
			if (single) return value;
			break;
		case BOT_BYTE_ECHO:
			debug(value);
			break;
		case BOT_BYTE_ERROR:
			error(value);
			break;
		case BOT_BYTE_FRAME:
			value = BotFrame;
			if (single) return value;
			break;
		case BOT_BYTE_I:
			value = BotLoopVarI;
			if (single) return value;
			break;
		case BOT_BYTE_INVALIDS:
			value = Invalids;
			if (single) return value;
			break;
		case BOT_BYTE_J:
			value = BotLoopVarJ;
			if (single) return value;
			break;
		case BOT_BYTE_K:
			value = BotLoopVarK;
			if (single) return value;
			break;
		case BOT_BYTE_LAST:
			value = LastButtonPressed;
			if (single) return value;
			break;
		case BOT_BYTE_LEFT:
			value = 64;
			if (single) return value;
			break;
		case BOT_BYTE_LOOP:
			if (value != 0)
			{
				int tel = value;
				// increase the position of the codepointer first
				// then back it up
				// at the end of the loop, go back one to get the
				// position where the code continues (after loop)
				int pos = ++GlobalCurrentCode;
				if (BotLoopVarI == 0)
				{
					for (; BotLoopVarI < tel; ++BotLoopVarI)
					{
						Interpreter(false);
						GlobalCurrentCode = pos;
					}
					BotLoopVarI = 0;
				}
				else if (BotLoopVarJ == 0)
				{
					for (; BotLoopVarJ < tel; ++BotLoopVarJ)
					{
						Interpreter(false);
						GlobalCurrentCode = pos;
					}
					BotLoopVarJ = 0;
				}
				else if (BotLoopVarK == 0)
				{
					for (; BotLoopVarK < tel; ++BotLoopVarK)
					{
						Interpreter(false);
						GlobalCurrentCode = pos;
					}
					BotLoopVarK = 0;
				}
				else
				{
					// well... this should never happen, if the encoder
					// checks for a max loop depth of 3...
					MessageBoxA(hwndBasicBot, "wtf1", "oopsie", MB_OK);
				}
				// go back one and fetch the position of where to continue
				GlobalCurrentCode += Bytecode[ByteCodeField][GlobalCurrentCode-1]-1;
			}
			else 
			{
				GlobalCurrentCode += Bytecode[ByteCodeField][GlobalCurrentCode];
			}
			if (single) return value;
			break;
		case BOT_BYTE_LSHIFT:
			value <<= Interpreter(true);
			if (single) return value;
			break;
		case BOT_BYTE_MAX:
			value = BOT_MAXPROB;
			if (single) return value;
			break;
		case BOT_BYTE_MEM:
			nextlit = (Interpreter(false) & 0x7FFFFFFF) % 65536;
			value = ARead[nextlit](nextlit);
			if (single) return value;
			break;
		case BOT_BYTE_MEMH:
			nextlit = (Interpreter(false) & 0x7FFFFFFF) % 65536;
			value = (ARead[nextlit](nextlit) & 0xF0) >> 8;
			if (single) return value;
			break;
		case BOT_BYTE_MEML:
			nextlit = (Interpreter(false) & 0x7FFFFFFF) % 65536;
			value = ARead[nextlit](nextlit) & 0x0F;
			if (single) return value;
			break;
		case BOT_BYTE_MEMW:
			{
			int nextlit = Interpreter(false);
            int v1 = (nextlit & 0x7FFFFFFF) % 65536;
            int v2 = ((nextlit+1) & 0x7FFFFFFF) % 65536;
            v1 = ARead[v1](v1);
			v2 = ARead[v2](v2);
			value = (v1<<8)+v2;
			if (single) return value;
			break;
			}
		case BOT_BYTE_NEVER:
			value = 0;
			if (single) return value;
			break;
		case BOT_BYTE_OKS:
			value = Oks;
			if (single) return value;
			break;
		case BOT_BYTE_RC:
			value = (BotCounter[(Interpreter(false) & 0x7FFFFFFF) % BOT_MAXCOUNTERS] = 0);
			if (single) return 0;
			break;
		case BOT_BYTE_RIGHT:
			value = 128;
			if (single) return value;
			break;
		case BOT_BYTE_RSHIFT:
			value >>= Interpreter(true);
			if (single) return value;
			break;
		case BOT_BYTE_START:
			value = 8;
			if (single) return value;
			break;
		case BOT_BYTE_SELECT:
			value = 4;
			if (single) return value;
			break;
		case BOT_BYTE_SC:
			BotCounter[(Interpreter(false) & 0x7FFFFFFF) % BOT_MAXCOUNTERS] = value;
			if (single) return value;
			break;
		case BOT_BYTE_SCORE1:
			value = BotBestScore[0];
			if (single) return value;
			break;
		case BOT_BYTE_SCORE2:
			value = BotBestScore[1];
			if (single) return value;
			break;
		case BOT_BYTE_SCORE3:
			value = BotBestScore[2];
			if (single) return value;
			break;
		case BOT_BYTE_SCORE4:
			value = BotBestScore[3];
			if (single) return value;
			break;
		case BOT_BYTE_SCORE5:
			value = BotBestScore[4];
			if (single) return value;
			break;
		case BOT_BYTE_SCORE6:
			value = BotBestScore[5];
			if (single) return value;
			break;
		case BOT_BYTE_STOP:
            // stops the code
            EvaluateError = true;
			MessageBox(hwndBasicBot, "Stopped by code!", "Stop signal", MB_OK);
			StopBasicBot();
			return 0;
		case BOT_BYTE_UP:
			value = 16;
			if (single) return value;
			break;
		case BOT_BYTE_P:
			value = Prand;
			if (single) return value;
			break;
		case BOT_BYTE_Q:
			value = Qrand;
			if (single) return value;
			break;
		case BOT_BYTE_X:
			value = X;
			if (single) return value;
			break;
		case BOT_BYTE_Y:
			value = Y;
			if (single) return value;
			break;
		case BOT_BYTE_Z:
			value = Z;
			if (single) return value;
			break;
		default:
			break;
		}
	}
	return value;
}
/**
 * Front end for the interpreter
 */
static int Interpret(int codefield)
{
	GlobalCurrentCode = 0;
	ByteCodeField = codefield;
	return Interpreter(false);
}


/**
 * Get a string showing the first x chars of the bytecode
 */
static void DebugByteCode(int code[])
{
	char result[1024];
	// for the love of god, why cant c++ just init this space :/
	memset(result, 0, 1024);

	sprintf(result, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
		code[0], code[1], code[2], code[3], code[4], 
		code[5], code[6], code[7], code[8], code[9],
		code[10], code[11], code[12], code[13], code[14], 
		code[15], code[16], code[17], code[18], code[19],
		code[20], code[21], code[22], code[23], code[24], 
		code[25], code[26], code[27], code[28], code[29],
		code[30], code[31], code[32], code[33], code[34], 
		code[35], code[36], code[37], code[38], code[39],
		code[40], code[41], code[42], code[43], code[44], 
		code[45], code[46], code[47], code[48], code[49],
		code[50], code[51], code[52], code[53], code[54], 
		code[55], code[56], code[57], code[58], code[59]);
	SetDlgItemTextA(hwndBasicBot,GUI_BOT_KEYS,result);
}
/**
 * Update BasicBot input
 * Called from the main game loop, every cycle
 **/
void UpdateBasicBot()
{
	// If there is any input on the buffer, dont update yet.
	// [0] means the number of inputs left on the BotInput buffer
	if(BotInput[0])
		return;
	if(hwndBasicBot && BotRunning && !EvaluateError)
	{
		if (MovingOn) // feed it the best result of the last part
		{
			if (BestAttempt[MovingPointer] != -1)
			{
				BotInput[0] = 1;
				BotInput[1] = BestAttempt[MovingPointer++];
			}
			else
			{
				SetNewPart();
			}
		}
		else // do normal bot stuff
		{
			// 1; just computing 1 frame of input.
			BotInput[0] = 1;
			
			// When the bot starts, or the "end when" is reached, NewAttempt is set.
			//	this boolean tells you to reset/init stuff.
			if(NewAttempt || ResetStatsASAP)
			{
				SetNewAttempt();
				// when 'reset' is pressed from gui
				// reset the current attmept and the stats
				// if you dont wait till the attempt is over
				// the stats will be screwed up
				if (ResetStatsASAP) {
					ResetStatsASAP = false;
					ResetStats();
				}
			}
			else
			{
				// Goal is met if the frame count goes above 1020, or if the 
				//	result was higher then a random number between 0 and 1000.
				//	I think the random part needs to be removed as it has no
				//	added value. You either get your goal or you dont. Leave
				//	the original, in case we want to revert it.
				
				//if(BotFrame > (BOT_MAXFRAMES-4) || (int) (NextRandom() % BOT_MAXPROB) < EvaluateFormula(Formula[20]))
				int i; // loopcounter
				bool improvement = false;
				bool invalid = false;
				if((BotFrame > (BOT_MAXFRAMES-5)) | (invalid = (Interpret(CODE_INVALID) >= BOT_MAXPROB)) | (Interpret(CODE_OK) >= BOT_MAXPROB))
				{
					// a little misleading, but if the improvement 
					// bool is set here, the attempt is invalid
					if (invalid)
						++Invalids;
					else
						++Oks;

					int currentscore[6];

					// This was the last frame of this attempt
					NewAttempt = true;

					// This evaluates each of the four strings (over and over again)
					//	which can become rather sluggish.
					//	maximize, tie1, tie2, tie3
					for(i=0; i < 6; i++)
					{
						currentscore[i] = Interpret(CODE_SCORE1+i);
					}

					// Update last score
					UpdateLastGUI(currentscore);

					// dont improve if this attempt was invalid
					if (!invalid)
					{
						// if this is the first attempt for this part, it's always better
						if (BestPart == NULL)
						{
							improvement = true;
						}
						else
						{
							// compare all scores. if scores are not equal, a break _will_
							//	occur. else the next will be checked. if all is equal, the
							//	old best is kept.
							for(i=0; i < 6; i++)
							{
								if(currentscore[i] > BotBestScore[i])
								{
									improvement = true;
									break;
								}
								else if(currentscore[i] < BotBestScore[i])
								{
									break;
								}
							}
						}
					}
					else
					{
						improvement = false;
					}
					
					// Update best
					if(improvement)
					{
						// Update the scores
						for(i = 0; i < 6; i++)
						{
							BotBestScore[i] = currentscore[i];
						}
						// Copy the new best run
						uint32 j;
						for(j = 0; j < AttemptPointer; j++)
						{
							BestAttempt[j] = CurrentAttempt[j];
						}

						// this may be the attempt that we want to continue on...
//						if (BestPart != NULL) // will be NULL when this is first 'best'
//							delete BestPart;
						BestPart = NewPart(Parts,BotAttempts,BotFrame,currentscore[0],currentscore[1],
							currentscore[2],currentscore[3],currentscore[4],currentscore[5], LastButtonPressed);

						// terminate
						BestAttempt[j] = -1;
						// duh
						UpdateBestGUI();
					}

					// this is the "post-frame goal-met" event part
					//if (LoggingEnabled())
					LogAttempt(currentscore, improvement);


					// if the attempt has finished, check whether this is the end of this part
					// just eval the part like the other stuff
					if (Interpret(CODE_MAXATTEMPTS) >= BOT_MAXPROB)
					{
//						if (LastPart != NULL) // only NULL after first part
//							delete LastPart;
						LastPart = BestPart;
//						BestPart = NULL; // dont delete it, we need the instance! it's inside LastPart now
						// update the frames and attempts in this computation
						LastPart->Tattempts = TotalAttempts;
						LastPart->Tframes = TotalFrames;
						// feed the best input to the app
						// we accomplish this by setting the bool
						MovingPointer = 0;
						MovingOn = true;
						// we're not actually going to do a new attempt because movingon is set
						SetNewAttempt();
						// next time the bot update is called, it will fetch input from the best buffer
						// once this is complete, the code will save the state and continue a new attempt
					}
				}
				else // Goal not met
				{
					// Generate 1 frame of output
					BotInput[1] = 0;

					// init frame event
					// Run code
					Interpret(CODE_EXTRA);
					
					// For two players, respectfully compute the probability
					//	for A B select start up down left and right buttons. In
					//	that order. If the number is higher then the generated
					//	random number, the button is pressed :)
					for (i=0;i<16;i++)
					{
						int res = Interpret(i);
						if (GetRandom(BOT_MAXPROB) < res)
						{
							// Button flags:
							//	button - player 1 - player 2
							//	A		 	1			 9
							//	B		 	2			10
							//	select	 	3			11
							//	start	 	4			12
							//	up		 	5			13
							//	down	 	6			14
							//	left	 	7			15
							//	right	 	8			16
							//	The input code will read the value of BotInput[1]
							//	If flag 17 is set, it will load a savestate, else
							//	it takes this input and puts the lower byte in 1
							//	and the upper byte in 2.
							BotInput[1] |= 1 << i;
						}
					}

					// Save what we've done
					// Only if we're not already at the max, should we 
					//	perhaps not check sooner if we are going to
					//	anyways?
					if (AttemptPointer < BOT_MAXFRAMES)
					{
						CurrentAttempt[AttemptPointer] = BotInput[1];
						AttemptPointer++;
					}
					
					// this is the "post-frame no-goal" event part

					// Remember the buttons pressed in this frame
					LastButtonPressed = BotInput[1];
				}
			}

			// Update statistics
			// Flag 17 for input means the code will load a savestate next. looks kinda silly. to be changed.
			if(BotInput[1] == 65536)
			{
				++BotAttempts;
				++TotalAttempts;
			}
			else
			{
				++BotFrames; // global
				++BotFrame;  // this attempt
				++TotalFrames; // global
			}
			// Increase the statistics
			if((BotFrames % 500) == 0)
			{
				UpdateCountersGUI();
			}
		}
	}
}


/**
 * Check the current settings for syntax validity
 **/
/*static void CheckCode()
{
	EvaluateError = false;
	for(int i=0;i<8;i++)
	{
		if (EvaluateError)
			break;
		switch (i)
		{
		case 0:
			debugS("Error at A");
			break;
		case 1:
			debugS("Error at B");
			break;
		case 2:
			debugS("Error at Start");
			break;
		case 3:
			debugS("Error at Select");
			break;
		case 4:
			debugS("Error at up");
			break;
		case 5:
			debugS("Error at down");
			break;
		case 6:
			debugS("Error at left");
			break;
		case 7:
			debugS("Error at right");
			break;
		default:
			break;
		}
		Interpret(i);
	}
	debugS("Error at extra code");
	Interpret(CODE_EXTRA);
	if (!EvaluateError)
	{
		debugS("Code syntax ok!");
		error(0);
	}
}
*/
/**
 * save code seems to be good.
 **/
static bool SaveBasicBot()
{
	const char filter[]="Basic Bot(*.bot)\0*.bot\0";
	char nameo[2048];
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Save Basic Bot As...";
	ofn.lpstrFilter=filter;
	nameo[0]=0;
	ofn.lpstrFile=nameo;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
	ofn.lpstrInitialDir=FCEU_GetPath(FCEUMKF_BBOT);
	if(GetSaveFileName(&ofn))
	{
		/*
		// Save the directory
		if(ofn.nFileOffset < 1024)
		{
			delete BasicBotDir;
			BasicBotDir=new char[strlen(ofn.lpstrFile)+1];
			strcpy(BasicBotDir,ofn.lpstrFile);
			BasicBotDir[ofn.nFileOffset]=0;
		}
		*/

		int i;
		// quick get length of nameo
		for(i=0;i<2048;i++)
		{
			if(nameo[i] == 0)
			{
				break;
			}
		}

		// add .bot if fn doesn't have it
		if((i < 4 || nameo[i-4] != '.') && i < 2040)
		{
			nameo[i] = '.';
			nameo[i+1] = 'b';
			nameo[i+2] = 'o';
			nameo[i+3] = 't';
			nameo[i+4] = 0;
		}
		return SaveBasicBotFile(nameo);
	}
	return false;
}
/**
 * Save to supplied filename
 */
static bool SaveBasicBotFile(char fn[])
{
	FILE *fp=FCEUD_UTF8fopen(fn,"wb");
	if (fp != NULL)
	{
		int i,j;
		fputc('b',fp);
		fputc('o',fp);
		fputc('t',fp);
		fputc(0,fp);
		fputc(BBsaveload&0xFF,fp);
		// for every line...
		for(i=0;i<BOT_FORMULAS;i++)
		{
			// first put down the length
			// this is an int, so 4 bytes
			// big/little endian doesnt matter
			// as long as we read it back the 
			// same order :)
			int len = GetLength(Formula[i]);
			fputc( len>>24,fp);
			fputc((len&0x00FF0000)>>16,fp);
			fputc((len&0x0000FF00)>>8,fp);
			fputc( len&0x000000FF,fp);
			// now put in the data
			for (j=0; j<len; ++j)
			{
				fputc(Formula[i][j],fp);
			}
		}
		fclose(fp);
		debugS("Saved settings!");
		return true;
	}
	debugS("Error saving settings! Unable to open file!");
	error(1005);
	return false;
}
/**
 * Loads a previously saved file
 * todo: need to add ensurance that saved file has the same BOT_MAXCODE value 
 *		 as currently set, or offer some kind of support for this "problem"
 **/
static bool LoadBasicBot()
{
	const char filter[]="Basic Bot (*.bot)\0*.bot\0";
	char nameo[2048];
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Load Basic Bot...";
	ofn.lpstrFilter=filter;
	nameo[0]=0;
	ofn.lpstrFile=nameo;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	ofn.lpstrInitialDir=FCEU_GetPath(FCEUMKF_BBOT);

	if(GetOpenFileName(&ofn))
	{
		/*
		// Save the directory
		if(ofn.nFileOffset < 1024)
		{
			delete BasicBotDir;
			BasicBotDir=new[GetLength(ofn.lpstrFile)+1];
			strcpy(BasicBotDir,ofn.lpstrFile);
			BasicBotDir[ofn.nFileOffset]=0;
		}
		*/

		// now actually load the file
		return LoadBasicBotFile(nameo);
	}
	return false;
}


/**
 * Load a file by supplying the filename
 */
static bool LoadBasicBotFile(char fn[])
{
	int i,j;

	// open the file
	FILE *fp=FCEUD_UTF8fopen(fn,"rb");
	if (fp != NULL) 
	{

		// if the first four chars of the file are not
		//       "bot0", it's not a valid botfile. only check
		//       done at the moment.
		if(fgetc(fp) != 'b'
			|| fgetc(fp) != 'o'
			|| fgetc(fp) != 't'
			|| fgetc(fp) != 0)
		{
			fclose(fp);
			debugS("Error loading file! Target not a botfile!");
			error(1005);
			return false;
		}
		// the next thing is the saveload version
		// if this differs, the current code has
		// a different layout for the botfile and
		// can not read the current botfile
		// this is just a byte, but i dont think
		// there will be more then 256 versions :p
		if (fgetc(fp) != BBsaveload)
		{
			fclose(fp);
			debugS("Error loading file! Incorrect version!");
			error(1005);
			return false;
		}

		// next come all the codefields
		for(i=0;i<BOT_FORMULAS;i++)
		{
			// first read the size. this is an int
			// so read the four bytes, in the same 
			// order as they were saved. +1 for \0
			int len = (fgetc(fp)<<24) + (fgetc(fp)<<16) + (fgetc(fp)<<8) + fgetc(fp);
			// now create some space for this thing
			char * code = new char[len+1];
			// add the \0
			code[len] = '\0';
			// now read this many bytes...
			for(j=0;j<len;j++)
			{
				code[j] = fgetc(fp);
				// if the char is 128, its 0 (?)
				// still have to figure out wtf
				// but i dont think this applies
				// anymore for the current code
				// i think it was to eliminate
				// chars >128, or something.
				if(code[j] & 128)
					code[j] = 0;
			}
			// we want no memleaks, no sir!
			if (Formula[i] != NULL)
				delete Formula[i];
			// hook them up!
			Formula[i] = code;
		}
		// release lock and resources
		fclose(fp);
		debugS("Loaded file!");
		return true;
	}
	debugS("Error loading file! Unable to open it!");
	error(1005);
	return false;
}


/**
 * Moved code to start function to be called from other places.
 * The code puts all the code from gui to variables and starts bot.
 **/
static void StartBasicBot()
{
	// show message
	debugS("Running!");
	FCEU_SetBotMode(1);
	BotRunning = true;
	EvaluateError = false;
	FromGUI();
	if (!EvaluateError)
		SetDlgItemText(hwndBasicBot,GUI_BOT_RUN,(LPTSTR)"Stop!");
	else
		return;
	NewAttempt = true;
	if (LastPart != NULL) delete LastPart;
	LastPart = NULL;
	Parts = 1;
	memset(BotCounter, 0, BOT_MAXCOUNTERS*sizeof(int)); // Sets all the counters to 0.
	LastButtonPressed = 0;
	// set up saved state, load default, save to botstate
	FCEUI_LoadState(0);
	FCEUI_SaveState(BOT_STATEFILE);
}

/**
 * Moved code to stop function to be called from other places
 **/
static void StopBasicBot() 
{
    //FCEU_SetBotMode(0);
	BotRunning = false;
    BotAttempts = 0;
    BotFrames = 0;
	TotalFrames = 0;
	TotalAttempts = 0;
    BotBestScore[0] = BotBestScore[1] = BotBestScore[2] = BotBestScore[3] = -999999999;
    NewAttempt = true;
	SetDlgItemText(hwndBasicBot,GUI_BOT_RUN,"Run!");
}


/**
 * Put all the editable values from the gui to the variables
 *	Note that this does not update the buttons for two players, just the one selected
 **/
static void FromGUI()
{
	if (hwndBasicBot)
	{
		// buttons
		for (int i = 0; i < BOT_FORMULAS; i++)
        {
			// since we alloc the strings, we need to delete them before replacing them...
			if (Formula[i] != NULL) 
				delete Formula[i];
			Formula[i] = GetText(hwndBasicBot,Labels[i]);
			// um yeah, eval-ing the comments etc is a bad idea :p
			if (i < CODE_COMMENTS) 
				Bytecode[i] = ToByteCode(Formula[i],i);
        }
		// function, because we call this from a button as well
		UpdateStatics();
	}
	UpdateFromGUI = false;
}

/**
 * Put all the editable values from the variables to the gui
 **/
static void ToGUI()
{
	if (hwndBasicBot && Formula)
	{
		for (int i=0; i<BOT_FORMULAS; ++i)
		{
			SetDlgItemText(hwndBasicBot,Labels[i],Formula[i]);
		}
	}
}


/**
 * Initializes the variables to 0 (and frame=1)
 **/
static void InitVars()
{
	char * n;
	for(int i = 0; i < BOT_FORMULAS; i++)
	{
		if (i != CODE_OK && i < CODE_COMMENTS)
		{
			n = new char[2];
			n[0] = '0';
			n[1] = 0;
			Formula[i] = n;
		}
	}
	n = new char[8];
	n[0] = 'f'; n[1] = 'r'; n[2] = 'a'; n[3] = 'm'; n[4] = 'e'; n[5] = '='; n[6] = '5'; n[7] = 0;
	Formula[CODE_OK] = n;
	n = new char[5];
	n[0] = 'N'; n[1] = 'o'; n[2] = 'n'; n[3] = 'e'; n[4] = 0;
	Formula[CODE_COMMENTS] = n;
	n = new char[12];
	n[0] = 'a'; n[1] = 't'; n[2] = 't'; n[3] = 'e'; n[4] = 'm'; n[5] = 'p';
	n[6] = 't'; n[7] = '='; n[8] = '1'; n[9] = '0'; n[10] = '0'; n[11] = 0;
	Formula[CODE_MAXATTEMPTS] = n;
	n = new char[7];
	n[0] = 'S'; n[1] = 'c'; n[2] = 'o'; n[3] = 'r'; n[4] = 'e'; n[5] = '1'; n[6] = 0;
	Formula[CODE_TITLE1] = n;
	n = new char[7];
	n[0] = 'S'; n[1] = 'c'; n[2] = 'o'; n[3] = 'r'; n[4] = 'e'; n[5] = '2'; n[6] = 0;
	Formula[CODE_TITLE2] = n;
	n = new char[7];
	n[0] = 'S'; n[1] = 'c'; n[2] = 'o'; n[3] = 'r'; n[4] = 'e'; n[5] = '3'; n[6] = 0;
	Formula[CODE_TITLE3] = n;
	n = new char[7];
	n[0] = 'S'; n[1] = 'c'; n[2] = 'o'; n[3] = 'r'; n[4] = 'e'; n[5] = '4'; n[6] = 0;
	Formula[CODE_TITLE4] = n;
	n = new char[7];
	n[0] = 'S'; n[1] = 'c'; n[2] = 'o'; n[3] = 'r'; n[4] = 'e'; n[5] = '5'; n[6] = 0;
	Formula[CODE_TITLE5] = n;
	n = new char[7];
	n[0] = 'S'; n[1] = 'c'; n[2] = 'o'; n[3] = 'r'; n[4] = 'e'; n[5] = '6'; n[6] = 0;
	Formula[CODE_TITLE6] = n;
	n = new char[12];
	n[0] = 'u'; n[1] = 'n'; n[2] = 'k'; n[3] = 'n'; n[4] = 'o'; n[5] = 'w';
	n[6] = 'n'; n[7] = '.'; n[8] = 'r'; n[9] = 'o'; n[10] = 'm'; n[11] = 0;
	Formula[CODE_ROMNAME] = n;
	n = new char[2];
	n[0] = '0'; n[1] = 0;
	Formula[CODE_SKIPS] = n;
	n = new char[2];
	n[0] = '0'; n[1] = 0;
	Formula[CODE_SLOW] = n;
	X = Y = Z = P = Prand = Q = Qrand = 0;
}

/**
 * Behaves like FromGUI(), except only for XYZPQ
 */
static void UpdateStatics()
{
	// These "statics" are only evaluated once
	//	(this also saves me looking up stringtoint functions ;)
	// todo: improve efficiency, crap atm due to conversion
	if (hwndBasicBot)
	{
		for (int i = CODE_X; i < 5; ++i)
        {
			// since we malloc the strings, we need to delete them before replacing them...
			if (Formula[i] != NULL) 
				delete Formula[i];
			Formula[i] = GetText(hwndBasicBot,Labels[i]);
			// um yeah, eval-ing the comments is a bad idea :p
			Bytecode[i] = ToByteCode(Formula[i],i);

			if (i == CODE_X) X = Interpret(i);
			else if (i == CODE_Y) Y = Interpret(i);
			else if (i == CODE_Z) Z = Interpret(i);
			else if (i == CODE_P) P = Interpret(i);
			else if (i == CODE_Q) Q = Interpret(i);
			else debugS("Grave error!");
        }
	}
}

/**
 * play the best found run
 **/
static void PlayBest()
{
	// feed BotInput the inputs until -1 is reached
	int i;
	for(i = 0; i < (BOT_MAXFRAMES-3) && BestAttempt[i] != -1; i++)
	{
		BotInput[i+2] = BestAttempt[i];
	}
	BotInput[0] = i+1; 		// the number of botframes currently in the variable
	BotInput[1] = 65536; 	// flag 17, when processed by input.cpp this causes the
							//	code to load the savestate. should really change
							//	this construction, its ugly (but i guess it works)
							//  i dont see the problem in calling a load from here
}

int skip = 1;
int pause = 0;
/**
 * Called from windows. main gui (=dialog) control.
 **/
static BOOL CALLBACK BasicBotCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		// cant change gui controls in this event
		// seems to be a inherited "feature", for backwards
		// compatibility.
		SetWindowPos(hwndDlg,0,BasicBot_wndx,BasicBot_wndy,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER);
		break;
	case WM_MOVE: {
		RECT wrect;
		GetWindowRect(hwndDlg,&wrect);
		BasicBot_wndx = wrect.left;
		BasicBot_wndy = wrect.top;
		break;
	};

	case WM_CLOSE:
	case WM_QUIT:
		CrashWindow();
		break;
	case WM_COMMAND:
		switch(HIWORD(wParam))
		{
		case BN_CLICKED:
			bool p1 = false; // if a input radiobutton is pressed, this tells me which one
			switch(LOWORD(wParam))
			{
			case GUI_BOT_INTERNAL:
				p1 = true;
			case GUI_BOT_EXTERNAL:
				// set the botmode
				FCEU_SetBotMode(p1?0:1);
				break;
			case GUI_BOT_SAVE:
				FromGUI();
				SaveBasicBot();
				break;			
			case GUI_BOT_LOAD:
				LoadBasicBot();
				ToGUI();
				break;
			case GUI_BOT_BEST:
				// if bot is running and no frames left on the botinput buffer
    			if(!BotRunning && BotInput[0] == 0)
				{
					PlayBest();
				}
				break;
			case GUI_BOT_RUN:
				if(BotRunning)
				{
                    StopBasicBot();
				}
				else
				{
                    StartBasicBot();
				}
				break;
			case GUI_BOT_CLEAR:
				if(MessageBox(hwndBasicBot, "Clear all text?", "Confirm clear", MB_YESNO)==IDYES)
				{
					InitVars();
					ToGUI();
				}
				break;
			case GUI_BOT_CLOSE:
				CrashWindow();
				break;
			case GUI_BOT_UPDATE:
				// put a flag up, when the current attempt stops, let the code 
				//	refresh the variables with the data from gui, without stopping.
				UpdateFromGUI = true;
				break;
			case GUI_BOT_U:
				// the small update button above the static variables ("U"),
				//	only update the statics.
				UpdateStatics();
				break;
			case GUI_BOT_TEST:
				{
				//MessageBox(hwndBasicBot, "Showing", "Hrm", MB_OK);
				//SetDlgItemText(hwndBasicBot,GUI_BOT_BOTMODE,"hello");
				
				FromGUI();
				DebugByteCode(Bytecode[CODE_EXTRA]);
				error(Interpret(CODE_EXTRA));
				
					/*
				FILE *f = fopen("file.txt","ab");
				fprintf(f,"hello %d earthlings!",5);
				fclose(f);
				*/
					ShowCounters();


				//char * t = GetText(hwndBasicBot,GUI_BOT_MAXPARTS);
				//debugS(t);
				//free(t);
				}
				break;
			case GUI_BOT_VALUES:
				{
				// display all the #DEFINEs and static values
				char valuesboxmsg[2048];
				sprintf(valuesboxmsg, 
					"%s\n%s\n\n%s%s\n%s%d\n%s%d\n%s%d\n%s%d",
					"These are the values that were",
					"hardcoded at compilation time:",
					"Basic Bot:		v", BBversion,
					"Botfile:		v", BBsaveload,
					"Max Frames:	", BOT_MAXFRAMES,
					"Counters:		", BOT_MAXCOUNTERS,
					"Probability Range:	0 - ", BOT_MAXPROB);
				
				MessageBox(hwndBasicBot, valuesboxmsg, "Harcoded values", MB_OK);
				}
				break;
			case GUI_BOT_RESET:
				ResetStatsASAP = true;
				break;
			case GUI_BOT_FRAMESKIP:
				skip = GetDlgItemInt(hwndBasicBot,GUI_BOT_SKIPS,0,false);
				pause = GetDlgItemInt(hwndBasicBot,GUI_BOT_SLOW,0,false);
				break;
			case GUI_BOT_TITLES:
				UpdateTitles();
				break;
			default:
				break;
			}
		}
		break; // WM_COMMAND
	default:
		break;
	}
	return 0;
}

/**
 * Creates dialog
 **/
void CreateBasicBot()
{
	SeedRandom(rand());
	InitVars();

	if(hwndBasicBot) // If already open, give focus
	{
		SetFocus(hwndBasicBot);
	}
	else
	{
		// Create
		hwndBasicBot=CreateDialog(fceu_hInstance,"BASICBOT",NULL,BasicBotCallB);
		EditPlayerOne = true;
		BotRunning = false;
		InitCode();
		SetWindowText(hwndBasicBot,BBcaption);
	}
}
/**
 * This function should be called whenever the dialog is created
 */
void InitCode()
{
	CheckDlgButton(hwndBasicBot, FCEU_BotMode() ? GUI_BOT_EXTERNAL : GUI_BOT_INTERNAL, 1); // select the player1 radiobutton
	//if (LoadBasicBotFile("default.bot"))
	if (false)
	{
		// false until i figure out why the directory changes
		ToGUI();
		debugS("Loaded default.bot");
	}
	else
	{
		//debugS("Error loading default.bot");
		InitVars();
		ToGUI();
	}
	UpdateExternalButton();
}
/**
 * This function should be called whenever the window is closing
 */
void CrashWindow()
{
	FromGUI();
	//SaveBasicBotFile("default.bot");
	for (int i=0; i<BOT_FORMULAS; ++i)
	{
		delete Formula[i];
		delete Bytecode[i];
	}
	DestroyWindow(hwndBasicBot);
	hwndBasicBot=0;
}
/**
 * Update the text on this button. Called from input, when this state changes.
 **/
void UpdateExternalButton()
{
	if (hwndBasicBot)
	{
		if (FCEU_BotMode())
		{
			CheckDlgButton(hwndBasicBot, GUI_BOT_INTERNAL, 0);
			CheckDlgButton(hwndBasicBot, GUI_BOT_EXTERNAL, 1);
		}
		else
		{
			CheckDlgButton(hwndBasicBot, GUI_BOT_INTERNAL, 1);
			CheckDlgButton(hwndBasicBot, GUI_BOT_EXTERNAL, 0);
		}
	}
}

/**
 * Reset the stats, and the gui as well...
 * Should be called after a attempt is done
 */
void ResetStats()
{
	BotAttempts = 0;
	Parts = 1;
	BotFrames = 0;
	Oks = 0;
	Invalids = 0;
	for (int i=0; i<BOT_MAXSCORES; ++i)
		BotBestScore[i] = 0;
	if (LastPart != NULL)
	{
		delete LastPart;
		LastPart = NULL;
	}
	UpdateFullGUI();
}
/**
 * Update all the values in the GUI
 */
void UpdateFullGUI()
{
	ToGUI();
	UpdateBestGUI();
	int empty[4] = {0,0,0,0};
	UpdateLastGUI(empty);
	UpdateCountersGUI();
	ShowCounters();
}
/**
 * Update the "best" parts of GUI
 */
void UpdateBestGUI()
{
	#define BOT_RESULTBUFSIZE (BOT_MAXFRAMES*sizeof(int))
	char tempstring[BOT_RESULTBUFSIZE];				// Used for the "result" part. last time i checked 1024*12 is > 4k, but ok.
	memset(tempstring, 0, BOT_RESULTBUFSIZE);		// clear the array
	char symbols[] = "ABET^v<>ABET^v<>";
	bool seenplayer2 = false;						// Bool keeps track of player two inputs

	// Update best scores
	SetDlgItemInt(hwndBasicBot,GUI_BOT_ATTEMPT_BEST,BotAttempts,TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_FRAMES_BEST,BotFrame,TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_SCORE_BEST,BotBestScore[0],TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE1_BEST,BotBestScore[1],TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE2_BEST,BotBestScore[2],TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE3_BEST,BotBestScore[3],TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE4_BEST,BotBestScore[4],TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE5_BEST,BotBestScore[5],TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_PART_BEST,Parts,TRUE);

	// Create the run in ascii
	int k = 0;     // keep track of len, needs to be below bufsize
	// Make output as text
	// While you're not exceeding the max of frames, a
	//	frame had input, and you're not exceeding the bufsize...
	//	Warning: the second condition prevents a bufferoverrun,
	//		-21 is because between checks a max of 21 chars
	//		could be added.
	for(int i = 0; i < BOT_MAXFRAMES && BestAttempt[i] != -1 && k < BOT_RESULTBUFSIZE-21; i++)
	{
		tempstring[k] = '0' + ((i/10) % 10);
		k++;
		tempstring[k] = '0' + (i%10);
		k++;
		tempstring[k] = '.';
		k++;
		seenplayer2=0;
		for(int j = 0; j < 16; j++)
		{
			if(BestAttempt[i] & (1 << j))
			{
				if(j >= 8 && seenplayer2==0)
				{
					seenplayer2=1;
					tempstring[k]=',';
					k++;
				}
				tempstring[k] = symbols[j];
				k++;
			}
		}
		tempstring[k] = ' ';
		k++;
	}

	// Update best score keys
	SetDlgItemText(hwndBasicBot,GUI_BOT_KEYS,tempstring);
}
/**
 * Update the Last Scores
 */
void UpdateLastGUI(int last[])
{
	// Update best scores
	SetDlgItemInt(hwndBasicBot,GUI_BOT_ATTEMPT_LAST,Parts,TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_ATTEMPT_LAST,BotAttempts,TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_FRAMES_LAST,BotFrame,TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_SCORE_LAST,last[0],TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE1_LAST,last[1],TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE2_LAST,last[2],TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE3_LAST,last[3],TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE4_LAST,last[4],TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE5_LAST,last[5],TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_PART_LAST,Parts,TRUE);
}
/**
 * update info for previous part
 **/
void UpdatePrevGUI(int best[])
{
	// Update previous part scores
	if (LastPart != NULL)
	{
		SetDlgItemInt(hwndBasicBot,GUI_BOT_PART_PREV,LastPart->part,TRUE);
		SetDlgItemInt(hwndBasicBot,GUI_BOT_ATTEMPT_PREV,LastPart->attempt,TRUE);
		SetDlgItemInt(hwndBasicBot,GUI_BOT_FRAMES_PREV,LastPart->frames,TRUE);
		SetDlgItemInt(hwndBasicBot,GUI_BOT_SCORE_PREV,best[0],TRUE);
		SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE1_PREV,best[1],TRUE);
		SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE2_PREV,best[2],TRUE);
		SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE3_PREV,best[3],TRUE);
		SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE4_PREV,best[4],TRUE);
		SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE5_PREV,best[5],TRUE);
		SetDlgItemInt(hwndBasicBot,GUI_BOT_LB_PREV,LastPart->lastbutton,TRUE);
	}
	else
	{
		SetDlgItemInt(hwndBasicBot,GUI_BOT_ATTEMPT_PREV,0,TRUE);
		SetDlgItemInt(hwndBasicBot,GUI_BOT_FRAMES_PREV,0,TRUE);
		SetDlgItemInt(hwndBasicBot,GUI_BOT_SCORE_PREV,0,TRUE);
		SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE1_PREV,0,TRUE);
		SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE2_PREV,0,TRUE);
		SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE3_PREV,0,TRUE);
		SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE4_PREV,0,TRUE);
		SetDlgItemInt(hwndBasicBot,GUI_BOT_TIE5_PREV,0,TRUE);
		SetDlgItemInt(hwndBasicBot,GUI_BOT_PART_PREV,0,TRUE);
	}
}

/**
 * Update the counters in the GUI
 */
void UpdateCountersGUI()
{
	SetDlgItemInt(hwndBasicBot,GUI_BOT_ATTEMPTS,TotalAttempts,TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_FRAMES,TotalFrames,TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_OKS,Oks,TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_INVALIDS,Invalids,TRUE);
}
/**
 * put the texts in the title controls to the result window, and in mem
 */
void UpdateTitles() {
	for (int i=0; i<BOT_MAXSCORES; ++i) {
		Formula[i+CODE_TITLE1] = GetText(hwndBasicBot,Labels[i+CODE_TITLE1]);
	}
	SetDlgItemTextA(hwndBasicBot,GUI_BOT_TITLEL1,Formula[CODE_TITLE1]);
	SetDlgItemTextA(hwndBasicBot,GUI_BOT_TITLEL2,Formula[CODE_TITLE2]);
	SetDlgItemTextA(hwndBasicBot,GUI_BOT_TITLEL3,Formula[CODE_TITLE3]);
	SetDlgItemTextA(hwndBasicBot,GUI_BOT_TITLEL4,Formula[CODE_TITLE4]);
	SetDlgItemTextA(hwndBasicBot,GUI_BOT_TITLEL5,Formula[CODE_TITLE5]);
	SetDlgItemTextA(hwndBasicBot,GUI_BOT_TITLEL6,Formula[CODE_TITLE6]);
}

/**
 * Set up the vars to start a new attempt
 */
void SetNewAttempt()
{
	if (UpdateFromGUI)
		FromGUI();
	LastButtonPressed = 0;
	BotInput[1] = 65536; 		 // 17th flag, interpreted below as start of a new attempt.
								 //		it loads the savestate when encountered in input.cpp.
								 //		should handle this more gracefully some day.
	NewAttempt = false;	 		 // Running an attempt now
	AttemptPointer = 0;  		 // Offset, frame counter?
	BotFrame = 0;        		 // Frame counter...
	// Update the P and Q values to something new
	Prand = GetRandom(P);
	Qrand = GetRandom(Q);

	// set the values from the previous parts correct
	if (LastPart != NULL)
	{
		LastButtonPressed = LastPart->lastbutton;
		for (int i=0; i<BOT_MAXCOUNTERS; ++i)
			BotCounter[i] = LastPart->counters[i];
	}
	else
	{
		memset(BotCounter, 0, BOT_MAXCOUNTERS*sizeof(int)); // Sets all the counters to 0.
		LastButtonPressed = 0;
	}
	ShowCounters();
	// Run init attempt code
	Interpret(CODE_INIT_ATTEMPT);

}





/**
 * code to run when a new part is created
 */
void SetNewPart()
{
				// save the current state to the default state
				FCEUI_SaveState(BOT_STATEFILE);
				// show stats
				UpdatePrevGUI(BotBestScore);
				// and continue...
				MovingOn = false;
				BotAttempts = 0;
				++Parts;
				SetNewAttempt();

				// init part event
				// Run code
				Interpret(CODE_INIT_PART);
}
/**
 * Returns the number of frames to skip
 * while computing in botmode
 * (called from main.cpp)
 */
int BotFrameSkip()
{
	return skip;
}
/**
 * Returns the ms to pause after each blit
 * This can be used to evaluate your bot
 * (called from main.cpp)
 */
int BotFramePause()
{
	return pause;
}
/**
 * write one line of logging to a default logfile
 */
void LogAttempt(int * scores, bool better)
{
	LogFile = fopen("bb-log.txt","at");
	//fprintf(LogFile, "hello world!");
	
	fprintf(LogFile,"%sPart %d: Attempt %d: frames: %d",
		better?"*":"",Parts,BotAttempts,BotFrame);
	
	// dont print scores that have - as title
	if (!(Formula[CODE_TITLE1][0] == '-' && Formula[CODE_TITLE1][1] == 0))
		fprintf(LogFile,", %s: %d",Formula[CODE_TITLE1],scores[0]);
	if (!(Formula[CODE_TITLE2][0] == '-' && Formula[CODE_TITLE2][1] == 0))
		fprintf(LogFile,", %s: %d",Formula[CODE_TITLE2],scores[1]);
	if (!(Formula[CODE_TITLE3][0] == '-' && Formula[CODE_TITLE3][1] == 0))
		fprintf(LogFile,", %s: %d",Formula[CODE_TITLE3],scores[2]);
	if (!(Formula[CODE_TITLE4][0] == '-' && Formula[CODE_TITLE4][1] == 0))
		fprintf(LogFile,", %s: %d",Formula[CODE_TITLE4],scores[3]);
	if (!(Formula[CODE_TITLE5][0] == '-' && Formula[CODE_TITLE5][1] == 0))
		fprintf(LogFile,", %s: %d",Formula[CODE_TITLE5],scores[4]);
	if (!(Formula[CODE_TITLE6][0] == '-' && Formula[CODE_TITLE6][1] == 0))
		fprintf(LogFile,", %s: %d",Formula[CODE_TITLE6],scores[5]);
	fprintf(LogFile,"\n");
	fclose(LogFile);	
}

void ShowCounters()
{
	HWND list = GetDlgItem(hwndBasicBot, GUI_BOT_COUNTERS);
	SendMessage(list,LB_RESETCONTENT,NULL,NULL);
	for (int i=0; i<BOT_MAXCOUNTERS; ++i)
	{
		char str[50];
		sprintf(str,"[%03d]: %02X",i,BotCounter[i]);
		SendMessage(list, LB_ADDSTRING, NULL, (LPARAM) str);
	}
}
