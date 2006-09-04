int res;
/**
 * qfox:
 * todo: boundrycheck every case in the switch! the string checks might override the boundries and read data its not supposed to. i'm not quite sure if this is an issue in C though.
 * todo: memi() to fetch a 32bit number? is this desired?
 * todo: check for existence of argument (low priority)
 * todo: let the code with errorchecking run through the code first so subsequent evals are faster. -> done when changing to bytecode/interpreter
 * todo: cleanup "static"s here and there and read up on what they mean in C, i think i've got them confused
 * todo: fix some button so you can test code without having to run it (so setting external input and opening a rom won't be required to test scriptcode). Code is there but you can't do certain commands when no rom is loaded and nothing is init'ed, so i'll put this on hold.
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

// qfox: v0.1.0 will be the last release by Luke, mine will start at v0.2.0 and
//	 go up each time something is released to the public, so you'll
//	 know your version is the latest or whatever. Version will be
//	 put in the project as well.
static char BBversion[] = "0.3.0";
// title
static char BBcaption[] = "Basic Bot v0.3.0 by qFox";

static HWND hwndBasicBot = 0;			// GUI handle
static bool BotRunning = false;			// Is the bot computing or not?

static uint32 rand1=1, rand2=0x1337EA75, rand3=0x0BADF00D;

#define BOT_MAXCODE 1024						// Max length of code-fields in bytes
#define BOT_FORMULAS 28							// Number of code fields in the array
static char Formula[BOT_FORMULAS][BOT_MAXCODE];	// These hold BOT_FORMULAS formula's entered in the GUI:
static int Bytecode[BOT_FORMULAS][BOT_MAXCODE]; // Byte code converted formulas
static int ByteCodePointer;						// Points to position of next value
static int ByteCodeField;						// Working field
static int  CODE_1_A        = 0,				// code fields
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
			CODE_STOP       = 16,
			CODE_MAX        = 17,
			CODE_TIE1       = 18,
			CODE_TIE2       = 19,
			CODE_TIE3       = 20,
			CODE_X			= 21,
			CODE_Y			= 22,
			CODE_Z			= 23,
			CODE_P			= 24,
			CODE_Q			= 25,
			CODE_EXTRA		= 26,
			CODE_COMMENTS   = 27;

const int	BOT_BYTE_LB		= 48,				// commands
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
			BOT_BYTE_ATTEMPT = 19,
			BOT_BYTE_AC		= 20,
			BOT_BYTE_ABS	= 21,
			BOT_BYTE_BUTTON = 22,
			BOT_BYTE_C		= 23,
			BOT_BYTE_DOWN	= 24,
			BOT_BYTE_ECHO	= 25,
			BOT_BYTE_FRAME	= 26,
			BOT_BYTE_I		= 27,
			BOT_BYTE_J		= 28,
			BOT_BYTE_K		= 29,
			BOT_BYTE_LAST   = 50,
			BOT_BYTE_LEFT	= 30,
			BOT_BYTE_LOOP	= 31,
			BOT_BYTE_LSHIFT = 51,
			BOT_BYTE_MEM	= 32,
			BOT_BYTE_MEMH	= 33,
			BOT_BYTE_MEML	= 34,
			BOT_BYTE_MEMW	= 35,
			BOT_BYTE_RC		= 37,
			BOT_BYTE_RIGHT	= 36,
			BOT_BYTE_RSHIFT = 52,
			BOT_BYTE_START	= 38,
			BOT_BYTE_SELECT = 39,
			BOT_BYTE_SC		= 40,
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

static int BotAttempts, 				// Number of attempts tried so far
	   BotFrames, 						// Number of frames computed so far
	   BotBestScore[4]; 				// Maximize, tie1, tie2, tie3
static bool NewAttempt;					// Tells code to reset certain run-specific info

static int LastButtonPressed;			// Used for lastbutton command (note that this 
										//	doesnt work for the very first frame of each attempt...)

static int 
	CurrentAttempt[BOT_MAXFRAMES],		// Contains all the frames of the current attempt.
	   BestAttempt[BOT_MAXFRAMES], 		// Contains all the frames of the best attempt so far.
	   AttemptPointer;					// Points to the last frame of the current attempt.

static bool ProcessCode = true;			// This boolean tells the code whether or not to process
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

float AverageScore[4];					// Keep the average scores in here. to prevent overflow
										//	this is not a total, but the average is computed after
										//	every attempt and put back in here. this shouldn't
										//	affect accuracy.

int X,Y,Z,P,Prand,Q,Qrand;              // Static variables (is this a contradiction?) and
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

static void SeedRandom(uint32 seed)
{
	rand1 = seed;
	if(rand1 == 0) rand1 = 1; // luke: I forget if this is necessary.
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
static void debugS(LPCSTR s)
{
	SetDlgItemTextA(hwndBasicBot,GUI_BOT_DEBUG,s);
}

/**
 * Print a number
 **/
static void debug(int n)
{
     SetDlgItemInt(hwndBasicBot,GUI_BOT_DEBUG,n,TRUE);
}
/**
 * When an error occurs, call this method to print the errorcode
 **/
static void error(int n)
{
     SetDlgItemInt(hwndBasicBot,GUI_BOT_ERROR,n,TRUE);
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
 * - codefield is the field for which this code is meant
 **/
static void ToByteCode(char * formula, int codefield)
{
	// clear array
	memset(Bytecode[codefield],0,BOT_MAXCODE*sizeof(int));
	// reset values
    EvaluateError = false;
    // set the global pointer to the string at the start of the string
    GlobalCurrentChar = formula;
    // set the global field to the field passed on
	ByteCodeField = codefield;

    // first we want to quickly check whether the brackets are matching or not
    // same for ? and :, both are pairs and should not exist alone, also they
	// can not appear out of order (so no "?;:" ";?:" or something)
	// also, 
    char * quickscan = formula;
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
		debugS("?:;() dont match!");
		return;
	}
    
	ByteCodePointer = 0;
	bool hexmode = false;
	bool negmode = false;

	// three stacks, used to branch properly
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
	int qpoint=-1,cpoint=-1,lpoint=-1;

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
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_LB;
			break;
		case ')':
			++GlobalCurrentChar;
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_RB;
			break;
		case 'x':
			++GlobalCurrentChar;
			hexmode = true;
			break;
		case '+':
			++GlobalCurrentChar;
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_ADD;
			break;
		case '-':
			++GlobalCurrentChar;
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_SUB;
			break;
		case '*':
			++GlobalCurrentChar;
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_MUL;
			break;
		case '/':
			++GlobalCurrentChar;
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_DIV;
			break;
		case '%':
			++GlobalCurrentChar;
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_MOD;
			break;
		case '&':
			++GlobalCurrentChar;
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_AND;
			break;
		case '|':
			++GlobalCurrentChar;
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_OR;
			break;
		case '^':
			++GlobalCurrentChar;
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_XOR;
			break;
		case '=':
			++GlobalCurrentChar;
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_EQ;
			break;
		case '>':
			if(*(GlobalCurrentChar + 1) == '=')
			{
                GlobalCurrentChar += 2;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_GTEQ;
            }
			else if(*(GlobalCurrentChar + 1) == '>')
			{
				GlobalCurrentChar += 2;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_RSHIFT;
			}
			else
			{
			    ++GlobalCurrentChar;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_GT;
            }
			break;
		case '<':
			if(*(GlobalCurrentChar + 1) == '=') 
            {
                GlobalCurrentChar += 2;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_LTEQ;
            }
			else if(*(GlobalCurrentChar + 1) == '<')
			{
				GlobalCurrentChar += 2;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_LSHIFT;
			}
			else
			{
			    ++GlobalCurrentChar;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_LT;
            }
			break;
		case '!':
			if(*(GlobalCurrentChar + 1) == '=')
			{
                GlobalCurrentChar += 2;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_UEQ;
			}
			else
			{
				BotSyntaxError(1001); 	// "!" is not a valid command, but perhaps we could 
          						  		//	use it as a prefix, checking if the next is 1000
		          				  		//	(or 0) or not and inverting it. For now error it.
				debugS("Unknown command (!...)");
				return;
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
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_IIF;
			qmarkstack[++qpoint] = ByteCodePointer++;
			break;
		case ':':
			{
			++GlobalCurrentChar;
			// colon encountered
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_COLON;
			// next position will contain jump to end of second statement
			colonstack[++cpoint] = ByteCodePointer++;
			// put the number of positions after this target as jump target after the ?
			int qmarkpos = qmarkstack[qpoint--];
			Bytecode[ByteCodeField][qmarkpos] = ByteCodePointer - qmarkpos;
			break;
			}
		case ';':
			{
			++GlobalCurrentChar;
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_SCOLON;
			// semicolon encountered
			// had to use the semicolon to simplify determining end of second arg...
			// next position is the target of the colon
			int colonpos = colonstack[cpoint--];
			Bytecode[ByteCodeField][colonpos] = ByteCodePointer - colonpos;
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
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_ATTEMPT;
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
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_AC;
			}
			else if(*(GlobalCurrentChar+1) == 'b'
				&&*(GlobalCurrentChar+2) == 's'
				&&*(GlobalCurrentChar+3) == '(')
			{
                // abs()
				GlobalCurrentChar += 4;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_ABS;
			}
			else
			{
                // a()
				++GlobalCurrentChar;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_LIT;
				Bytecode[ByteCodeField][ByteCodePointer++] = 1;
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
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_BUTTON;
			}
			else
			{
                // button B
				++GlobalCurrentChar;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_LIT;
				Bytecode[ByteCodeField][ByteCodePointer++] = 2;
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
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_C;
			}
			else
			{
				BotSyntaxError(2001);
				debugS("Unknown Command (c...)");
				return;
			}
			break;
		case 'd':
			if(*(GlobalCurrentChar+1) == 'o'
				&&*(GlobalCurrentChar+2) == 'w'
				&&*(GlobalCurrentChar+3) == 'n')
			{
                // down
				GlobalCurrentChar += 4;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_DOWN;
			}
			else
			{
				BotSyntaxError(3001);
				debugS("Unknown Command (d...)");
				return;
			}
			break;
		case 'e':
			if(*(GlobalCurrentChar+1) == 'c'
				&&*(GlobalCurrentChar+2) == 'h'
				&&*(GlobalCurrentChar+3) == 'o')
			{
				GlobalCurrentChar += 4;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_ECHO;
    			break;
			}
			else
			{
				BotSyntaxError(4001);
				debugS("Unknown Command (e...)");
				return;
			}
		case 'f':
			if(*(GlobalCurrentChar+1) == 'r'
				&& *(GlobalCurrentChar+2) == 'a'
				&& *(GlobalCurrentChar+3) == 'm'
				&& *(GlobalCurrentChar+4) == 'e')
			{
                // frame
				GlobalCurrentChar += 5;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_FRAME;
			}
			else
			{
				BotSyntaxError(5001);
				debugS("Unknown Command (f...)");
				return;
			}
			break;
		case 'i':
			++GlobalCurrentChar;
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_I;
			break;
		case 'j':
			++GlobalCurrentChar;
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_J;
			break;
		case 'k':
			++GlobalCurrentChar;
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_K;
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
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_LAST;
			}
			else if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 'f'
				&&*(GlobalCurrentChar+3) == 't')
			{
                // left
				GlobalCurrentChar += 4;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_LEFT;
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
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_LOOP;
				colonstack[++cpoint] = ByteCodePointer++;
			}
			else
			{
				BotSyntaxError(7001);
				debugS("Unknown Command (l...)");
				return;
			}
			break;
		case 'm':
			if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 'm'
				&&*(GlobalCurrentChar+3) == '(')
			{
                // mem()
                GlobalCurrentChar += 4;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_MEM;
			}
			else if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 'm'
				&&*(GlobalCurrentChar+3) == 'h'
				&&*(GlobalCurrentChar+4) == '(')
			{
                // memh()
                GlobalCurrentChar += 5;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_MEMH;
			}
			else if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 'm'
				&&*(GlobalCurrentChar+3) == 'l'
				&&*(GlobalCurrentChar+4) == '(')
			{
                // meml()
                GlobalCurrentChar += 5;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_MEML;
			}
			else if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 'm'
				&&*(GlobalCurrentChar+3) == 'w'
				&&*(GlobalCurrentChar+4) == '(')
			{
                // memw()
                GlobalCurrentChar += 5;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_MEMW;
			}
			else
			{
				BotSyntaxError(8001);
				debugS("Unknown Command (m...)");
				return;
			}
			break;
		case 'n':
            // this is a little sketchy, but the principle works.
			++GlobalCurrentChar;
			negmode = true;
			break;
		case 'r':
			if(*(GlobalCurrentChar+1) == 'i'
				&&*(GlobalCurrentChar+2) == 'g'
				&&*(GlobalCurrentChar+3) == 'h'
				&&*(GlobalCurrentChar+4) == 't')
			{
                // right
                GlobalCurrentChar += 5;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_RIGHT;
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
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_RC;
			}
			else
			{
				BotSyntaxError(9001);
				debugS("Unknown Command (r...)");
				return;
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
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_START;
			}
			else if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 'l'
				&&*(GlobalCurrentChar+3) == 'e'
				&&*(GlobalCurrentChar+4) == 'c'
				&&*(GlobalCurrentChar+5) == 't')
			{
                // select
                GlobalCurrentChar += 6;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_SELECT;
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
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_SC;
			}
			// qfox: Added stop command. Should do exactly the same as pressing the "stop" button.\
			//	 But probably won't yet :)
			else if(*(GlobalCurrentChar+1) == 't'
				&&*(GlobalCurrentChar+2) == 'o'
				&&*(GlobalCurrentChar+3) == 'p')
			{
                // stop
                GlobalCurrentChar += 4;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_STOP;
            }
			else
			{
				BotSyntaxError(10001);
				debugS("Unknown Command (s...)");
				return;
			}
			break;
		case 'u':
			if(*(GlobalCurrentChar+1) == 'p')
			{
                // up
                GlobalCurrentChar += 2;
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_UP;
			}
			else
			{
				BotSyntaxError(11001);
				debugS("Unknown Command (p...)");
				return;
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
				BotSyntaxError(12001);
				debugS("Unknown Command (v...)");
				return;
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
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_LIT;
				Bytecode[ByteCodeField][ByteCodePointer++] = value;
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
				Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_LIT;
				Bytecode[ByteCodeField][ByteCodePointer++] = value;
				negmode = false;
				hexmode = false;
			}
			else
			{
				// If not in hexmode, we cant use these characters.
				BotSyntaxError(1003);
				debugS("Not in hexmode!");
				return;
			}
			break;
		case 'P':
			++GlobalCurrentChar;
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_P;
			break;
		case 'Q':
			++GlobalCurrentChar;
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_Q;
			break;
		case 'X':
			++GlobalCurrentChar;
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_X;
			break;
		case 'Y':
			++GlobalCurrentChar;
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_Y;
			break;
		case 'Z':
			++GlobalCurrentChar;
			Bytecode[ByteCodeField][ByteCodePointer++] = BOT_BYTE_Z;
			break;
		default:
			// Unknown characters should error out. You can use spaces.
			BotSyntaxError(12001);
			debugS("Unknown character...");
			return;
		}
	}
	return;
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
			value = (BotCounter[(Interpreter(false) & 0x7FFFFFFF) % 256] += value);
			if (single) return value;
			break;
		case BOT_BYTE_ABS:
			if (value < 0) value = 0-value;
			if (single) return value;
			break;
		case BOT_BYTE_BUTTON:
			if (value == 0) value = BotInput[1];
			else value &= BotInput[1];
			if (single) return value;
			break;
		case BOT_BYTE_C:
			value = BotCounter[(Interpreter(false) & 0x7FFFFFFF) % 256];
			if (single) return value;
			break;
		case BOT_BYTE_DOWN:
			value = 32;
			if (single) return value;
			break;
		case BOT_BYTE_ECHO:
			debug(value);
			break;
		case BOT_BYTE_FRAME:
			value = BotFrame;
			if (single) return value;
			break;
		case BOT_BYTE_I:
			value = BotLoopVarI;
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
		case BOT_BYTE_RC:
			value = (BotCounter[(Interpreter(false) & 0x7FFFFFFF) % 256] = 0);
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
			BotCounter[(Interpreter(false) & 0x7FFFFFFF) % 256] = value;
			if (single) return value;
			break;
		case BOT_BYTE_STOP:
            // stops the code with an error...
            BotSyntaxError(5000);
            EvaluateError = true;
			debugS("Stopped by code");
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

	sprintf(result, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
		code[0], code[1], code[2], code[3], code[4], 
		code[5], code[6], code[7], code[8], code[9],
		code[10], code[11], code[12], code[13], code[14], 
		code[15], code[16], code[17], code[18], code[19],
		code[20], code[21], code[22], code[23], code[24], 
		code[25], code[26], code[27], code[28], code[29]);
	SetDlgItemTextA(hwndBasicBot,GUI_BOT_KEYS,result);
}
/**
 * Update BasicBot input
 **/
void UpdateBasicBot()
{
	if(hwndBasicBot && BotRunning)
	{
		// If there is any input on the buffer, dont update yet.
		// [0] means the number of inputs left on the BotInput buffer
		if(BotInput[0])
			return;
		
		// 1; just computing 1 frame of input.
		BotInput[0] = 1;
		
		// When the bot starts, or the "end when" is reached, NewAttempt is set.
		//	this boolean tells you to reset/init stuff.
		if(NewAttempt)
		{
			SetNewAttempt();
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
			if(BotFrame > (BOT_MAXFRAMES-5) || Interpret(CODE_STOP) == BOT_MAXPROB)
			{

				int currentscore[4];
				bool better = false;

				// This was the last frame of this attempt
				NewAttempt = true;

				// This evaluates each of the four strings (over and over again)
				//	which can become rather sluggish.
				//	maximize, tie1, tie2, tie3
				for(i=0; i < 4; i++)
				{
					currentscore[i] = Interpret(CODE_MAX+i);
				}

				// Update last score
				UpdateLastGUI(currentscore);

				// Update avg score
				for (i=0; i<4; ++i)
					AverageScore[i] = ((AverageScore[i]*BotAttempts)+currentscore[i])/(BotAttempts+1);
				// show them
				UpdateAvgGUI();


				// compare all scores. if scores are not equal, a break _will_
				//	occur. else the next will be checked. if all is equal, the
				//	old best is kept.
				for(i=0; i < 4; i++)
				{
					if(currentscore[i] > BotBestScore[i])
					{
						better = true;
						break;
					}
					else if(currentscore[i] < BotBestScore[i])
					{
						break;
					}
				}
				
				// Update best
				if(better)
				{
					// Update the scores
					for(i = 0; i < 4; i++)
					{
						BotBestScore[i] = currentscore[i];
					}
					// Copy the new best run
					for(i = 0; i < AttemptPointer; i++)
					{
						BestAttempt[i] = CurrentAttempt[i];
					}
					// Set the remainder of the array to -1, indicating a "end of run"
					for(; i < BOT_MAXFRAMES; i++)
					{
						BestAttempt[i] = -1;
					}
					UpdateBestGUI();
				}
			}
			else // Goal not met
			{
				// Generate 1 frame of output
				BotInput[1] = 0;
				
				// For two players, respectfully compute the probability
				//	for A B select start up down left and right buttons. In
				//	that order. If the number is higher then the generated
				//	random number, the button is pressed :)
				for(i=0;i<16;i++)
				{
					res = Interpret(i);
					if(GetRandom(BOT_MAXPROB) < res)
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
				if(AttemptPointer < BOT_MAXFRAMES)
				{
					CurrentAttempt[AttemptPointer] = BotInput[1];
					AttemptPointer++;
				}
				
				// Run extra commands
				Interpret(CODE_EXTRA);
				// Remember the buttons pressed in this frame
				LastButtonPressed = BotInput[1];
			}
		}

		// Update statistics
		// Flag 17 for input means the code will load a savestate next. looks kinda silly. to be changed.
		if(BotInput[1] == 65536)
		{
			BotAttempts++;
		}
		else
		{
			BotFrames++;
			// BotFrame is redundant? always equal to AttemptPointer
			BotFrame++;
		}
		// Increase the statistics
		if((BotFrames % 500) == 0)
		{
			UpdateCountersGUI();
		}
	}
}


/**
 * Check the current settings for syntax validity
 **/
static void CheckCode()
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
	ofn.lpstrInitialDir=BasicBotDir;
	if(GetSaveFileName(&ofn))
	{
		// Save the directory
		if(ofn.nFileOffset < 1024)
		{
			free(BasicBotDir);
			BasicBotDir=(char*)malloc(strlen(ofn.lpstrFile)+1);
			strcpy(BasicBotDir,ofn.lpstrFile);
			BasicBotDir[ofn.nFileOffset]=0;
		}

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
		for(i=0;i<BOT_FORMULAS;i++)
		{
			for(j=0;j<BOT_MAXCODE;j++)
			{
				fputc(Formula[i][j],fp);
			}
		}
		fclose(fp);
		debugS("Saved settings!");
		return true;
	}
	debugS("Error saving settings!");
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
	ofn.lpstrInitialDir=BasicBotDir;

	if(GetOpenFileName(&ofn))
	{
		// Save the directory
		if(ofn.nFileOffset < 1024)
		{
			free(BasicBotDir);
			BasicBotDir=(char*)malloc(strlen(ofn.lpstrFile)+1); //mbg merge 7/18/06 added cast
			strcpy(BasicBotDir,ofn.lpstrFile);
			BasicBotDir[ofn.nFileOffset]=0;
		}

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
			return false;
		}
		
		// read the codes for the buttons for player one and
		//       two, and the end/result/ties.
		for(i=0;i<BOT_FORMULAS;i++)
		{
			// read BOT_MAXCODE bytes per codefield
			for(j=0;j<BOT_MAXCODE;j++)
			{
				Formula[i][j] = fgetc(fp);
				// if the char is 128, its 0 (?)
				if(Formula[i][j] & 128)
				{
					// todo: figure out wtf this is
					Formula[i][j] = 0;
				}
			}
		}
		// release lock and resources
		fclose(fp);
		debugS("Loaded file!");
		return true;
	}
	debugS("Error loading file!");
	return false;
}


/**
 * Moved code to start function to be called from other places.
 * The code puts all the code from gui to variables and starts bot.
 **/
static void StartBasicBot() {
	// todo: make sure you are or get into botmode here
	FCEU_SetBotMode(1);
	BotRunning = true;
	FromGUI();
	for (int i=0; i<4; ++i)
		AverageScore[4] = 0.0;
	SetDlgItemText(hwndBasicBot,GUI_BOT_RUN,(LPTSTR)"Stop!");
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
        int i;
		// buttons
        for(i = 0; i < 8; i++)
        {
        	// id 1000-1008 are for the buttons
			memset(Formula[i + (EditPlayerOne?0:8)],0,BOT_MAXCODE);
        	GetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayerOne?0:8)], BOT_MAXCODE);
			ToByteCode(Formula[i+(EditPlayerOne?0:8)],i+(EditPlayerOne?0:8));
        }
		memset(Formula[CODE_STOP],0,BOT_MAXCODE);
		GetDlgItemText(hwndBasicBot, GUI_BOT_END, Formula[CODE_STOP], BOT_MAXCODE);
		ToByteCode(Formula[CODE_STOP],CODE_STOP);
		memset(Formula[CODE_MAX],0,BOT_MAXCODE);
		GetDlgItemText(hwndBasicBot, GUI_BOT_MAX, Formula[CODE_MAX], BOT_MAXCODE);
		ToByteCode(Formula[CODE_MAX],CODE_MAX);
		memset(Formula[CODE_TIE1],0,BOT_MAXCODE);
		GetDlgItemText(hwndBasicBot, GUI_BOT_TIE1, Formula[CODE_TIE1], BOT_MAXCODE);
		ToByteCode(Formula[CODE_TIE1],CODE_TIE1);
		memset(Formula[CODE_TIE2],0,BOT_MAXCODE);
		GetDlgItemText(hwndBasicBot, GUI_BOT_TIE2, Formula[CODE_TIE2], BOT_MAXCODE);
		ToByteCode(Formula[CODE_TIE2],CODE_TIE2);
		memset(Formula[CODE_TIE3],0,BOT_MAXCODE);
		GetDlgItemText(hwndBasicBot, GUI_BOT_TIE3, Formula[CODE_TIE3], BOT_MAXCODE);
		ToByteCode(Formula[CODE_TIE3],CODE_TIE3);
		memset(Formula[CODE_EXTRA],0,BOT_MAXCODE);
        GetDlgItemText(hwndBasicBot, GUI_BOT_EXTRA, Formula[CODE_EXTRA], BOT_MAXCODE);
		ToByteCode(Formula[CODE_EXTRA],CODE_EXTRA);
		memset(Formula[CODE_COMMENTS],0,BOT_MAXCODE);
		GetDlgItemText(hwndBasicBot, GUI_BOT_COMMENTS, Formula[CODE_COMMENTS], BOT_MAXCODE);
		// DONT EVAL THE COMMENTS! gah
		
		// function, because we call this from a different button as well
		UpdateStatics();
	}
	UpdateFromGUI = false;
}

/**
 * Put all the editable values from the variables to the gui
 **/
static void ToGUI()
{
	if (hwndBasicBot && Formula && Formula[CODE_EXTRA])
	{
		int i;
		// buttons
		for(i = 0; i < 8; i++)
		{
			SetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayerOne?0:8)]);
		}
		SetDlgItemText(hwndBasicBot, GUI_BOT_END, Formula[CODE_STOP]);
		SetDlgItemText(hwndBasicBot, GUI_BOT_MAX, Formula[CODE_MAX]);
		SetDlgItemText(hwndBasicBot, GUI_BOT_TIE1, Formula[CODE_TIE1]);
		SetDlgItemText(hwndBasicBot, GUI_BOT_TIE2, Formula[CODE_TIE2]);
		SetDlgItemText(hwndBasicBot, GUI_BOT_TIE3, Formula[CODE_TIE3]);
		SetDlgItemText(hwndBasicBot, GUI_BOT_EXTRA, Formula[CODE_EXTRA]);
		SetDlgItemText(hwndBasicBot, GUI_BOT_COMMENTS, Formula[CODE_COMMENTS]);
		SetDlgItemText(hwndBasicBot, GUI_BOT_P, Formula[CODE_P]);
		SetDlgItemText(hwndBasicBot, GUI_BOT_Q, Formula[CODE_Q]);
		SetDlgItemText(hwndBasicBot, GUI_BOT_X, Formula[CODE_X]);
		SetDlgItemText(hwndBasicBot, GUI_BOT_Y, Formula[CODE_Y]);
		SetDlgItemText(hwndBasicBot, GUI_BOT_Z, Formula[CODE_Z]);
	}
}


/**
 * Initializes the variables to 0 (and frame=1)
 **/
static void InitVars()
{
	memset(Formula, 0, BOT_FORMULAS*BOT_MAXCODE);
	memset(Bytecode, 0, BOT_FORMULAS*BOT_MAXCODE*sizeof(int));
	int i;
	for(i = 0; i < BOT_FORMULAS; i++)
	{
		Formula[i][0] = '0';
	}
	Formula[CODE_STOP][0] = 'f';
	Formula[CODE_STOP][1] = 'r';
	Formula[CODE_STOP][2] = 'a';
	Formula[CODE_STOP][3] = 'm';
	Formula[CODE_STOP][4] = 'e';
	Formula[CODE_STOP][5] = '=';
	Formula[CODE_STOP][6] = '1';
	X = Y = Z = P = Prand = Q = Qrand = 0;
	Formula[CODE_COMMENTS][0] = 'N';
	Formula[CODE_COMMENTS][1] = 'o';
	Formula[CODE_COMMENTS][2] = 'n';
	Formula[CODE_COMMENTS][3] = 'e';
}

/**
 * Behaves like FromGUI(), except only for XYZPQ
 */
static void UpdateStatics()
{
	// These "statics" are only evaluated once
	//	(this also saves me looking up stringtoint functions ;)
	// todo: improve efficiency, crap atm due to conversion
	memset(Formula[CODE_X],0,BOT_MAXCODE);
	GetDlgItemTextA(hwndBasicBot, GUI_BOT_X, Formula[CODE_X], BOT_MAXCODE);
	memset(Formula[CODE_Y],0,BOT_MAXCODE);
	GetDlgItemTextA(hwndBasicBot, GUI_BOT_Y, Formula[CODE_Y], BOT_MAXCODE);
	memset(Formula[CODE_Z],0,BOT_MAXCODE);
	GetDlgItemTextA(hwndBasicBot, GUI_BOT_Z, Formula[CODE_Z], BOT_MAXCODE);
	memset(Formula[CODE_P],0,BOT_MAXCODE);
	GetDlgItemTextA(hwndBasicBot, GUI_BOT_P, Formula[CODE_P], BOT_MAXCODE);
	memset(Formula[CODE_Q],0,BOT_MAXCODE);
	GetDlgItemTextA(hwndBasicBot, GUI_BOT_Q, Formula[CODE_Q], BOT_MAXCODE);
	ToByteCode(Formula[CODE_X],CODE_X);
	ToByteCode(Formula[CODE_Y],CODE_Y);
	ToByteCode(Formula[CODE_Z],CODE_Z);
	ToByteCode(Formula[CODE_P],CODE_P);
	ToByteCode(Formula[CODE_Q],CODE_Q);
	X = Interpret(CODE_X);
	Y = Interpret(CODE_Y);
	Z = Interpret(CODE_Z);
	P = Interpret(CODE_P);
	Q = Interpret(CODE_Q);
}

/**
 * Called from windows. main gui (=dialog) control.
 **/
static BOOL CALLBACK BasicBotCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int i;
	char valuesboxmsg[2048];
	DSMFix(uMsg); // stops sound on 5 of the messages...
	switch(uMsg)
	{
	case WM_INITDIALOG:
		// todo: why cant i change gui controls in this event?
		break;
	case WM_CLOSE:
	case WM_QUIT:
		CrashWindow();
		break;
	case WM_COMMAND:
		switch(HIWORD(wParam))
		{
		case BN_CLICKED:
			bool p1 = false; // if a player radiobutton is pressed, this tells me which one
			switch(LOWORD(wParam))
			{
			case GUI_BOT_P1:
				if (EditPlayerOne) break;
				p1 = true;
			case GUI_BOT_P2: 
				if (!p1 && !EditPlayerOne) break;

				// if the player, that was clicked on, is already being edited, dont bother
				//	otherwise...
				EditPlayerOne = (p1 == true);
				// swap player code info
				for(i = 0; i < 8; i++)
				{
					GetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayerOne?8:0)],BOT_MAXCODE);
					SetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayerOne?0:8)]);
				}
				break;
			case GUI_BOT_SAVE:
				StopSound(); // required?
				FromGUI();
				SaveBasicBot();
				break;			
			case GUI_BOT_LOAD:
				StopSound(); // required?
				LoadBasicBot();
				ToGUI();
				break;
			case GUI_BOT_BEST:
				// if bot is running and no frames left on the botinput buffer
    			if(!BotRunning && BotInput[0] == 0)
				{
					// feed BotInput the inputs until -1 is reached
					for(i = 0; i < (BOT_MAXFRAMES-2) && BestAttempt[i] != -1; i++)
					{
						BotInput[i+2] = BestAttempt[i];
					}
					BotInput[0] = i+1; 		// the number of botframes currently in the variable
					BotInput[1] = 65536; 	// flag 17, when processed by input.h this causes the
											//	code to load the savestate. should really change
											//	this construction, its ugly (but i guess it works)
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
				StopSound();
				if(MessageBox(hwndBasicBot, "Clear all text?", "Confirm clear", MB_YESNO)==IDYES)
				{
					InitVars();
					ToGUI();
				}
				break;
			case GUI_BOT_CLOSE:
				CrashWindow();
				break;
			case GUI_BOT_BOTMODE:
				// toggle external input mode
				FCEU_SetBotMode(FCEU_BotMode()?0:1);
				//SetDlgItemText(hwndBasicBot,GUI_BOT_BOTMODE,FCEU_BotMode()?"Disable external input":"Enable external input");
				// no need for the previous line, will be done when UpdateExternalButton() gets called by FCEU_BotMode()
				break;
			case GUI_BOT_CHECK:
				// check the current code for errors, without starting attempts
				CheckCode();
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
				//MessageBox(hwndBasicBot, "Showing", "Hrm", MB_OK);
				//SetDlgItemText(hwndBasicBot,GUI_BOT_BOTMODE,"hello");
				FromGUI();
				ToByteCode(Formula[CODE_EXTRA],CODE_EXTRA);
				DebugByteCode(Bytecode[CODE_EXTRA]);
				GlobalCurrentCode = 0;
				ByteCodeField = CODE_EXTRA;
				error(Interpreter(false));
				break;
			case GUI_BOT_VALUES:
				// display all the #DEFINEs and static values
				sprintf(valuesboxmsg, 
					"%s\n%s\n\n%s%s\n%s%d\n%s%d\n%s%d\n%s%d",
					"These are the values that were",
					"hardcoded at compilation time:",
					"Basic Bot:		v", BBversion,
					"Max Frames:	", BOT_MAXFRAMES,
					"Max Codelength:	", BOT_MAXCODE,
					"Counters:		", BOT_MAXCOUNTERS,
					"Probability Range:	0 - ", BOT_MAXPROB);
				
				MessageBox(hwndBasicBot, valuesboxmsg, "Harcoded values", MB_OK);
				break;
			case GUI_BOT_RESET:
				ResetStats();
				break;
			default:
				break;
			}
		}
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
	CheckDlgButton(hwndBasicBot, GUI_BOT_P1, 1); // select the player1 radiobutton
	if (LoadBasicBotFile("default.bot"))
	{
		ToGUI();
		debugS("Loaded default.bot");
	}
	else
	{
		debugS("Error loading default.bot");
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
	SaveBasicBotFile("default.bot");
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
		SetDlgItemText(hwndBasicBot,GUI_BOT_BOTMODE,FCEU_BotMode()?"Disable external input":"Enable external input");
	}
}

/**
 * Reset the stats, and the gui as well...
 */
void ResetStats()
{
	BotAttempts = 0;
	BotFrames = 0;
	memset(BotBestScore,0,sizeof(int)*4);
	memset(BestAttempt,-1,BOT_MAXFRAMES*sizeof(int));
	AverageScore[0]=AverageScore[1]=AverageScore[2]=AverageScore[3]=0;
	UpdateFullGUI();
}
/**
 * Update all the values in the GUI
 */
void UpdateFullGUI()
{
	ToGUI();
	UpdateBestGUI();
	UpdateAvgGUI();
	int empty[4];
	empty[0]=empty[1]=empty[2]=empty[3]=0;
	UpdateLastGUI(empty);
	UpdateCountersGUI();
}
/**
 * Update the "best" parts of GUI
 */
void UpdateBestGUI()
{
	#define BOT_RESULTBUFSIZE (BOT_MAXFRAMES*sizeof(int))
	char tempstring[BOT_RESULTBUFSIZE];				// Used for the "result" part. last time i checked 1024*12 is > 4k, but ok.
	char symbols[] = "ABET^v<>ABET^v<>";
	bool seenplayer2 = false;						// Bool keeps track of player two inputs

	// Update best score
	sprintf(tempstring, "%d %d %d %d (%dth)", BotBestScore[0], BotBestScore[1], BotBestScore[2], BotBestScore[3], BotAttempts);
	SetDlgItemText(hwndBasicBot,GUI_BOT_BESTRESULT,tempstring);
	memset(tempstring, 0, BOT_RESULTBUFSIZE); // clear the array

	// Create the run in ascii
	int i,j,k;
	k = 0; // keep track of len, needs to be below bufsize
	// Make output as text
	// While you're not exceeding the max of frames, a
	//	frame had input, and you're not exceeding the bufsize...
	//	Warning: the second condition prevents a bufferoverrun,
	//		-21 is because between checks a max of 21 chars
	//		could be added.
	for(i = 0; i < BOT_MAXFRAMES && BestAttempt[i] != -1 && k < BOT_RESULTBUFSIZE-21; i++)
	{
		tempstring[k] = '0' + ((i/10) % 10);
		k++;
		tempstring[k] = '0' + (i%10);
		k++;
		tempstring[k] = '.';
		k++;
		seenplayer2=0;
		for(j = 0; j < 16; j++)
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
 * Update the averages
 */
void UpdateAvgGUI()
{
	SetDlgItemInt(hwndBasicBot,GUI_BOT_AVGMAX,AverageScore[0], TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_AVGTIE1,AverageScore[1], TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_AVGTIE2,AverageScore[2], TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_AVGTIE3,AverageScore[3], TRUE);
}
/**
 * Update the Last Scores
 */
void UpdateLastGUI(int last[])
{
	//	300 should be enough..?
	char lastscore[300];
	sprintf(lastscore, "%d %d %d %d (%dth)", last[0], last[1], last[2], last[3], BotAttempts);
	SetDlgItemText(hwndBasicBot,GUI_BOT_LAST,lastscore);
}

/**
 * Update the counters in the GUI
 */
void UpdateCountersGUI()
{
	SetDlgItemInt(hwndBasicBot,GUI_BOT_ATTEMPTS,BotAttempts,TRUE);
	SetDlgItemInt(hwndBasicBot,GUI_BOT_FRAMES,BotFrames,TRUE);
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
	memset(BotCounter, 0, BOT_MAXCOUNTERS*sizeof(int)); // Sets all the counters to 0.
	// Update the P and Q values to something new
	Prand = GetRandom(P);
	Qrand = GetRandom(Q);
}
