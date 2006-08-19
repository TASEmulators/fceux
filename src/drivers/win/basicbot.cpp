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
#include "../../fceu.h"
#include "basicbot.h"




// qfox: v1 will be the last release by Luke, mine will start at v2 and
//	 go up each time something is released to the public, so you'll
//	 know your version is the latest or whatever. Version will be
//	 put in the project as well.
static char BBversion[] = "0.2.1";

static HWND hwndBasicBot = 0;	// qfox: GUI handle
static int BotRunning = 0;
static uint32 rand1=1, 
	      rand2=0x1337EA75, 
	      rand3=0x0BADF00D;
static char Formula[24][1024];  // qfox: These hold the 24 formula's entered in the GUI:
				//       First 8 are the buttons, in order, for player 1.
			        //	 Second 8 are the buttons, in order, for player 2.
			        //	 16=stop condition, 17=maximize function, 18=tie1,
			        //	 19=tie2, 20=tie3, 21=extra1, 22=extra2, 23=?
//static int BotInput[1024];        // qfox: missing from original source?
static int BotCounter[256];     // qfox: The counters. All ints, maybe change this to
			        //	 another type? Long or something?
static int EditPlayer = 1, 
	   BotFrame,
	   BotLoopVar = 0;
static int BotAttempts, 
	   BotFrames, 
	   BotBestScore[4], 
	   NewAttempt;
static int CurrentAttempt[1024],// qfox: A hardcoded limit of 1024 seems small to me. Maybe
				//	 I'm overlooking something here though, not quite sure
				//	 yet how things are saved (like, does the 1024 mean 
				//	 frames?).
	   BestAttempt[1024], 
	   AttemptPointer;
static bool ProcessCode = true;	// qfox: Seems to be a boolean that tells the code whether to 
			     	//       actually process counter commands. If set to 0, any
			     	//       of the counter() c() setcounter() resetcounter() rc()
			     	//       addcounter() and ac() commands will not be executed
			     	//       and simply ignored. Used for branching (if then else).
char *BasicBotDir = 0;
static bool EvaluateError = false;// qfox: Indicates whether an error was thrown.
static int LastEvalError;         // qfox: Last code thrown by the parser.
char * GlobalCurrentChar;         // qfox: This little variable will hold the position of the
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

static void SeedRandom(uint32 seed)
{
	rand1 = seed;
	if(rand1 == 0) rand1 = 1; //I forget if this is necessary.
}

//Simulation-quality random numbers: taus88 algorithm
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

//Should do something to notify user of an error...  eh
// qfox: All syntax-errors lead here. Some gui notification should be 
//       made next, but I don't know about that in C.
void BotSyntaxError(int errorcode)
{
     EvaluateError = true;
     LastEvalError = errorcode;
     debug(errorcode);
}

//recursively evaluate the formula
//horribly hacky, not meant to be a real expression evaluator!

// qfox: I am wondering whether Luke went with a function approach first, and changed the
//	 thing to recursive later on. The breaks are a little confusing. Seems to me like
//	 any function should return it's own value. Instead, they set "value" and return 0.
//	 This could bork up in the evaluation when scanning ahead for a number. At least,
//	 that's what I think. He made a few flaws however, I've tried to fix them.
//
//	 Changing the original behaviour, whatever it may be, to the following:
//	 Calls to EvaluateFormula parse the string for numbers. The code will remain on the
//	 same "depth" except for: '(' and '?', which will increase the depth by one, until
//	 a number is found. The ret argument indicates that the code should return immediately
//	 when a number is determined. Otherwise breaks are used. Anything between braces
//	 (like function arguments) is being parsed with ret as false. This allows you to
//	 use the counters like: abs(50 ac(32 ac(5)))
//	 If ret were true, the code for abs would search for just 50, return and screw up.
//	 The ')' will ensure the returning of a recursion (perhapse the code should first
//	 check whether there are as many '(' as ')', before even attempting to parse the
//	 string).
//	 Literals are explained below, where they are parsed.
//
//	 It is my intention to increase functionality of this parser and change it to create
//	 bytecode to be fed to a interpreter. This should increase performance, as the code
//	 currently evaluates all 23 lines completely EVERY frame. This is a lot of useless
//	 redundant computation. Converting the code to bytecode will eliminate string-parsing,
//	 error-checking and other overhead, without the risk of compromising the rest of the 
//	 code. Oh and by bytecode, I think I'll be meaning intcode since it's easier :)
//	 Think of it the way Java works when compiling and executing binaries.

// todo: boundrycheck every case in the switch! it might overflow and crash.
// todo: make loop nest-able
// todo: make code stop running if error is set
// todo: change error to int, make codes to clarify what went wrong
// todo: allow fetching (mem) of one byte or one int somehow
// todo: fix negative number handling (signed numbers)
// todo: check for existence of argument (low priority)
// todo: bracket matching, ?: matching.
// todo: let the code with errorchecking run through the code first so subsequent evals are faster.
// todo: cleanup "static"s here and there and read up on what they mean in C, i think i've got them confused

/**
 * Parse given string
 * - formula points to the string to be evaluated.
 * - nextposition is absoleted
 * - ret is absoleted
 */
static int EvaluateFormula(char * formula, char **nextposition, bool ret)
{
    EvaluateError = false;
    BotLoopVar = 0;
    GlobalCurrentChar = formula;
    return ParseFormula(ret);
}

static int ParseFormula(bool ret)
{
	int value=0; // qfox: value is local, no need to set it if ret is set.
	bool hexmode = false; // qfox: hexmode is boolean now
	bool negmode = false; // qfox: if a number is prefixed by a -
	while(*GlobalCurrentChar != 0)
	{
        if (EvaluateError) return value;
		switch(*GlobalCurrentChar)
		{
		case 0:
			// qfox: not the value 0 but the ascii value 0, d'oh, stringterminator.
			return value; // qfox: There is no more input, so return.
		case '(':
			// qfox: recurse one level deeper
			++GlobalCurrentChar;
			value = ParseFormula(false);
			if (ret) return value;
			break;
		case ')':
			// qfox: return from this recursion level
			++GlobalCurrentChar;
			return value;
		case ':':
			// todo: check whether we are actually in a iif
			++GlobalCurrentChar;
			return value;
		case 'x':
			hexmode = true; // qfox: hexmode is boolean now
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
				BotSyntaxError(1001); // qfox: "!" is not a valid command, but perhaps we could 
          						  // 	   use it as a prefix, checking if the next is 1000
		          				  // 	   (or 0) or not and inverting it. For now error it.
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
        case 10:                   // qfox: whitespace is whitespace.
        case 13:
        case 9:                    // qfox: or was char 8 tab? i always mix up
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
            // button A
			if(*(GlobalCurrentChar+1) == 't'
				&&*(GlobalCurrentChar+2) == 't'
				&&*(GlobalCurrentChar+3) == 'e'
				&&*(GlobalCurrentChar+4) == 'm'
				&&*(GlobalCurrentChar+5) == 'p'
				&&*(GlobalCurrentChar+6) == 't')
			{
                GlobalCurrentChar += 7;
				if (ProcessCode) value = BotAttempts;
			}
			else if(*(GlobalCurrentChar+1) == 'd'
				&&*(GlobalCurrentChar+2) == 'd'
				&&*(GlobalCurrentChar+3) == 'c'
				&&*(GlobalCurrentChar+4) == 'o'
				&&*(GlobalCurrentChar+5) == 'u'
				&&*(GlobalCurrentChar+6) == 'n'
				&&*(GlobalCurrentChar+7) == 't'
				&&*(GlobalCurrentChar+8) == 'e'
				&&*(GlobalCurrentChar+9) == 'r'
				&&*(GlobalCurrentChar+10)== '(')
			{
                // addcounter()
				GlobalCurrentChar += 11;
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
			else if(*(GlobalCurrentChar+1) == 'c'
				&&*(GlobalCurrentChar+2) == '(')
			{
				GlobalCurrentChar += 3;
				// qfox: next literal needs to be parsed regardless of ProcessCode's value
				int nextlit = ParseFormula(false);
				if(ProcessCode)
				{
					// todo: Throw an error for arguments >256?
					value = (BotCounter[(nextlit & 0x7FFFFFFF) % 256] += value);
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
				BotSyntaxError(1001);
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
				BotSyntaxError(1001);
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
				BotSyntaxError(1001);
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
				BotSyntaxError(1001);
				return value;
			}
			if (ret) return value;
			break;
		case 'i':
			++GlobalCurrentChar;
			if (ProcessCode) value = BotLoopVar;
			if (ret) return value;
			break;
		case 'l':
			if(*(GlobalCurrentChar+1) == 'e'
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
					if(BotLoopVar == 0)
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
						if (value>1) 
                        {
                            char * pos;
    						for(BotLoopVar = 0; BotLoopVar < value-1; BotLoopVar++)
    						{
                                pos = GlobalCurrentChar; // backup position (um, is it safe to copy pointers to chars like this?)
    							ParseFormula(false);
    							GlobalCurrentChar = pos;     // restore position
    						}
                        }
						ParseFormula(false); // the last time, but this time the char pointer remains moved
						BotLoopVar = 0;
						
					}
					else
					{
						// qfox: Can't nest loops (but if this is desired, perhaps we can fix that...)
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
				BotSyntaxError(1001);
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
                // todo: allow fetching of one byte or one int somehow
                GlobalCurrentChar += 4;
				int nextlit = ParseFormula(false);
				if (ProcessCode) {
                    // qfox: Throw error if beyond boundry?
                    nextlit = (nextlit & 0x7FFFFFFF) % 65536;
					value = ARead[nextlit](nextlit);
				}
			}
			else
			{
				BotSyntaxError(1001);
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
			if(*(GlobalCurrentChar+1) == 'a'
				&&*(GlobalCurrentChar+2) == 'm'
				&&*(GlobalCurrentChar+3) == '(')
			{
                // ram()
				// qfox: absoleted by mem(), perhaps it should be removed? 
                GlobalCurrentChar += 4;
                // qfox: next literal needs to be parsed regardless of ProcessCode's value
                int nextlit = ParseFormula(false);
				if (ProcessCode) value = RAM[(nextlit & 0x7FFFFFFF) % 2048];
			}
			else if(*(GlobalCurrentChar+1) == 'i'
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
				BotSyntaxError(1001);
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
				BotSyntaxError(1001);
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
				BotSyntaxError(1001);
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
				// todo: border check (we dont want to check beyond our stringlength)
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
				// todo: number-size check (int is max)
				// todo: boundry-check (we dont want to check beyond our stringlength)
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
			BotSyntaxError(1001);
			return value;
		}
	}
	return value;
}

// qfox: abuse the "frames" for debugging. as long as i cant edit the dialog i'm forced to do this
void debug(int n)
{
     SetDlgItemInt(hwndBasicBot,1017,n,FALSE);
}

//Update BasicBot input
void UpdateBasicBot()
{
	if(hwndBasicBot && BotRunning)
	{
		int i,j,k;
		char *dummy = "dum";
		
		if(BotInput[0])
			return;
		
		//Create the bot output...
		BotInput[0] = 1;
		if(NewAttempt)
		{
			BotInput[1] = 65536;
			NewAttempt = 0;
			AttemptPointer = 0;
			BotFrame = 0;
			memset(BotCounter, 0, 1024);
		}
		else
		{
			//Check if goal is met
			if(BotFrame > 1020 || (int) (NextRandom() % 1000) < EvaluateFormula(Formula[20], &dummy, false))
			{
				int currentscore[4];
				int better = 0;

				NewAttempt = 1;

				//Check if better score
				for(i=0; i < 4; i++)
				{
					currentscore[i] = EvaluateFormula(Formula[16+i], &dummy, false);
				}
				for(i=0; i < 4; i++)
				{
					if(currentscore[i] > BotBestScore[i])
					{
						better = 1;
						break;
					}
					else if(currentscore[i] < BotBestScore[i])
					{
						break;
					}
				}
				//Update best
				if(better)
				{
					char tempstring[4096];
					char symbols[16] = "ABET^v<>ABET^v<>";
					int seenplayer2=0;
					for(i = 0; i < 4; i++)
					{
						BotBestScore[i] = currentscore[i];
					}
					for(i = 0; i < AttemptPointer; i++)
					{
						BestAttempt[i] = CurrentAttempt[i];
					}
					for(; i < 1024; i++)
					{
						BestAttempt[i] = -1;
					}
					sprintf(tempstring, "%d %d %d %d (%d attempt)", BotBestScore[0], BotBestScore[1], BotBestScore[2], BotBestScore[3], BotAttempts);
					SetDlgItemText(hwndBasicBot,1018,tempstring);
					memset(tempstring, 0, 4096);
					k = 0;

					//Make output as text
					for(i = 0; i < 1024 && BestAttempt[i] != -1 && k < 4095; i++)
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
					SetDlgItemText(hwndBasicBot,1019,tempstring);
				}
			}
			else //Goal not met
			{
				//Generate 1 frame of output
				BotInput[1] = 0;
				
				for(i=0;i<16;i++)
				{
					if((int) (NextRandom() % 1000) < EvaluateFormula(Formula[i], &dummy, false))
					{
						BotInput[1] |= 1 << i;
					}
				}

				//Save what we've done
				if(AttemptPointer < 1024)
				{
					CurrentAttempt[AttemptPointer] = BotInput[1];
					AttemptPointer++;
				}
				
				//Run extra commands
				EvaluateFormula(Formula[21],&dummy, false);
				EvaluateFormula(Formula[22],&dummy, false);
			}
		}

		//Update statistics
		if(BotInput[1] == 65536)
		{
			BotAttempts++;
		}
		else
		{
			BotFrames++;
			BotFrame++;
		}
		if(BotFrames % 100 == 0)
		{
			SetDlgItemInt(hwndBasicBot,1037,BotAttempts,FALSE);
			// SetDlgItemInt(hwndBasicBot,1017,BotFrames,FALSE); // qfox: using this to debug...
		}
	}
    if(EvaluateError) StopBasicBot(); // qfox: When the evaluate function errors out, this
                                      //       var is set. In that case stop running.
}

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
			BasicBotDir=malloc(strlen(ofn.lpstrFile)+1);
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
		for(i=0;i<23;i++)
		{
			for(j=0;j<1024;j++)
			{
				fputc(Formula[i][j],fp);
			}
		}
		fclose(fp);
	}
}

static void StopBasicBot() 
{
    BotRunning = 0;
    BotAttempts = BotFrames = 0;
    BotBestScore[0] = BotBestScore[1] = BotBestScore[2] = BotBestScore[3] = -999999999;
    NewAttempt = 1;
}
static void StartBasicBot() {
    if (hwndBasicBot)
    {
        int i;
        BotRunning = 1;
        for(i = 0; i < 8; i++)
        {
        	GetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayer?0:8)],1024);
        }
        for(i = 0; i < 4; i++)
        {
        	GetDlgItemText(hwndBasicBot,1010+i,Formula[16+i],1024);
        }
        GetDlgItemText(hwndBasicBot, 1009, Formula[20],1024);
        GetDlgItemText(hwndBasicBot, 1020, Formula[21],1024);
        GetDlgItemText(hwndBasicBot, 1022, Formula[22],1024);
    }
}

//Loads a previously saved file
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
		
		//Save the directory
		if(ofn.nFileOffset < 1024)
		{
			free(BasicBotDir);
			BasicBotDir=malloc(strlen(ofn.lpstrFile)+1);
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
		
		for(i=0;i<23;i++)
		{
			for(j=0;j<1024;j++)
			{
				Formula[i][j] = fgetc(fp);
				if(Formula[i][j] & 128)
				{
					Formula[i][j] = 0;
				}
			}
		}
		fclose(fp);
	}
}


static BOOL CALLBACK BasicBotCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int i;
	DSMFix(uMsg);
	switch(uMsg)
	{
	case WM_INITDIALOG:
		break;
	case WM_CLOSE:
	case WM_QUIT:
		for(i = 0; i < 8; i++)
		{
			GetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayer?0:8)],1024);
		}
		for(i = 0; i < 4; i++)
		{
			GetDlgItemText(hwndBasicBot,1010+i,Formula[16+i],1024);
		}
		GetDlgItemText(hwndBasicBot, 1009, Formula[20],1024);
		GetDlgItemText(hwndBasicBot, 1020, Formula[21],1024);
		GetDlgItemText(hwndBasicBot, 1022, Formula[22],1024);
		DestroyWindow(hwndBasicBot);
		hwndBasicBot=0;
		break;
	case WM_COMMAND:
		switch(HIWORD(wParam))
		{
		case BN_CLICKED:
			switch(LOWORD(wParam))
			{
			case 1008: //change player button clicked
				EditPlayer ^= 1;
				SetDlgItemText(hwndBasicBot,1008,(LPTSTR)(EditPlayer?"Edit Player 2":"Edit Player 1"));
				SetDlgItemText(hwndBasicBot,2000,(LPTSTR)(EditPlayer?"Player 1":"Player 2"));
				for(i = 0; i < 8; i++)
				{
					GetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayer?8:0)],1024);
					SetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayer?0:8)]);
				}
				break;
			case 1014: //Save button clicked
				for(i = 0; i < 8; i++)
				{
					GetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayer?0:8)],1024);
				}
				for(i = 0; i < 4; i++)
				{
					GetDlgItemText(hwndBasicBot,1010+i,Formula[16+i],1024);
				}
				GetDlgItemText(hwndBasicBot, 1009, Formula[20],1024);
				GetDlgItemText(hwndBasicBot, 1020, Formula[21],1024);
				GetDlgItemText(hwndBasicBot, 1022, Formula[22],1024);
				StopSound();
				SaveBasicBot();
				break;			
			case 1015: //Load button clicked
				StopSound();
				LoadBasicBot();
				for(i = 0; i < 8; i++)
				{
					SetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayer?0:8)]);
				}
				for(i = 0; i < 4; i++)
				{
					SetDlgItemText(hwndBasicBot,1010+i,Formula[16+i]);
				}
				SetDlgItemText(hwndBasicBot, 1009, Formula[20]);
				SetDlgItemText(hwndBasicBot, 1020, Formula[21]);
				SetDlgItemText(hwndBasicBot, 1022, Formula[22]);
				break;
			case 1021: //Play best button clicked
    			if(BotRunning == 0 && BotInput[0] == 0)
				{
					for(i = 0; i < 1022 && BestAttempt[i] != -1; i++)
					{
						BotInput[i+2] = BestAttempt[i];
					}
					BotInput[0] = i+1;
					BotInput[1] = 65536;
				}
				break;
			case 1016: //Run button clicked
				if(BotRunning)
				{
                    StopBasicBot();
					//BotAttempts = BotFrames = 0;
					//BotBestScore[0] = BotBestScore[1] = BotBestScore[2] = BotBestScore[3] = -999999999;
					//NewAttempt = 1;
				}
				else // qfox: The else code does not need to be run (again) if stopping
                {
                     StartBasicBot();
                     /*
                    BotRunning = 1;
    				for(i = 0; i < 8; i++)
    				{
    					GetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayer?0:8)],1024);
    				}
    				for(i = 0; i < 4; i++)
    				{
    					GetDlgItemText(hwndBasicBot,1010+i,Formula[16+i],1024);
    				}
    				GetDlgItemText(hwndBasicBot, 1009, Formula[20],1024);
    				GetDlgItemText(hwndBasicBot, 1020, Formula[21],1024);
    				GetDlgItemText(hwndBasicBot, 1022, Formula[22],1024);
                    */
                }
				SetDlgItemText(hwndBasicBot,1016,(LPTSTR)(BotRunning?"Stop":"Run!"));
				break;
			case 1036: //Clear button clicked
				StopSound();
				if(MessageBox(hwndBasicBot, "Clear all text?", "Confirm clear", MB_YESNO)==IDYES)
				{
					memset(Formula,0, 24*1024);
					for(i = 0; i < 24; i++)
					{
						Formula[i][0] = '0';
					}
					Formula[20][0] = 'f';
					Formula[20][1] = 'r';
					Formula[20][2] = 'a';
					Formula[20][3] = 'm';
					Formula[20][4] = 'e';
					Formula[20][5] = '=';
					Formula[20][6] = '1';
					for(i = 0; i < 8; i++)
					{
						SetDlgItemText(hwndBasicBot,1000+i,Formula[i+(EditPlayer?0:8)]);
					}
					for(i = 0; i < 4; i++)
					{
						SetDlgItemText(hwndBasicBot,1010+i,Formula[16+i]);
					}
					SetDlgItemText(hwndBasicBot, 1009, Formula[20]);
					SetDlgItemText(hwndBasicBot, 1020, Formula[21]);
					SetDlgItemText(hwndBasicBot, 1022, Formula[22]);
				}
				break;

			default:
				break;
			}
		}

		if(!(wParam>>16)) //Close button clicked
		{
			switch(wParam&0xFFFF)
			{
			case 1:
				for(i = 0; i < 8; i++)
				{
					GetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayer?0:8)],1024);
				}
				for(i = 0; i < 4; i++)
				{
					GetDlgItemText(hwndBasicBot,1010+i,Formula[16+i],1024);
				}
				GetDlgItemText(hwndBasicBot, 1009, Formula[20],1024);
				GetDlgItemText(hwndBasicBot, 1020, Formula[21],1024);
				GetDlgItemText(hwndBasicBot, 1022, Formula[22],1024);
				DestroyWindow(hwndBasicBot);
				hwndBasicBot=0;
				break;
			}
		}
		break;
	default:
		break;
	}
	return 0;
}

void CreateBasicBot()
{
	static int BotNeedsInit = 1;
	int i;
	if(BotNeedsInit) 
	{
		BotNeedsInit = 0;
		SeedRandom(rand());
		memset(Formula,0, 24*1024);
		for(i = 0; i < 24; i++)
		{
			Formula[i][0] = '0';
		}
		Formula[20][0] = 'f';
		Formula[20][1] = 'r';
		Formula[20][2] = 'a';
		Formula[20][3] = 'm';
		Formula[20][4] = 'e';
		Formula[20][5] = '=';
		Formula[20][6] = '1';
	}

	if(hwndBasicBot) //If already open, give focus
	{
		SetFocus(hwndBasicBot);
		return;
	}
	
	//Create
	hwndBasicBot=CreateDialog(fceu_hInstance,"BASICBOT",NULL,BasicBotCallB);
	EditPlayer = 1;
	BotRunning = 0;
	for(i = 0; i < 8; i++)
	{
		SetDlgItemText(hwndBasicBot,1000+i,Formula[i]);
	}
	for(i = 0; i < 4; i++)
	{
		SetDlgItemText(hwndBasicBot,1010+i,Formula[16+i]);
	}
	SetDlgItemText(hwndBasicBot, 1009, Formula[20]);
	SetDlgItemText(hwndBasicBot, 1020, Formula[21]);
	SetDlgItemText(hwndBasicBot, 1022, Formula[22]);
}

