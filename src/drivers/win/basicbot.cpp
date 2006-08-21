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
static char BBversion[] = "0.2.1";

static HWND hwndBasicBot = 0;			// qfox: GUI handle
static bool BotRunning = false;			// qfox: Is the bot computing or not? I think...

static uint32 rand1=1, rand2=0x1337EA75, rand3=0x0BADF00D;

#define BOT_MAXCODE 1024				// qfox: max length of code, 1kb may be too little...
#define BOT_FORMULAS 21
static char Formula[BOT_FORMULAS][BOT_MAXCODE];	// qfox: These hold the 21 formula's entered in the GUI:
static int  CODE_1_A        = 0,
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
			CODE_TIE3       = 20;
static char ExtraCode[BOT_MAXCODE];		// qfox: Contains the extra code, executed after dealing
										//		 with input.

#define BOT_MAXCOUNTERS 256
static int BotCounter[BOT_MAXCOUNTERS]; // qfox: The counters. All ints, maybe change this to
					        			//	 	another type? Long or something?

static bool EditPlayerOne = true;				// qfox: Which player is currently shown in the GUI?
static int BotFrame,					// qfox: which frame is currently or last computed?
	   BotLoopVari = 0,					// qfox: used for loop(), "i" returns this value.
	   BotLoopVarj = 0,					// qfox: used for a loop() inside another loop(), "j" 
	   									//		 returns this value.
	   BotLoopVark = 0;					// qfox: used for a loop() inside a already nested 
	   									//		 loop(), "k" returns this value.

static int BotAttempts, 				// qfox: number of attempts tried so far
	   BotFrames, 						// qfox: number of frames computed so far
	   BotBestScore[4]; 				// qfox: maximize, tie1, tie2, tie3
static bool NewAttempt;					// qfox: tells code to reset certain run-specific info

static int LastButtonPressed;			// qfox: used for lastbutton command (note that this 
										//		 doesnt work for the very first frame of each attempt...)

static int CurrentAttempt[BOT_MAXFRAMES],//qfox: A hardcoded limit of 1024 seems small to me. Maybe
										//	 I'm overlooking something here though, not quite sure
										//	 yet how things are saved (like, does the 1024 mean 
										//	 frames?).
	   BestAttempt[BOT_MAXFRAMES], 		// qfox: only the best attempt is saved
	   AttemptPointer;					// qfox: Is this the same as BotFrame ?

static bool ProcessCode = true;			// qfox: Seems to be a boolean that tells the code whether to 
								     	//       actually process counter commands. If set to 0, any
								     	//       of the counter() c() setcounter() resetcounter() rc()
								     	//       addcounter() and ac() commands will not be executed
								     	//       and simply ignored. Used for branching (if then else).

char *BasicBotDir = 0;					// qfox: last used load/save dir is put in this var

static bool EvaluateError = false;		// qfox: Indicates whether an error was thrown.
static int LastEvalError;         		// qfox: Last code thrown by the parser.

char * GlobalCurrentChar;         		// qfox: This little variable will hold the position of the
	                                    //       string we are currently evaluating. I'm pretty sure
	                                    //       Luke used a structure of two arguments, where the
	                                    //       second argument somehow points to the currentchar
	                                    //       pointer (although I can't find where this happens
	                                    //       anywhere in his source) to remember increments 
	                                    //       made in a recursive higher depth (since these
	                                    //       were local variables, going back a depth would mean
	                                    //       loosing the changes). I think he used the second
	                                    //       argument to alter the original pointer. Since there
	                                    //       is only one evaluation-job going on at any single
	                                    //       moment, I think it's easier (better legible) to use
	                                    //       a global variable. Hence, that's what this is for.

bool UpdateFromGUI = false;				// qfox: when this flag is set, the next new attempt will reload
										//		 the settings from gui to mem. note that this cannot set
										//		 the settings for both players at once... maybe later.

float AverageScore[4];					// qfox: keep the average scores in here. to prevent overflow
										//		 this is not a total, but the average is computed after
										//		 every attempt and put back in here. this shouldn't
										//		 affect accuracy.

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
 * qfox: All syntax-errors lead here. Some gui notification should be 
 *       made next, but I don't know about that in C.
 **/
void BotSyntaxError(int errorcode)
{
     EvaluateError = true;
     LastEvalError = errorcode;
     error(errorcode);
     StopBasicBot();
}

/**
 * qfox: I am wondering whether Luke went with a function approach first, and changed the
 *	 thing to recursive later on. The breaks are a little confusing. Seems to me like
 *	 any function should return it's own value. Instead, they set "value" and return 0.
 *	 This could bork up in the evaluation when scanning ahead for a number. At least,
 *	 that's what I think. He made a few flaws however, I've tried to fix them.
 *
 *	 Changing the original behaviour, whatever it may be, to the following:
 *	 Calls to EvaluateFormula parse the string for numbers. The code will remain on the
 *	 same "depth" except for: '(' and '?', which will increase the depth by one, until
 *	 a number is found. The ret argument indicates that the code should return immediately
 *	 when a number is determined. Otherwise breaks are used. Anything between braces
 *	 (like function arguments) is being parsed with ret as false. This allows you to
 *	 use the counters like: abs(50 ac(32 ac(5)))
 *	 If ret were true, the code for abs would search for just 50, return and screw up.
 *	 The ')' will ensure the returning of a recursion (perhapse the code should first
 *	 check whether there are as many '(' as ')', before even attempting to parse the
 *	 string).
 *	 Literals are explained below, where they are parsed.
 *
 *	 It is my intention to increase functionality of this parser and change it to create
 *	 bytecode to be fed to a interpreter. This should increase performance, as the code
 *	 currently evaluates all 23 lines completely EVERY frame. This is a lot of useless
 *	 redundant computation. Converting the code to bytecode will eliminate string-parsing,
 *	 error-checking and other overhead, without the risk of compromising the rest of the 
 *	 code. Oh and by bytecode, I think I'll be meaning intcode since it's easier :)
 *	 Think of it the way Java works when compiling and executing binaries.
 **/
/**
 * Parse given string and return value of it. this 
 * function is not called recursively, only once per string!
 * - formula points to the string to be evaluated.
 * - returns the value of the evaluated string
 */
static int EvaluateFormula(char * formula)
{
	// qfox: reset values
    EvaluateError = false;
    // qfox: reset loop counters
    BotLoopVari = BotLoopVarj = BotLoopVark = 0;
    // qfox: set the global pointer to the string at the start of the string
    GlobalCurrentChar = formula;
    
    // qfox: first we want to quickly check whether the brackets are matching or not
    // 		 same for ? and :, both are pairs and should not exist alone
    char * quickscan = formula;
    // qfox: counters
    int iifs = 0, bracks = 0;
    // qfox: until EOL
    while (*quickscan != 0)
    {
    	switch(*quickscan)
    	{
    		case '?': ++iifs; break;
    		case ':': --iifs; break;
    		case '(': ++bracks; break;
    		case ')': --bracks; break;
    		default: break;
    	}
    	++quickscan;
    }
    if (iifs || bracks)
    	BotSyntaxError(1004);
    
    return ParseFormula(false);
}

/**
 * Parse one level of a string, return the value of the right most number
 * - ret indicates whether the function should return as soon as it determines a number,
 *       otherwise the code will continue until a ')', ':' or EOL
 * - returns the right most value once evaluated ("5 6" would return 6, "5+1" would return 11,
 *       "5+1 3+4" would return 7)
 **/
static int ParseFormula(bool ret)
{
	signed int value=0; 					// qfox: value is local, no need to set it if ret is set.
	bool hexmode = false; 					// qfox: hexmode is boolean now
	bool negmode = false; 					// qfox: if a number is prefixed by a -
	while(*GlobalCurrentChar != 0)
	{
        if (EvaluateError) return value;
		switch(*GlobalCurrentChar)
		{
		case 0:
			// qfox: not the value 0 but the ascii value 0, d'oh, stringterminator.
			return value; 					// qfox: There is no more input, so return.
		case '(':
			// qfox: recurse one level deeper
			++GlobalCurrentChar;
			value = ParseFormula(false);
			if (ret) return value;
			break;
		case ')':
			// qfox: return from this recursion level
			negmode = false; 
			hexmode = false;
			++GlobalCurrentChar;
			return value;
		case ':':
			// todo: check whether we are actually in a iif
			++GlobalCurrentChar;
			return value;
		case 'x':
			hexmode = true;
			++GlobalCurrentChar;
			break;
		case '+':
			++GlobalCurrentChar;
			value += ParseFormula(true);
			if (ret) return value;
			break;
		case '-':
			++GlobalCurrentChar;
			value -= ParseFormula(true);
			if (ret) return value;
			break;
		case '*':
			++GlobalCurrentChar;
			value *= ParseFormula(true);
			if (ret) return value;
			break;
		case '/':
			++GlobalCurrentChar;
			value /= ParseFormula(true);
			if (ret) return value;
			break;
		case '%':
			++GlobalCurrentChar;
			value %= ParseFormula(true);
			if (ret) return value;
			break;
		case '&':
			++GlobalCurrentChar;
			value &= ParseFormula(true);
			if (ret) return value;
			break;
		case '|':
			++GlobalCurrentChar;
			value |= ParseFormula(true);
			if (ret) return value;
			break;
		case '^':
			++GlobalCurrentChar;
			value ^= ParseFormula(true);
			if (ret) return value;
			break;
		case '=':
			++GlobalCurrentChar;
			value = ((value == ParseFormula(true)) ? 1000 : 0);
			if (ret) return value;
			break;
		case '>':
			if(*(GlobalCurrentChar + 1) == '=')
			{
                GlobalCurrentChar += 2;
				value = (value >= ParseFormula(true)) ? 1000 : 0;
            }
			else
			{
			    ++GlobalCurrentChar;
				value = (value > ParseFormula(true)) ? 1000 : 0;
            }
			if (ret) return value;
			break;
		case '<':
			if(*(GlobalCurrentChar + 1) == '=') 
            {
                GlobalCurrentChar += 2;
				value = (value <= ParseFormula(true)) ? 1000 : 0;
            }
			else
			{
			    ++GlobalCurrentChar;
				value = (value < ParseFormula(true)) ? 1000 : 0;
            }
			if (ret) return value;
			break;
		case '!':
			if(*(GlobalCurrentChar + 1) == '=')
			{
                GlobalCurrentChar += 2;
				value = (value != ParseFormula(true)) ? 1000: 0;
				if (ret) return value;
				break;
			}
			else
			{
				BotSyntaxError(1001); 	// qfox: "!" is not a valid command, but perhaps we could 
          						  		//		use it as a prefix, checking if the next is 1000
		          				  		//		(or 0) or not and inverting it. For now error it.
				return value;
			}
		case '?':
            // iif 
			// qfox: Nesting iif was screwed up because a nested iif was executed regardless of the
			//	 condition of it's parent.
			//   Also beforehand count whether the number of '?' is equal to the number of ':' as
			//   they can only appear together.
			//   Note: You need to bracket anything that needs to be evaluated at the '?', anything
			//         between the '?' and the ':' is evaluated as one value, and if you want more
			//         then one symbol to be used for the "else", you need to bracket it again.
			//         eg. (3 ac(5)) ? 2 ac(1) : (5 sc(3))
			//             3 ? 5 : (3+2)
            //         If you would not bracket the last example, only the 3 was used for the
            //         "else" result, instead of 5.
            ++GlobalCurrentChar;
			if (ProcessCode) 
            {
				int valuetrue, valuefalse; // qfox: preserve "value" until we return here.
				
				// qfox: if condition is met, get the first number, else the seconde.
				ProcessCode = (value != 0);
				
				// qfox: get first value (before the colon)
				valuetrue = ParseFormula(false);
					
				// qfox: If we got the first number, we don't want the second number. If we
				//	 didn't get the first number, we'll want the second number.
				ProcessCode = (ProcessCode == false);
				
				// qfox: get the second value (after the colon)
                valuefalse = ParseFormula(true);

				// qfox: Continue processing.				
				ProcessCode = true;
				
				// copy to-use value
				value = value ? valuetrue : valuefalse;
			}
			// qfox: We only need to move the cursor...
            //       ProcessCode is already false.
			else {
                ParseFormula(true);
                ParseFormula(true);
			}
			if (ret) return value;
			break;
        case 10:                   // qfox: whitespace is whitespace, I count spaces,
        case 13:                   //       returns and tabs as whitespace.
        case 9:
		case ' ':
            ++GlobalCurrentChar;
            hexmode = false;       // qfox: well, since hexmode is local, and there more
                                   //       then one literal is allowed to be on one
                                   //       line, we need to turn hexmode off at every
                                   //       space. Maybe even at every non-digit char...
                                   //       Hexmode should only be on for 1 number, the 
                                   //       next number can be a decimal digit (and x is
                                   //       not a toggle).
            negmode = false;
			break;                 // qfox: Never return here, always break.
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
				if (ProcessCode) value = BotAttempts;
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
				// qfox: next literal needs to be parsed regardless of ProcessCode's value
				int nextlit = ParseFormula(false);
				if(ProcessCode)
				{
					// todo: Throw an error for arguments >256?
					value = (BotCounter[(nextlit & 0x7FFFFFFF) % 256] += value);
				}
			}
			else if(*(GlobalCurrentChar+1) == 'b'
				&&*(GlobalCurrentChar+2) == 's'
				&&*(GlobalCurrentChar+3) == '(')
			{
                // abs()
				GlobalCurrentChar += 4;
				// qfox: next literal needs to be parsed regardless of ProcessCode's value
				int nextlit = ParseFormula(false);
				if (ProcessCode) 
				{
					value = nextlit < 0 ? -value : value;
				}
			}
			else
			{
                // c()
				++GlobalCurrentChar;
				if (ProcessCode) value = 1;
			}
			if (ret) return value;
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
				if (ProcessCode) 
                {
                     // qfox: Requesting button for 0 means all buttons
                     //       pressed will be returned. Makes it easy to check
                     //       whether some button was the only button pressed.
                     //		  BotInput[1] is the last input given (?)
                     if (value == 0) value = BotInput[1];
                     else value &= BotInput[1];
                }
			}
			else
			{
                // button B
				++GlobalCurrentChar;
				if (ProcessCode) value = 2;
			}
			if (ret) return value;
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
				// qfox: next literal needs to be parsed regardless of ProcessCode's value
				int nextlit = ParseFormula(false);
				if(ProcessCode)
				{
					// todo: Throw an error for arguments >256?
					value = BotCounter[(nextlit & 0x7FFFFFFF) % 256];
				}
			}
			else
			{
				BotSyntaxError(2001);
				return value;
			}
			if (ret) return value;
			break;
		case 'd':
			if(*(GlobalCurrentChar+1) == 'o'
				&&*(GlobalCurrentChar+2) == 'w'
				&&*(GlobalCurrentChar+3) == 'n')
			{
                // down
				GlobalCurrentChar += 4;
				if (ProcessCode) value = 32;
			}
			else
			{
				BotSyntaxError(3001);
				return value;
			}
			if (ret) return value;
			break;
		case 'e':
			// qfox: Added experimental echo command for debugging. 
			//       Echo does not change value and does not return anything.
			//       It does not count as a number, more like a nop (always
            //       breaks from switch).
			if(*(GlobalCurrentChar+1) == 'c'
				&&*(GlobalCurrentChar+2) == 'h'
				&&*(GlobalCurrentChar+3) == 'o')
			{
                // echo
				GlobalCurrentChar += 4;
				debug(value);
    			break;
			}
			else
			{
				BotSyntaxError(4001);
				return value;
			}
		case 'f':
			if(*(GlobalCurrentChar+1) == 'r'
				&& *(GlobalCurrentChar+2) == 'a'
				&& *(GlobalCurrentChar+3) == 'm'
				&& *(GlobalCurrentChar+4) == 'e')
			{
                // frame
				GlobalCurrentChar += 5;
				if (ProcessCode) value = BotFrame;
			}
			else
			{
				BotSyntaxError(5001);
				return value;
			}
			if (ret) return value;
			break;
		case 'i':
			++GlobalCurrentChar;
			if (ProcessCode) value = BotLoopVari;
			if (ret) return value;
			break;
		case 'j':
			++GlobalCurrentChar;
			if (ProcessCode) value = BotLoopVarj;
			if (ret) return value;
			break;
		case 'k':
			++GlobalCurrentChar;
			if (ProcessCode) value = BotLoopVark;
			if (ret) return value;
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
				if (ProcessCode) 
					value = LastButtonPressed;
			}
			else if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 'f'
				&&*(GlobalCurrentChar+3) == 't')
			{
                // left
				GlobalCurrentChar += 4;
				if (ProcessCode) value = 64;
			}
			else if(*(GlobalCurrentChar+1) == 'o'
				&&*(GlobalCurrentChar+2) == 'o'
				&&*(GlobalCurrentChar+3) == 'p'
				&&*(GlobalCurrentChar+4) == '(')
			{
                // loop
				GlobalCurrentChar += 5;
				// qfox: loop 1 or more times, unless we dont process code
				//	 returns the number of loops (doesn't change value)
				if(value > 0 && ProcessCode)
				{
					// qfox: if the third counter is 0, we can nest another loop()
					if(BotLoopVark == 0)
					{
						// qfox: Why was there a limit to 64k iterations?
						//       Changed starting counter to 0, makes more
						//       sense to coders. If users want this changed
						//       back to 1, it shall be done so.
						//       The reason for -1 is because the global char
						//       position is reset every loop. But we do want
						//       this to be moved on after this loop, so the
						//       last time won't be reset.
						//       Value must be >0 so the last one is safe.
                        int which = 0; // qfox: which loopcounter to reset afterwards?
                        if (value>1) 
                        {
                        	// qfox: I've just copied the code for each loopvar, seems easier
                        	//		 and probably faster then throwing nested ?: in the for
                            char * pos;
                            if (BotLoopVari == 0)
                            {
                            	which = 1;
	    						for(; BotLoopVari < value-1; BotLoopVari++)
	    						{
	                                pos = GlobalCurrentChar; // qfox: backup position (um, is it safe to copy pointers to chars like this?)
	    							ParseFormula(false);
	    							GlobalCurrentChar = pos; // qfox: restore position
	    						}
	    					}
                            else if (BotLoopVarj == 0)
                            {
                            	which = 2;
	    						for(; BotLoopVarj < value-1; BotLoopVarj++)
	    						{
	                                pos = GlobalCurrentChar; // qfox: backup position (um, is it safe to copy pointers to chars like this?)
	    							ParseFormula(false);
	    							GlobalCurrentChar = pos; // qfox: restore position
	    						}
	    					}
                            else
                            {
                            	which = 3;
	    						for(; BotLoopVark < value-1; BotLoopVark++)
	    						{
	                                pos = GlobalCurrentChar; // qfox: backup position (um, is it safe to copy pointers to chars like this?)
	    							ParseFormula(false);
	    							GlobalCurrentChar = pos; // qfox: restore position
	    						}
	    					}
                        }
						ParseFormula(false); 			 	 // qfox: the last time, but this time the char pointer remains moved
						// qfox: reset (the proper) counter
						if (which == 1) BotLoopVari = 0;
						else if (which == 2) BotLoopVarj = 0;
						else BotLoopVark = 0;
						
					}
					else
					{
						// qfox: For each nesting, we need an additional counter, I think nesting loop() three deep max will suffice here.
						BotSyntaxError(1002);
						return value;
					}
				}
				else
				{
					// qfox: Backup current setting. If "value == 0" then ProcessCode might be true
					// 	 in which case we want to restore that at the end. This part skips over
					// 	 the argument for loop().
					bool bak = ProcessCode;
					ProcessCode = false;
					ParseFormula(false);
					ProcessCode = bak;
				}
			}
			else
			{
				BotSyntaxError(7001);
				return value;
			}
			if (ret) return value;
			break;
		case 'm':
			if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 'm'
				&&*(GlobalCurrentChar+3) == '(')
			{
                // mem()
                GlobalCurrentChar += 4;
				int nextlit = ParseFormula(false);
				if (ProcessCode) {
                    // qfox: Throw error if beyond boundry?
                    nextlit = (nextlit & 0x7FFFFFFF) % 65536;
					value = ARead[nextlit](nextlit);
				}
			}
			else if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 'm'
				&&*(GlobalCurrentChar+3) == 'h'
				&&*(GlobalCurrentChar+4) == '(')
			{
                // memh()
                GlobalCurrentChar += 5;
                
				int nextlit = ParseFormula(false);
				if (ProcessCode) {
                    // qfox: Throw error if beyond boundry?
                    // qfox: memh() will return the high nibble of a byte, the value
                    //		 mem(arg) & 0xF0
                    nextlit = (nextlit & 0x7FFFFFFF) % 65536;
					value = (ARead[nextlit](nextlit) & 0xF0) >> 8;
				}
			}
			else if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 'm'
				&&*(GlobalCurrentChar+3) == 'l'
				&&*(GlobalCurrentChar+4) == '(')
			{
                // meml()
                GlobalCurrentChar += 5;
                
				int nextlit = ParseFormula(false);
				if (ProcessCode) {
                    // qfox: Throw error if beyond boundry?
                    // qfox: meml() will return the low nibble of a byte, the value
                    //		 mem(arg) & 0x0F
                    nextlit = (nextlit & 0x7FFFFFFF) % 65536;
					value = ARead[nextlit](nextlit) & 0x0F;
				}
			}
			else if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 'm'
				&&*(GlobalCurrentChar+3) == 'w'
				&&*(GlobalCurrentChar+4) == '(')
			{
                // memw()
                GlobalCurrentChar += 5;
                
				int nextlit = ParseFormula(false);
				if (ProcessCode) {
                    // qfox: Throw error if beyond boundry?
                    // qfox: memw() will return 2bytes (a word), the value
                    //		 (mem(arg) << 8) + (mem(arg+1)).
                    int v1 = (nextlit & 0x7FFFFFFF) % 65536;
                    int v2 = ((nextlit+1) & 0x7FFFFFFF) % 65536;
                    v1 = ARead[v1](v1);
					v2 = ARead[v2](v2);
					value = (v1<<8)+v2;
				}
			}
			else
			{
				BotSyntaxError(8001);
				return value;
			}
			if (ret) return value;
			break;
		case 'n':
            // qfox: this is a little buggy, but the principle works.
			negmode = true;
			++GlobalCurrentChar;
			break;
		case 'r':
			if(*(GlobalCurrentChar+1) == 'i'
				&&*(GlobalCurrentChar+2) == 'g'
				&&*(GlobalCurrentChar+3) == 'h'
				&&*(GlobalCurrentChar+4) == 't')
			{
                // right
                GlobalCurrentChar += 5;
				if (ProcessCode) value = 128;
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
                // qfox: next literal needs to be parsed regardless of ProcessCode's value
                int nextlit = ParseFormula(false);
				if(ProcessCode) value = (BotCounter[(nextlit & 0x7FFFFFFF) % 256] = 0);
			}
			else
			{
				BotSyntaxError(9001);
				return value;
			}
			if (ret) return value;
			break;
		case 's':
			if(*(GlobalCurrentChar+1) == 't'
				&&*(GlobalCurrentChar+2) == 'a'
				&&*(GlobalCurrentChar+3) == 'r'
				&&*(GlobalCurrentChar+4) == 't')
			{
                // start
                GlobalCurrentChar += 5;
				if (ProcessCode) value = 8;
			}
			else if(*(GlobalCurrentChar+1) == 'e'
				&&*(GlobalCurrentChar+2) == 'l'
				&&*(GlobalCurrentChar+3) == 'e'
				&&*(GlobalCurrentChar+4) == 'c'
				&&*(GlobalCurrentChar+5) == 't')
			{
                // select
                GlobalCurrentChar += 6;
				if (ProcessCode) value = 4;
			}
			// qfox: why was there no short version for setcounter()? There is now... :)
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
                // qfox: next literal needs to be parsed regardless of ProcessCode's value
                int nextlit = ParseFormula(false);
				if(ProcessCode) value = (BotCounter[(nextlit & 0x7FFFFFFF) % 256] = value);
			}
			// qfox: Added stop command. Should do exactly the same as pressing the "stop" button.\
			//	 But probably won't yet :)
			else if(*(GlobalCurrentChar+1) == 't'
				&&*(GlobalCurrentChar+2) == 'o'
				&&*(GlobalCurrentChar+3) == 'p')
			{
                // stop
                GlobalCurrentChar += 4;
                // qfox: stops the code with an error...
                BotSyntaxError(5000);
                EvaluateError = true;
				return 0;
            }
			else
			{
				BotSyntaxError(10001);
				return value;
			}
			if (ret) return value;
			break;
		case 'u':
			if(*(GlobalCurrentChar+1) == 'p')
			{
                // up
                GlobalCurrentChar += 2;
				if (ProcessCode) value = 16;
			}
			else
			{
				BotSyntaxError(11001);
				return value;
			}
			if (ret) return value;
			break;
		case 'v':
			if(*(GlobalCurrentChar+1) == 'a'
				&&*(GlobalCurrentChar+2) == 'l'
				&&*(GlobalCurrentChar+3) == 'u'
				&&*(GlobalCurrentChar+4) == 'e')
			{
				// value
				// qfox: value is completely transparent, more like a nop (or no-operation).
				//		 could be used in a iif where there is no else, or to make it more
				//		 clear how value is used internally...
				GlobalCurrentChar += 5;
			}
			else
			{
				BotSyntaxError(12001);
				return value;
			}
			if (ret) return value;
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
			// qfox: Not quite sure how the original literals worked, but it doesn't work in my 
			//	 implementation so I had to rewrite it. When a digit is encountered it will
			//	 start reading character (starting at the first). "value" is multiplied by 10
			//	 and the next digit is added to it. When a non-digit char is encountered, the
			//	 code stops parsing this number and behaves like everywhere else. (returning
			//   it if ret is true, breaking else).
			//	 If hexmode, this is done below, taking note of the additional "numbers".
			if (!hexmode) {
				// qfox: keep on reading input until no more numbers are encountered (0-9)
				int ahead = 0;
				value = 0;
				while (*(GlobalCurrentChar+ahead) >= '0' && *(GlobalCurrentChar+ahead) <= '9') {
					if (negmode) value = (value*10)-(*(GlobalCurrentChar+ahead) - '0');
					else value = (value*10)+(*(GlobalCurrentChar+ahead) - '0');
					++ahead;
				}
				// qfox: I don't think ahead needs to be incremented before adding it to
				//	 currentchar, because the while increments it for _every_ character.
				GlobalCurrentChar += ahead;
				if (ret) return value;
				break;
			}
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			// qfox: Hexmode does things a liiiitle differently. We need to multiply each number 
			//	 by 16 instead of 10, and if the character is ABCDEF we need to subtract 'A'
			//	 and add 10, instead of just subtracting '0'. Otherwise same principle.
			if(hexmode)
			{
				// qfox: Keep on reading input until no more numbers are encountered (0-9)
				int ahead = 0;
				value = 0;
				// todo: number-size check (int is max) -> do it when converting to bytecode
				while ((*(GlobalCurrentChar+ahead) >= '0' && *(GlobalCurrentChar+ahead) <= '9')
					|| (*(GlobalCurrentChar+ahead) >= 'A' && *(GlobalCurrentChar+ahead) <= 'F')) {
                    // qfox: next code is a little redundant, but it's either redundant or even harder to read.
					if (*(GlobalCurrentChar+ahead) >= '0' && *(GlobalCurrentChar+ahead) <= '9')
						if (negmode) value = (value*16)-(*(GlobalCurrentChar+ahead) - '0');
						else value = (value*16)+(*(GlobalCurrentChar+ahead) - '0');
					else
						if (negmode) value = (value*16)-((*(GlobalCurrentChar+ahead) - 'A')+10);
						else value = (value*16)+((*(GlobalCurrentChar+ahead) - 'A')+10);
					++ahead;
				}
				GlobalCurrentChar += ahead;
			}
			else
			{
				// qfox: If not in hexmode, we cant use these characters.
				BotSyntaxError(1003);
				return value;
			}
			if (ret) return value;
			break;
		default:
			// qfox: Unknown characters should error out from now on. You can use spaces.
			BotSyntaxError(12001);
			return value;
		}
	}
	return value;
}


/**
 * qfox: print a number
 **/
static void debug(int n)
{
     SetDlgItemInt(hwndBasicBot,GUI_BOT_DEBUG,n,TRUE);
}
/**
 * Print a string
 **/
static void debugS(LPCSTR s)
{
	SetDlgItemTextA(hwndBasicBot,GUI_BOT_DEBUG,s);
}

/**
 * qfox: when an error occurs, call this method to print the errorcode
 **/
static void error(int n)
{
     SetDlgItemInt(hwndBasicBot,GUI_BOT_ERROR,n,TRUE);
}


/**
 * Update BasicBot input
 **/
void UpdateBasicBot()
{
	if(hwndBasicBot && BotRunning)
	{
		int i,j,k;

		// qfox: If there is any input on the buffer, dont update yet.
		//		 [0] means the number of inputs left on the BotInput buffer
		if(BotInput[0])
			return;
		
		// luke: Create the bot output...
		// qfox: 1; just computing 1 frame of input.
		BotInput[0] = 1;
		
		// qfox: when the bot starts, or the "end when" is reached, NewAttempt is set.
		//		this boolean tells you to reset/init stuff.
		if(NewAttempt)
		{
			if (UpdateFromGUI)
				FromGUI();
			LastButtonPressed = 0;
			BotInput[1] = 65536; 		 // qfox: 17th flag, interpreted below as start of a new frame.
										 //		  it loads the savestate when encountered in input.cpp.
										 //		  should handle this more gracefully some day.
			NewAttempt = false;	 		 // qfox: running an attempt now
			AttemptPointer = 0;  		 // qfox: offset, frame counter?
			BotFrame = 0;        		 // qfox: frame counter...
			memset(BotCounter, 0, BOT_MAXCOUNTERS*4); // qfox: sets all the counters to 0. *4 because
													  //	   memset writes bytes and the counters are
													  //	   ints (4 bytes! :)
		}
		else
		{
			// luke: Check if goal is met
			// qfox: Goal is met if the frame count goes above 1020, or if the 
			//		 result was higher then a random number between 0 and 1000.
			//		 I think the random part needs to be removed as it has no
			//		 added value. You either get your goal or you dont. leave
			//		 the original, in case we want to revert it.
			
			//if(BotFrame > (BOT_MAXFRAMES-4) || (int) (NextRandom() % 1000) < EvaluateFormula(Formula[20]))
			if(BotFrame > (BOT_MAXFRAMES-5) || EvaluateFormula(Formula[CODE_STOP]) == 1000)
			{

				int currentscore[4];
				bool better = false;

				// qfox: this was the last frame of this attempt
				NewAttempt = true;

				// luke: Check if better score
				// qfox: this evaluates each of the four strings (over and over again)
				//		 which can become rather sluggish.
				//		 maximize, tie1, tie2, tie3
				for(i=0; i < 4; i++)
				{
					currentscore[i] = EvaluateFormula(Formula[CODE_MAX+i]);
				}

				// qfox: update last score
				//		 300 should be enough..?
				char lastscore[300];
				sprintf(lastscore, "%d %d %d %d (%dth)", currentscore[0], currentscore[1], currentscore[2], currentscore[3], BotAttempts);
				SetDlgItemText(hwndBasicBot,GUI_BOT_LAST,lastscore);
				// qfox: update avg score
				memset(lastscore, 0, 300); // clear the array

				// qfox: update the averages
				for (i=0; i<4; ++i)
					AverageScore[i] = ((AverageScore[i]*BotAttempts)+currentscore[i])/(BotAttempts+1);
				// qfox: show them
				//sprintf(lastscore, "%d.1 %d.1 %d.1 %d.1", AverageScore[0], AverageScore[1], AverageScore[2], AverageScore[3]);
				//SetDlgItemText(hwndBasicBot,GUI_BOT_AVG,lastscore);
				SetDlgItemInt(hwndBasicBot,GUI_BOT_AVGMAX,AverageScore[0], TRUE);
				SetDlgItemInt(hwndBasicBot,GUI_BOT_AVGTIE1,AverageScore[1], TRUE);
				SetDlgItemInt(hwndBasicBot,GUI_BOT_AVGTIE2,AverageScore[2], TRUE);
				SetDlgItemInt(hwndBasicBot,GUI_BOT_AVGTIE3,AverageScore[3], TRUE);

				// qfox: compare all scores. if scores are not equal, a break _will_
				//		 occur. else the next will be checked. if all is equal, the
				//		 old best is kept.
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
				
				// luke: Update best
				if(better)
				{

					#define BOT_RESULTBUFSIZE (BOT_MAXFRAMES*4)
					char tempstring[BOT_RESULTBUFSIZE];				// qfox: used for the "result" part. last time i checked 1024*12 is > 4k, but ok.
					char symbols[] = "ABET^v<>ABET^v<>"; 			//mbg merge 7/18/06 changed dim to unspecified to leave room for the null
					bool seenplayer2 = false;						// qfox: bool keeps track of player two inputs
					// qfox: update the scores
					for(i = 0; i < 4; i++)
					{
						BotBestScore[i] = currentscore[i];
					}
					// qfox: copy the new best run
					for(i = 0; i < AttemptPointer; i++)
					{
						BestAttempt[i] = CurrentAttempt[i];
					}
					// qfox: set the remainder of the array to -1, indicating a "end of run"
					for(; i < BOT_MAXFRAMES; i++)
					{
						BestAttempt[i] = -1;
					}

					// qfox: update best score
					sprintf(tempstring, "%d %d %d %d (%dth)", BotBestScore[0], BotBestScore[1], BotBestScore[2], BotBestScore[3], BotAttempts);
					SetDlgItemText(hwndBasicBot,GUI_BOT_BESTRESULT,tempstring);
					memset(tempstring, 0, BOT_RESULTBUFSIZE); // clear the array
					
					// qfox: create the run in ascii
					k = 0; // keep track of len, needs to be below bufsize
					// luke: Make output as text
					// qfox: while you're not exceeding the max of frames, a
					//		 frame had input, and you're not exceeding the bufsize...
					//		 Warning: the second condition prevents a bufferoverrun,
					//		          -21 is because between checks a max of 21 chars
					//				  could be added.
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

					// qfox: update best score keys
					SetDlgItemText(hwndBasicBot,GUI_BOT_KEYS,tempstring);
				}
			}
			else // luke: Goal not met
			{
				// luke: Generate 1 frame of output
				// qfox: reset input
				BotInput[1] = 0;
				
				// qfox: for two players, respectfully compute the probability
				//		 for A B select start up down left and right buttons. In
				//		 that order. If the number is higher then the generated
				//		 random number, the button is pressed :)
				for(i=0;i<16;i++)
				{
					if((int) (NextRandom() % 1000) < EvaluateFormula(Formula[i]))
					{
						// qfox: button flags:
						// 		button - player 1 - player 2
						//		A		 	1			 9
						//		B		 	2			10
						//		select	 	3			11
						//		start	 	4			12
						// 		up		 	5			13
						// 		down	 	6			14
						//		left	 	7			15
						//		right	 	8			16
						//		The input code will read the value of BotInput[1]
						//		If flag 17 is set, it will load a savestate, else
						//		it takes this input and puts the lower byte in 1
						//		and the upper byte in 2.
						BotInput[1] |= 1 << i;
					}
				}

				// luke: Save what we've done
				// qfox: only if we're not already at the max, should we 
				//		 perhaps not check sooner if we are going to
				//		 anyways?
				if(AttemptPointer < BOT_MAXFRAMES)
				{
					CurrentAttempt[AttemptPointer] = BotInput[1];
					AttemptPointer++;
				}
				
				// luke: Run extra commands
				EvaluateFormula(ExtraCode);
				// qfox: remember the buttons pressed in this frame
				LastButtonPressed = BotInput[1];
			}
		}

		// luke: Update statistics
		// qfox: flag 17 for input means the code will load a savestate next. looks kinda silly. to be changed.
		if(BotInput[1] == 65536)
		{
			BotAttempts++;
		}
		else
		{
			BotFrames++;
			// qfox: BotFrame is redundant? always equal to AttemptPointer
			BotFrame++;
		}
		// qfox: increase the statistics
		if((BotFrames % 500) == 0)
		{
			SetDlgItemInt(hwndBasicBot,GUI_BOT_ATTEMPTS,BotAttempts,TRUE);
			SetDlgItemInt(hwndBasicBot,GUI_BOT_FRAMES,BotFrames,TRUE);
		}
	}
}


/**
 * Check the current settings for syntax validity
 **/
void CheckCode()
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
		EvaluateFormula(Formula[i]);
	}
	debugS("Error at extra code");
	EvaluateFormula(ExtraCode);
	if (!EvaluateError)
	{
		debugS("Code syntax ok!");
		error(0);
	}
}

/**
 * qfox: save code seems to be good.
 **/
static void SaveBasicBot()
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
		int i,j;

		//Save the directory
		if(ofn.nFileOffset < 1024)
		{
			free(BasicBotDir);
			BasicBotDir=(char*)malloc(strlen(ofn.lpstrFile)+1); //mbg merge 7/18/06 added cast
			strcpy(BasicBotDir,ofn.lpstrFile);
			BasicBotDir[ofn.nFileOffset]=0;
		}

		//quick get length of nameo
		for(i=0;i<2048;i++)
		{
			if(nameo[i] == 0)
			{
				break;
			}
		}

		//add .bot if nameo doesn't have it
		if((i < 4 || nameo[i-4] != '.') && i < 2040)
		{
			nameo[i] = '.';
			nameo[i+1] = 'b';
			nameo[i+2] = 'o';
			nameo[i+3] = 't';
			nameo[i+4] = 0;
		}

		FILE *fp=FCEUD_UTF8fopen(nameo,"wb");
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
		for(j=0;j<BOT_MAXCODE;j++)
		{
			fputc(ExtraCode[j],fp);
		}
		fclose(fp);
	}
}

/**
 * luke: Loads a previously saved file
 * qfox: code seems good
 *		 do need to add ensurance that saved file has the same BOT_MAXCODE value 
 *		 as currently set, or offer some kind of support for this "problem"
 **/
static void LoadBasicBot()
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
		int i,j;
		
		// luke: Save the directory
		if(ofn.nFileOffset < 1024)
		{
			free(BasicBotDir);
			BasicBotDir=(char*)malloc(strlen(ofn.lpstrFile)+1); //mbg merge 7/18/06 added cast
			strcpy(BasicBotDir,ofn.lpstrFile);
			BasicBotDir[ofn.nFileOffset]=0;
		}

		FILE *fp=FCEUD_UTF8fopen(nameo,"rb");
		if(fgetc(fp) != 'b'
			|| fgetc(fp) != 'o'
			|| fgetc(fp) != 't'
			|| fgetc(fp) != 0)
		{
			fclose(fp);
			return;
		}
		
		for(i=0;i<BOT_FORMULAS;i++)
		{
			for(j=0;j<BOT_MAXCODE;j++)
			{
				Formula[i][j] = fgetc(fp);
				if(Formula[i][j] & 128)
				{
					Formula[i][j] = 0;
				}
			}
		}
		for(j=0;j<BOT_MAXCODE;j++)
		{
			ExtraCode[j] = fgetc(fp);
			if(ExtraCode[j] & 128)
			{
				ExtraCode[j] = 0;
			}
		}
		fclose(fp);
	}
}


/**
 * qfox: moved code to start function to be called from other places.
 *		 the code puts all the code from gui to variables and starts bot.
 **/
static void StartBasicBot() {
	// todo: make sure you are or get into botmode here
	BotRunning = true;
	FromGUI();
	for (int i=0; i<4; ++i)
		AverageScore[4] = 0.0;
	SetDlgItemText(hwndBasicBot,GUI_BOT_RUN,(LPTSTR)"Stop!");
}

/**
 * qfox: moved code to stop function to be called from other places
 **/
static void StopBasicBot() 
{
    BotRunning = false;
    BotAttempts = 0;
    BotFrames = 0;
    BotBestScore[0] = BotBestScore[1] = BotBestScore[2] = BotBestScore[3] = -999999999;
    NewAttempt = true;
	SetDlgItemText(hwndBasicBot,GUI_BOT_RUN,"Run!");
}


/**
 * qfox: put all the editable values from the gui to the variables
 *		 usable for a future "update" button :)
 **/
static void FromGUI()
{
	if (hwndBasicBot)
	{
        int i;
        for(i = 0; i < 8; i++)
        {
        	// qfox: id 1000-1008 are for the buttons
        	GetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayerOne?0:8)], BOT_MAXCODE);
        }
        for(i = 0; i < 4; i++)
        {
        	// qfox: id 1010-1014 are for goals
			GetDlgItemText(hwndBasicBot,1010+i,Formula[CODE_MAX+i], BOT_MAXCODE);
        }
        // qfox: id 1009 is for "end when"
		GetDlgItemText(hwndBasicBot, 1009, Formula[CODE_STOP], BOT_MAXCODE);
        
        GetDlgItemText(hwndBasicBot, GUI_BOT_EXTRA, ExtraCode, BOT_MAXCODE);
	}
	UpdateFromGUI = false;
}

/**
 * qfox: put all the editable values from the variables to the gui
 **/
static void ToGUI()
{
	if (hwndBasicBot && Formula && ExtraCode)
	{
		int i;
		for(i = 0; i < 8; i++)
		{
			SetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayerOne?0:8)]);
		}
		for(i = 0; i < 4; i++)
		{
			SetDlgItemText(hwndBasicBot,1010+i,Formula[CODE_MAX+i]);
		}
		// qfox: end when field
		SetDlgItemText(hwndBasicBot, 1009, Formula[CODE_STOP]);

		SetDlgItemText(hwndBasicBot, GUI_BOT_EXTRA, ExtraCode);
	}
}


/**
 * qfox: initializes the variables to 0 (and frame=1)
 **/
static void InitVars()
{
	memset(Formula, 0, BOT_FORMULAS*BOT_MAXCODE);
	memset(ExtraCode, 0, BOT_MAXCODE);
	int i;
	for(i = 0; i < BOT_FORMULAS; i++)
	{
		Formula[i][0] = '0';
	}
	ExtraCode[0] = '0';
	Formula[CODE_STOP][0] = 'f';
	Formula[CODE_STOP][1] = 'r';
	Formula[CODE_STOP][2] = 'a';
	Formula[CODE_STOP][3] = 'm';
	Formula[CODE_STOP][4] = 'e';
	Formula[CODE_STOP][5] = '=';
	Formula[CODE_STOP][6] = '1';
}

/**
 * qfox: called from windows. main gui (=dialog) control.
 **/
static BOOL CALLBACK BasicBotCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int i;
	DSMFix(uMsg); // qfox: stops sound on 5 of the messages...
	switch(uMsg)
	{
	case WM_INITDIALOG:
		// todo: load last used settings -> will have to figure out the load/save part first.
		// todo: why the hell is this event not firing?
		CheckDlgButton(hwndBasicBot, GUI_BOT_P1, 0); // select the player1 radiobutton
		break;
	case WM_CLOSE:
	case WM_QUIT:
		// todo: save last used settings
		// qfox: before closing, it quickly copies the contents to the variables... for some reason.
		FromGUI();
		DestroyWindow(hwndBasicBot);
		hwndBasicBot=0;
		break;
	case WM_COMMAND:
		switch(HIWORD(wParam))
		{
		case BN_CLICKED:
			bool p1 = false; // qfox: if a player radiobutton is pressed, this tells me which one
			switch(LOWORD(wParam))
			{
			case GUI_BOT_P1:
				if (EditPlayerOne) break;
				p1 = true;
			case GUI_BOT_P2: 
				if (!p1 && !EditPlayerOne) break;

				// qfox: if the player, that was clicked on, is already being edited, dont bother
				//		 otherwise...
				EditPlayerOne = (p1 == true);
				// qfox: swap player code info
				for(i = 0; i < 8; i++)
				{
					GetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayerOne?8:0)],BOT_MAXCODE);
					SetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayerOne?0:8)]);
				}
				break;
			case GUI_BOT_SAVE:
				StopSound(); // qfox: required?
				FromGUI();
				SaveBasicBot();
				break;			
			case GUI_BOT_LOAD:
				StopSound(); // qfox: required?
				LoadBasicBot();
				ToGUI();
				break;
			case GUI_BOT_BEST:
				// qfox: if bot is running and no frames left on the botinput buffer
    			if(BotRunning == 0 && BotInput[0] == 0)
				{
					// qfox: feed BotInput the inputs until -1 is reached
					for(i = 0; i < (BOT_MAXFRAMES-2) && BestAttempt[i] != -1; i++)
					{
						BotInput[i+2] = BestAttempt[i];
					}
					BotInput[0] = i+1; 		// qfox: the number of botframes currently in the variable
					BotInput[1] = 65536; 	// qfox: flag 17, when processed by input.h this causes the
											//		 code to load the savestate. should really change
											//		 this construction, its ugly (but i guess it works)
				}
				break;
			case GUI_BOT_RUN:
				if(BotRunning)
                    StopBasicBot();
				else
                    StartBasicBot();
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
				FromGUI(); // qfox: might tear this out later. seems a little pointless.
				DestroyWindow(hwndBasicBot);
				hwndBasicBot=0;
				break;
			case GUI_BOT_BOTMODE:
				// qfox: toggle external input mode
				FCEU_SetBotMode(FCEU_BotMode()?0:1);
				//SetDlgItemText(hwndBasicBot,GUI_BOT_BOTMODE,FCEU_BotMode()?"Disable external input":"Enable external input");
				// qfox: no need for the previous line, will be done when UpdateExternalButton() gets called by FCEU_BotMode()
				break;
			case GUI_BOT_CHECK:
				// qfox: check the current code for errors, without starting attempts
				CheckCode();
				break;
			case GUI_BOT_UPDATE:
				// qfox: put a flag up, when the current attempt stops, let the code 
				//		 refresh the variables with the data from gui, without stopping.
				UpdateFromGUI = true;
				break;
			default:
				break;
			}
		}
/*
		if(!(wParam>>16)) // luke: Close button clicked // qfox: why is this not one of the cases above. it'll take me ages to find it's id now :(
		{
			// qfox: there used to be a switch here with just one label, but I have no idea why...
			if ((wParam&0xFFFF) == 1)
			{
				FromGUI();
				DestroyWindow(hwndBasicBot);
				hwndBasicBot=0;
			}
		}
		break;
*/
	default:
		break;
	}
	return 0;
}

/**
 * qfox: creates dialog
 **/
void CreateBasicBot()
{
	SeedRandom(rand());
	InitVars();

	if(hwndBasicBot) //If already open, give focus
	{
		SetFocus(hwndBasicBot);
	}
	else
	{
		//Create
		hwndBasicBot=CreateDialog(fceu_hInstance,"BASICBOT",NULL,BasicBotCallB);
		EditPlayerOne = true;
		BotRunning = false;
		ToGUI();
	}
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