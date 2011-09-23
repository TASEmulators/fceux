#include <set>
#include <fstream>
#include <sstream>
#include <time.h>

#include "common.h"
#include "tasedit.h"
#include "taseditlib/taseditproj.h"
#include "fceu.h"
#include "debugger.h"
#include "replay.h"
#include "movie.h"
#include "utils/xstring.h"
#include "Win32InputBox.h"
#include "window.h"
#include "keyboard.h"
#include "joystick.h"
#include "help.h"
#include "main.h"	//For the GetRomName() function

using namespace std;

//to change header font
//http://forums.devx.com/archive/index.php/t-37234.html

int old_movie_readonly = -1;
int lastCursor;
int old_pauseframe;
bool old_show_pauseframe;
bool show_pauseframe;
bool TASEdit_focus = false;
int saved_eoptions = 0;

// vars saved in cfg file
int TasEdit_wndx, TasEdit_wndy;
bool TASEdit_follow_playback = true;
bool TASEdit_show_lag_frames = true;
bool TASEdit_restore_position = false;
int TASEdit_greenzone_capacity = GREENZONE_DEFAULT_CAPACITY;
extern bool muteTurbo;

string tasedithelp = "{16CDE0C4-02B0-4A60-A88D-076319909A4D}"; //Name of TASEdit Help page
char buttonNames[NUM_JOYPAD_BUTTONS][2] = {"A", "B", "S", "T", "U", "D", "L", "R"};

HWND hwndTasEdit = 0;
static HMENU hmenu, hrmenu;
static HWND hwndList, hwndHeader;
static WNDPROC hwndHeader_oldWndproc, hwndList_oldWndProc;

typedef std::set<int> TSelectionFrames;
static TSelectionFrames selectionFrames;

//hacky.. we need to think about how to convey information from the driver to the movie code.
//add a new fceud_ function?? blehhh maybe
extern EMOVIEMODE movieMode;

TASEDIT_PROJECT project;

static void GetDispInfo(NMLVDISPINFO* nmlvDispInfo)
{
	LVITEM& item = nmlvDispInfo->item;
	if(item.mask & LVIF_TEXT)
	{
		switch(item.iSubItem)
		{
			case COLUMN_ARROW:
			if(item.iImage == I_IMAGECALLBACK && item.iItem == currFrameCounter)
				item.iImage = 0;
			else
				item.iImage = -1;
			break;
			case COLUMN_FRAMENUM:
			case COLUMN_FRAMENUM2:
			U32ToDecStr(item.pszText,item.iItem);
			break;
			case COLUMN_JOYPAD1_A: case COLUMN_JOYPAD1_B: case COLUMN_JOYPAD1_S: case COLUMN_JOYPAD1_T:
			case COLUMN_JOYPAD1_U: case COLUMN_JOYPAD1_D: case COLUMN_JOYPAD1_L: case COLUMN_JOYPAD1_R:
			case COLUMN_JOYPAD2_A: case COLUMN_JOYPAD2_B: case COLUMN_JOYPAD2_S: case COLUMN_JOYPAD2_T:
			case COLUMN_JOYPAD2_U: case COLUMN_JOYPAD2_D: case COLUMN_JOYPAD2_L: case COLUMN_JOYPAD2_R:
			case COLUMN_JOYPAD3_A: case COLUMN_JOYPAD3_B: case COLUMN_JOYPAD3_S: case COLUMN_JOYPAD3_T:
			case COLUMN_JOYPAD3_U: case COLUMN_JOYPAD3_D: case COLUMN_JOYPAD3_L: case COLUMN_JOYPAD3_R:
			case COLUMN_JOYPAD4_A: case COLUMN_JOYPAD4_B: case COLUMN_JOYPAD4_S: case COLUMN_JOYPAD4_T:
			case COLUMN_JOYPAD4_U: case COLUMN_JOYPAD4_D: case COLUMN_JOYPAD4_L: case COLUMN_JOYPAD4_R:
			{
				int joy = (item.iSubItem - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
				int bit = (item.iSubItem - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
				uint8 data = currMovieData.records[item.iItem].joysticks[joy];
				if(data & (1<<bit))
				{
					item.pszText[0] = MovieRecord::mnemonics[bit];
					item.pszText[1] = 0;
				} else 
					item.pszText[0] = 0;
			}
			break;
		}
	}
}

#define CDDS_SUBITEMPREPAINT       (CDDS_SUBITEM | CDDS_ITEMPREPAINT)
#define CDDS_SUBITEMPOSTPAINT      (CDDS_SUBITEM | CDDS_ITEMPOSTPAINT)
#define CDDS_SUBITEMPREERASE       (CDDS_SUBITEM | CDDS_ITEMPREERASE)
#define CDDS_SUBITEMPOSTERASE      (CDDS_SUBITEM | CDDS_ITEMPOSTERASE)

static LONG CustomDraw(NMLVCUSTOMDRAW* msg)
{
	int cell_x, cell_y;
	switch(msg->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;
	case CDDS_SUBITEMPREPAINT:
		SelectObject(msg->nmcd.hdc,debugSystem->hFixedFont);
		cell_x = msg->iSubItem;
		cell_y = msg->nmcd.dwItemSpec;
		if(cell_x > COLUMN_ARROW)
		{
			if(cell_x == COLUMN_FRAMENUM || cell_x == COLUMN_FRAMENUM2)
			{
				// frame number
				if (cell_y == currFrameCounter || (cell_y == pauseframe-1 && show_pauseframe))
				{
					// current frame
					msg->clrTextBk = CUR_FRAMENUM_COLOR;
				} else if(cell_y < currMovieData.greenZoneCount && !currMovieData.savestates[cell_y].empty())
				{
					if (TASEdit_show_lag_frames && currMovieData.frames_flags[cell_y] && (currMovieData.frames_flags[cell_y] & LAG_FLAG_BIT))
					{
						// lag frame
						msg->clrTextBk = LAG_FRAMENUM_COLOR;
					} else
					{
						// green zone frame
						msg->clrTextBk = GREENZONE_FRAMENUM_COLOR;
					}
				} else msg->clrTextBk = NORMAL_FRAMENUM_COLOR;
			} else if((cell_x - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS == 0 || (cell_x - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS == 2)
			{
				// pad 1 or 3
				if (cell_y == currFrameCounter || (cell_y == pauseframe-1 && show_pauseframe))
				{
					// current frame
					msg->clrTextBk = CUR_INPUT_COLOR1;
				} else if(cell_y < currMovieData.greenZoneCount && !currMovieData.savestates[cell_y].empty())
				{
					if (TASEdit_show_lag_frames && currMovieData.frames_flags[cell_y] && (currMovieData.frames_flags[cell_y] & LAG_FLAG_BIT))
					{
						// lag frame
						msg->clrTextBk = LAG_INPUT_COLOR1;
					} else
					{
						// green zone frame
						msg->clrTextBk = GREENZONE_INPUT_COLOR1;
					}
				} else msg->clrTextBk = NORMAL_INPUT_COLOR1;
			} else if((cell_x - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS == 1 || (cell_x - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS == 3)
			{
				// pad 2 or 4
				if (cell_y == currFrameCounter || (cell_y == pauseframe-1 && show_pauseframe))
				{
					// current frame
					msg->clrTextBk = CUR_INPUT_COLOR2;
				} else if(cell_y < currMovieData.greenZoneCount && !currMovieData.savestates[cell_y].empty())
				{
					if (TASEdit_show_lag_frames && currMovieData.frames_flags[cell_y] && (currMovieData.frames_flags[cell_y] & LAG_FLAG_BIT))
					{
						// lag frame
						msg->clrTextBk = LAG_INPUT_COLOR2;
					} else
					{
						// green zone frame
						msg->clrTextBk = GREENZONE_INPUT_COLOR2;
					}
				} else msg->clrTextBk = NORMAL_INPUT_COLOR2;
			}
		}
		return CDRF_DODEFAULT;
	default:
		return CDRF_DODEFAULT;
	}
}

// called from the rest of the emulator when things happen and the tasedit should change to reflect it
void UpdateTasEdit()
{
	if(!hwndTasEdit) return;

	if(FCEUI_EmulationPaused()==0)
	{
		if (FCEUMOV_ShouldPause())
		{
			FCEUI_ToggleEmulationPause();
			turbo = false;
		}
	}

	//update the number of items
	int currLVItemCount = ListView_GetItemCount(hwndList);
	if(currMovieData.getNumRecords() != currLVItemCount)
	{
		ListView_SetItemCountEx(hwndList,currMovieData.getNumRecords(),LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
	}

	//update the cursor
	if(currFrameCounter != lastCursor)
	{
		FollowPlayback();
		//update the old and new rows
		RedrawRow(lastCursor);
		RedrawRow(currFrameCounter);
		UpdateWindow(hwndList);
		lastCursor = currFrameCounter;
	}

	// update flashing pauseframe
	if (old_pauseframe != pauseframe && old_pauseframe > 0) RedrawRow(old_pauseframe-1);
	old_pauseframe = pauseframe;
	old_show_pauseframe = show_pauseframe;
	if (pauseframe > 0)
		show_pauseframe = (int)(clock() / PAUSEFRAME_BLINKING_PERIOD) & 1;
	else
		show_pauseframe = false;
	if (old_show_pauseframe != show_pauseframe) RedrawRow(pauseframe-1);

	// update window caption
	if ((!old_movie_readonly) == movie_readonly)
	{
		old_movie_readonly = movie_readonly;
		if (movie_readonly)
		{
			SetWindowText(hwndTasEdit, "TAS Editor");
		} else 
		{
			SetWindowText(hwndTasEdit, "TAS Editor (Recording)");
		}
	}
}

void RedrawTasedit()
{
	InvalidateRect(hwndTasEdit,0,FALSE);
}
void RedrawList()
{
	InvalidateRect(hwndList,0,FALSE);
}
void RedrawRow(int index)
{
	ListView_RedrawItems(hwndList,index,index);
}

enum ECONTEXTMENU
{
	CONTEXTMENU_STRAY = 0,
	CONTEXTMENU_SELECTED = 1,
};

void ShowMenu(ECONTEXTMENU which, POINT& pt)
{
	HMENU sub = GetSubMenu(hrmenu,(int)which);
	TrackPopupMenu(sub,0,pt.x,pt.y,TPM_RIGHTBUTTON,hwndTasEdit,0);
}

void StrayClickMenu(LPNMITEMACTIVATE info)
{
	POINT pt = info->ptAction;
	ClientToScreen(hwndList,&pt);
	ShowMenu(CONTEXTMENU_STRAY,pt);
}

void RightClickMenu(LPNMITEMACTIVATE info)
{
	POINT pt = info->ptAction;
	ClientToScreen(hwndList,&pt);
	ShowMenu(CONTEXTMENU_SELECTED,pt);
}

void RightClick(LPNMITEMACTIVATE info)
{
	int index = info->iItem;
	int column = info->iSubItem;

	//stray clicks give a context menu:
	if(index == -1)
	{
		StrayClickMenu(info);
		return;
	}

	//make sure that the click is in our currently selected set.
	//if it is not, then we don't know what to do yet
	if(selectionFrames.find(index) == selectionFrames.end())
	{
		return;
	}
	
	RightClickMenu(info);
}

void InvalidateGreenZone(int after)
{
	if (currMovieData.greenZoneCount > after+1)
	{
		currMovieData.greenZoneCount = after+1;
		currMovieData.rerecordCount++;
		// either set playback cursor to the end of greenzone or run seeking to restore playback position
		if (currFrameCounter >= currMovieData.greenZoneCount)
		{
			if (TASEdit_restore_position)
			{
				if (pauseframe-1 > currFrameCounter)
					JumpToFrame(pauseframe-1);
				else
					JumpToFrame(currFrameCounter);
				turbo = true;
			} else
			{
				JumpToFrame(currMovieData.greenZoneCount-1);
			}
		}
	}
	// redraw list even if greenzone didn't change
	RedrawList();
}

bool JumpToFrame(int index)
{
	// Returns true if a jump to the frame is made, false if nothing's done. 
	if (index<0) return false; 

	if (index >= currMovieData.greenZoneCount)
	{
		/* Handle jumps outside greenzone. */
		if (JumpToFrame(currMovieData.greenZoneCount-1))
		{
			// continue from the end of greenzone
			if (FCEUI_EmulationPaused()) FCEUI_ToggleEmulationPause();
			turbo = (currMovieData.greenZoneCount-1+FRAMES_TOO_FAR < index);
			pauseframe = index+1;
			return true;
		}
		return false;
	}
	/* Handle jumps inside greenzone. */
	if (currMovieData.loadTasSavestate(index))
	{
		currFrameCounter = index;
		// if playback was seeking, pause emulation right here
		if (pauseframe > 0)
		{
			if (!FCEUI_EmulationPaused()) FCEUI_ToggleEmulationPause();
			pauseframe = -1;
		}
		turbo = false;
		return true;
	}
	//Search for an earlier frame with savestate
	int i = (index>0)? index-1 : 0;
	for (; i > 0; --i)
	{
		if (currMovieData.loadTasSavestate(i)) break;
	}
	// continue from the frame
	currFrameCounter = i;
	if (FCEUI_EmulationPaused()) FCEUI_ToggleEmulationPause();
	turbo = (i+FRAMES_TOO_FAR < index);
	pauseframe=index+1;
	if (!i)
	{
		//starting from frame 0
		poweron(true);
		MovieData::dumpSavestateTo(&currMovieData.savestates[0],0);
	}
	return true;
}

void DoubleClick(LPNMITEMACTIVATE info)
{
	int index = info->iItem;

	//stray click
	if(index == -1)
		return;

	//if the icon or frame columns were double clicked:
	if(info->iSubItem == COLUMN_ARROW)
	{

	} else if(info->iSubItem == COLUMN_FRAMENUM || info->iSubItem == COLUMN_FRAMENUM2)
	{
		JumpToFrame(index);
		ClearSelection();
		RedrawList();
	}
	else if(info->iSubItem >= COLUMN_JOYPAD1_A && info->iSubItem <= COLUMN_JOYPAD4_R)
	{
		//toggle the bit
		int joy = (info->iSubItem - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
		int bit = (info->iSubItem - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;

		if (info->uKeyFlags&(LVKF_SHIFT|LVKF_CONTROL))
		{
			//update multiple rows 
			for(TSelectionFrames::iterator it(selectionFrames.begin()); it != selectionFrames.end(); it++)
			{
				currMovieData.records[*it].toggleBit(joy,bit);
			}
			index=*selectionFrames.begin();
		} 
		else 
		{
			//update one row
			currMovieData.records[index].toggleBit(joy,bit);
		}
		InvalidateGreenZone(index);
	}
}

//removes all selections
static void ClearSelection()
{
	int frameCount = ListView_GetItemCount(hwndList);

	ListView_SetItemState(hwndList,-1,0, LVIS_SELECTED);

	selectionFrames.clear();
}

//insert frames at the currently selected positions.
static void InsertFrames()
{
	int frames = selectionFrames.size();

	//this is going to be slow.

	//to keep this from being even slower than it would otherwise be, go ahead and reserve records
	currMovieData.records.reserve(currMovieData.records.size()+frames);

	//insert frames before each selection
	for(TSelectionFrames::reverse_iterator it(selectionFrames.rbegin()); it != selectionFrames.rend(); it++)
	{
		currMovieData.insertEmpty(*it,1);
	}

	InvalidateGreenZone(*selectionFrames.begin());
	UpdateTasEdit();
	RedrawList();
}

//delete frames at the currently selected positions.
static void DeleteFrames()
{
	int frames = selectionFrames.size();

	if(frames == currMovieData.records.size())
	{
		MessageBox(hwndTasEdit,"Please don't delete all of the frames in the movie. This violates an internal invariant we need to keep.","Error deleting",0);
		return;	///adelikat: why not just add an empty frame in the event of deleting all frames?
	}
	//this is going to be _really_ slow.

	//insert frames before each selection
	int ctr=0;
	for(TSelectionFrames::reverse_iterator it(selectionFrames.rbegin()); it != selectionFrames.rend(); it++)
	{
		currMovieData.records.erase(currMovieData.records.begin()+*it);
	}

	int index = *selectionFrames.begin();
	if (index>0) --index;
	ClearSelection();
	InvalidateGreenZone(index);
	UpdateTasEdit();
}

//the column set operation, for setting a button for a span of selected values
static void ColumnSet(int column)
{
	int joy = (column - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
	if (joy < 0 || joy >= NUM_JOYPADS) return;
	int button = (column - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;

	//inspect the selected frames. count the set and unset rows
	int set=0, unset=0;
	for(TSelectionFrames::iterator it(selectionFrames.begin()); it != selectionFrames.end(); it++)
	{
		if(currMovieData.records[*it].checkBit(joy,button))
			set++;
		else unset++;
	}

	//if it is half and half, then set them all
	//if they are all set, unset them all
	//if they are all unset, set them all
	bool setz = (set==0);
	bool unsetz = (unset==0);
	bool newValue;
	
	//do nothing if we didnt even have any work to do
	if(setz && unsetz)
		return;
	//all unset.. set them
	else if(setz && !unsetz)
		newValue = true;
	//all set.. unset them
	else if(!setz && unsetz)
		newValue = false;
	//a mix. set them.
	else newValue = true;

	//operate on the data and update the listview
	for(TSelectionFrames::iterator it(selectionFrames.begin()); it != selectionFrames.end(); it++)
	{
		currMovieData.records[*it].setBitValue(joy,button,newValue);
	}
	InvalidateGreenZone(*selectionFrames.begin());
}

//Highlights all frames in current input log
static void SelectAll()
{
	ClearSelection();
	for(unsigned int i=0;i<currMovieData.records.size();i++)
	{
		selectionFrames.insert(i);
		ListView_SetItemState(hwndList,i,LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
	}
	RedrawList();
}

//copies the current selection to the clipboard
static bool Copy()
{
	if (selectionFrames.size()==0) return false;

	int cframe=*selectionFrames.begin()-1;
    try 
	{
		int range = *selectionFrames.rbegin() - *selectionFrames.begin()+1;
		//std::string outbuf clipString("TAS");

		std::stringstream clipString;
		clipString << "TAS " << range << std::endl;

		for(TSelectionFrames::iterator it(selectionFrames.begin()); it != selectionFrames.end(); it++)
		{
			if (*it>cframe+1)
			{
				clipString << '+' << (*it-cframe) << '|';
			}
			cframe=*it;

			int cjoy=0;
			for (int joy=0; joy<NUM_JOYPADS; ++joy)
			{
				while (currMovieData.records[*it].joysticks[joy] && cjoy<joy) 
				{
					clipString << '|';
					++cjoy;
				}
				for (int bit=0; bit<NUM_JOYPAD_BUTTONS; ++bit)
				{
					if (currMovieData.records[*it].joysticks[joy] & (1<<bit))
					{
						clipString << MovieRecord::mnemonics[bit];
					}
				}
			}
			clipString << std::endl;

			if (!OpenClipboard(hwndTasEdit))
				return false;
			EmptyClipboard();

			HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, clipString.str().size()+1);

			if (hGlobal==INVALID_HANDLE_VALUE)
			{
				CloseClipboard();
				return false;
			}
			char *pGlobal = (char*)GlobalLock(hGlobal);
			strcpy(pGlobal, clipString.str().c_str());
			GlobalUnlock(hGlobal);
			SetClipboardData(CF_TEXT, hGlobal);

			CloseClipboard();
		}
		
	}
	catch (std::bad_alloc e)
	{
		return false;
	}

	return true;
}

//cuts the current selection, copying it to the clipboard
static void Cut()
{
	if (Copy())
	{
		DeleteFrames();
	}
}

//pastes the current clipboard selection into current inputlog
static bool Paste()
{
	bool result = false;
	if (selectionFrames.size()==0)
		return false;

	int pos = *selectionFrames.begin();

	if (!OpenClipboard(hwndTasEdit))
		return false;
	
	HANDLE hGlobal = GetClipboardData(CF_TEXT);
	if (hGlobal)
	{
		char *pGlobal = (char*)GlobalLock((HGLOBAL)hGlobal);

		// TAS recording info starts with "TAS ".
		if (pGlobal[0]=='T' && pGlobal[1]=='A' && pGlobal[2]=='S')
		{
			int range;

			// Extract number of frames
			sscanf (pGlobal+3, "%d", &range);
			if (currMovieData.records.size()<static_cast<unsigned int>(pos+range))
			{
				currMovieData.insertEmpty(currMovieData.records.size(),pos+range-currMovieData.records.size());
			}

			pGlobal = strchr(pGlobal, '\n');
			int joy=0;
			--pos;
			
			while (pGlobal++ && *pGlobal!='\0')
			{
				char *frame = pGlobal;

				// Detect skipped frames in paste.
				if (frame[0]=='+')
				{
					pos += atoi(frame+1);
					while (*frame && *frame != '\n' && *frame!='|')
						++frame;
					if (*frame=='|') ++frame;
				} else
				{
					++pos;
				}

				currMovieData.records[pos].joysticks[0]=0;
				currMovieData.records[pos].joysticks[1]=0;
				int joy=0;

				while (*frame && *frame != '\n' && *frame !='\r')
				{
					switch (*frame)
					{
					case '|': // Joystick marker
						++joy;
						break;
					default:
						for (int bit=0; bit<NUM_JOYPAD_BUTTONS; ++bit)
						{
							if (*frame==MovieRecord::mnemonics[bit])
							{
								currMovieData.records[pos].joysticks[joy]|=(1<<bit);
								break;
							}
						}
						break;
					}
					++frame;
				}

				pGlobal = strchr(pGlobal, '\n');
			}
			InvalidateGreenZone(*selectionFrames.begin());
			result = true;
		}

		GlobalUnlock(hGlobal);
	}

	CloseClipboard();
	return result;
}

void AddMarker()
{
}
void RemoveMarker()
{
}


// ---------------------------------------------------------------------------------
//The subclass wndproc for the listview header
static LRESULT APIENTRY HeaderWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
	case WM_LBUTTONDBLCLK:
	case WM_SETCURSOR:
		return true;	// no column resizing
	case WM_LBUTTONDOWN:
		{
			//perform hit test
			HD_HITTESTINFO info;
			info.pt.x = GET_X_LPARAM(lParam);
			info.pt.y = GET_Y_LPARAM(lParam);
			SendMessage(hWnd,HDM_HITTEST,0,(LPARAM)&info);
			if(info.iItem >= COLUMN_JOYPAD1_A && info.iItem <= COLUMN_JOYPAD4_R)
				ColumnSet(info.iItem);
		}
	}
	return CallWindowProc(hwndHeader_oldWndproc,hWnd,msg,wParam,lParam);
}

//The subclass wndproc for the listview
static LRESULT APIENTRY ListWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
	case WM_CHAR:
		return 0;
	case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->code)
			{
			case HDN_BEGINTRACKW:
			case HDN_BEGINTRACKA:
			case HDN_TRACK:
				return true;	// no column resizing
			}
			break;
		}

	}
	return CallWindowProc(hwndList_oldWndProc,hWnd,msg,wParam,lParam);
}

//All dialog initialization
static void InitDialog()
{
	//prepare the listview
	ListView_SetExtendedListViewStyleEx(hwndList,
                             LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES ,
                             LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES );

	//subclass the header
	hwndHeader = ListView_GetHeader(hwndList);
	hwndHeader_oldWndproc = (WNDPROC)SetWindowLong(hwndHeader,GWL_WNDPROC,(LONG)HeaderWndProc);

	//subclass the whole listview, so we can block some keystrokes
	hwndList_oldWndProc = (WNDPROC)SetWindowLong(hwndList,GWL_WNDPROC,(LONG)ListWndProc);

	//setup all images for the listview
	HIMAGELIST himglist = ImageList_Create(8,12,ILC_COLOR8 | ILC_MASK,1,1);
	HBITMAP bmp = LoadBitmap(fceu_hInstance,MAKEINTRESOURCE(IDB_TE_ARROW));
	ImageList_AddMasked(himglist, bmp, 0xFF00FF);
	DeleteObject(bmp);
	ListView_SetImageList(hwndList,himglist,LVSIL_SMALL);
	//doesnt work well??
	//HIMAGELIST himglist = ImageList_LoadImage(fceu_hInstance,MAKEINTRESOURCE(IDB_TE_ARROW),12,1,RGB(255,0,255),IMAGE_BITMAP,LR_DEFAULTCOLOR);

	//setup columns
	LVCOLUMN lvc;
	int colidx=0;
	// arrow column
	lvc.mask = LVCF_WIDTH;
	lvc.cx = 12;
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	// frame number column
	lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
	lvc.fmt = LVCFMT_CENTER;
	lvc.cx = 92;
	lvc.pszText = "Frame#";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	// pads columns
	lvc.cx = 21;
	// add pads 1 and 2
	for (int joy = 0; joy < 2; ++joy)
	{
		for (int btn = 0; btn < NUM_JOYPAD_BUTTONS; ++btn)
		{
			lvc.pszText = buttonNames[btn];
			ListView_InsertColumn(hwndList, colidx++, &lvc);
		}
	}
	// add pads 3 and 4 and frame_number2
	if (currMovieData.fourscore) AddFourscoreColumns();

	//the initial update
	UpdateTasEdit();
}
void AddFourscoreColumns()
{
	LVCOLUMN lvc;
	lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
	lvc.fmt = LVCFMT_CENTER;
	lvc.cx = 21;
	int colidx = COLUMN_JOYPAD3_A;
	for (int joy = 0; joy < 2; ++joy)
	{
		for (int btn = 0; btn < NUM_JOYPAD_BUTTONS; ++btn)
		{
			lvc.pszText = buttonNames[btn];
			ListView_InsertColumn(hwndList, colidx++, &lvc);
		}
	}
	// frame number column again
	lvc.cx = 92;
	lvc.pszText = "Frame#";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
}
void RemoveFourscoreColumns()
{
	for (int i = COLUMN_FRAMENUM2; i >= COLUMN_JOYPAD3_A; --i)
	{
		ListView_DeleteColumn (hwndList, i);
	}
}

bool CheckSaveChanges()
{
	//TODO: determine if project has changed, and ask to save changes
	return true; 
}

//Creates a new TASEdit Project
static void NewProject()
{
	//determine if current project changed
	//if so, ask to save changes
	//close current project
	if (!CheckSaveChanges()) return;

	//TODO: close current project instance, create a new one with a non-parameterized constructor
}

//Opens a new Project file
static void OpenProject()
{
//TODO
	//determine if current project changed
	//if so, ask to save changes
	//close current project
		
	//If OPENFILENAME dialog successful, open up a completely new project instance and scrap the old one
	//Run the project Load() function to pull all info from the .tas file into this new project instance

	const char TPfilter[]="TASEdit Project (*.tas)\0*.tas\0\0";	

	OPENFILENAME ofn;								
	memset(&ofn,0,sizeof(ofn));						
	ofn.lStructSize=sizeof(ofn);					
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Open TASEdit Project...";
	ofn.lpstrFilter=TPfilter;

	char nameo[2048];								//File name
	strcpy(nameo, GetRomName());					//For now, just use ROM name

	ofn.lpstrFile=nameo;							
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_FILEMUSTEXIST;
	string initdir =  FCEU_GetPath(FCEUMKF_MOVIE);	
	ofn.lpstrInitialDir=initdir.c_str();

	if(GetOpenFileName(&ofn))							//If it is a valid filename
	{							
		std::string tempstr = nameo;					//Make a temporary string for filename
		char drv[512], dir[512], name[512], ext[512];	//For getting the filename!
		if(tempstr.rfind(".tas") == std::string::npos)	//If they haven't put ".tas" after it
		{
			tempstr.append(".tas");						//Stick it on ourselves
			splitpath(tempstr.c_str(), drv, dir, name, ext);	//Split the path...
			std::string filename = name;				//Get the filename
			filename.append(ext);						//Shove the extension back onto it...
			project.SetProjectFile(filename);			//And update the project's filename.
		} else {										//If they've been nice and done it for us...
			splitpath(tempstr.c_str(), drv, dir, name, ext);	//Split it up...
			std::string filename = name;				//Grab the name...
			filename.append(ext);						//Stick extension back on...
			project.SetProjectFile(filename);			//And update the project's filename.
		}
		//Set the project's name to the ROM name
		project.SetProjectName(GetRomName());
		//Set the fm2 name
		std::string thisfm2name = project.GetProjectName();
		thisfm2name.append(".fm2");
		project.SetFM2Name(thisfm2name);
		// load project and change number of listview columns if needed
		bool last_fourscore = currMovieData.fourscore;
		project.LoadProject(project.GetProjectFile());
		if (last_fourscore && !currMovieData.fourscore)
			RemoveFourscoreColumns();
		else if (!last_fourscore && currMovieData.fourscore)
			AddFourscoreColumns();
		if (!FCEUI_EmulationPaused()) FCEUI_ToggleEmulationPause();
		FollowPlayback();
		RedrawTasedit();
	}
}

// Saves current project
static void SaveProjectAs()
{
//Save project as new user selected filename
//flag project as not changed

	const char TPfilter[]="TASEdit Project (*.tas)\0*.tas\0All Files (*.*)\0*.*\0\0";	//Filetype filter

	OPENFILENAME ofn;								
	memset(&ofn,0,sizeof(ofn));						
	ofn.lStructSize=sizeof(ofn);					
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Save TASEdit Project As...";
	ofn.lpstrFilter=TPfilter;

	char nameo[2048];										//File name
	strcpy(nameo, GetRomName());							//For now, just use ROM name

	ofn.lpstrFile=nameo;									//More parameters
	ofn.lpstrDefExt="tas";
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
	string initdir =  FCEU_GetPath(FCEUMKF_MOVIE);			//Initial directory
	ofn.lpstrInitialDir=initdir.c_str();

	if(GetSaveFileName(&ofn))								//If it is a valid filename
	{
		std::string tempstr = nameo;						//Make a temporary string for filename
		char drv[512], dir[512], name[512], ext[512];		//For getting the filename!

		splitpath(tempstr.c_str(), drv, dir, name, ext);	//Split it up...
		std::string filename = name;						//Grab the name...
		filename.append(ext);								//Stick extension back on...
		project.SetProjectFile(filename);					//And update the project's filename.

		project.SetProjectName(GetRomName());				//Set the project's name to the ROM name
		std::string thisfm2name = project.GetProjectName();
		thisfm2name.append(".fm2");							//Setup the fm2 name
		project.SetFM2Name(thisfm2name);					//Set the project's fm2 name
		project.SaveProject();
	}

}

//Saves current project
static void SaveProject()
{
//TODO: determine if file exists, if not, do SaveProjectAs()
//Save work, flag project as not changed
	if (!project.SaveProject())
		SaveProjectAs();
}

//Takes a selected .fm2 file and adds it to the Project inputlog
static void Import()
{
	//Pull the fm2 header, comments, subtitle information out and put it into the project info
	//Pull the input out and add it to the main branch input log file
}

//Takes current inputlog and saves it as a .fm2 file
static void Export()
{
	//TODO: redesign this
	//Dump project header info into file, then comments & subtitles, then input log
	//This will require special prunctions, ::DumpHeader  ::DumpComments etc
	const char filter[]="FCEUX Movie File (*.fm2)\0*.fm2\0All Files (*.*)\0*.*\0\0";
	char fname[2048] = {0};
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Export TAS as...";
	ofn.lpstrFilter=filter;
	ofn.lpstrFile=fname;
	ofn.lpstrDefExt="fm2";
	ofn.nMaxFile=256;
	std::string initdir = FCEU_GetPath(FCEUMKF_MOVIE);
	ofn.lpstrInitialDir=initdir.c_str();
	if(GetSaveFileName(&ofn))
	{
		EMUFILE* osRecordingMovie = FCEUD_UTF8_fstream(fname, "wb");
		currMovieData.dump(osRecordingMovie,false);
		delete osRecordingMovie;
		osRecordingMovie = 0;
	}
}

static void Truncate()
{
	int frame = currFrameCounter;

	if (selectionFrames.size()>0)
	{
		frame=*selectionFrames.begin();
		JumpToFrame(frame);
	}
	ClearSelection();
	
	currMovieData.truncateAt(frame+1);
	InvalidateGreenZone(frame);
	UpdateTasEdit();
}

//likewise, handles a changed item range from the listview
static void ItemRangeChanged(NMLVODSTATECHANGE* info)
{
	bool ON = !(info->uOldState & LVIS_SELECTED) && (info->uNewState & LVIS_SELECTED);
	bool OFF = (info->uOldState & LVIS_SELECTED) && !(info->uNewState & LVIS_SELECTED);

	if(ON)
		for(int i=info->iFrom;i<=info->iTo;i++)
			selectionFrames.insert(i);
	else
		for(int i=info->iFrom;i<=info->iTo;i++)
			selectionFrames.erase(i);
}

//handles a changed item from the listview
//used to track selection
static void ItemChanged(NMLISTVIEW* info)
{
	int item = info->iItem;
	
	bool ON = !(info->uOldState & LVIS_SELECTED) && (info->uNewState & LVIS_SELECTED);
	bool OFF = (info->uOldState & LVIS_SELECTED) && !(info->uNewState & LVIS_SELECTED);

	//if the item is -1, apply the change to all items
	if(item == -1)
	{
		if(OFF)
		{
			selectionFrames.clear();
		}
		else
			FCEUD_PrintError("Unexpected condition in TasEdit ItemChanged. Please report.");
	}
	else
	{
		if(ON)
			selectionFrames.insert(item);
		else if(OFF) 
			selectionFrames.erase(item);
	}
}

BOOL CALLBACK WndprocTasEdit(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_PAINT:
			{
			}
			break;
		case WM_INITDIALOG:
			if (TasEdit_wndx==-32000) TasEdit_wndx=0; //Just in case
			if (TasEdit_wndy==-32000) TasEdit_wndy=0;
			SetWindowPos(hwndDlg,0,TasEdit_wndx,TasEdit_wndy,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER);

			hwndList = GetDlgItem(hwndDlg,IDC_LIST1);
			InitDialog();
			break; 

		case WM_MOVE:
			{
				if (!IsIconic(hwndDlg))
				{
					RECT wrect;
					GetWindowRect(hwndDlg,&wrect);
					TasEdit_wndx = wrect.left;
					TasEdit_wndy = wrect.top;

					WindowBoundsCheckNoResize(TasEdit_wndx,TasEdit_wndy,wrect.right);
				}
				break;
			}

		case WM_NOTIFY:

			switch(wParam)
			{
			case IDC_LIST1:
				switch(((LPNMHDR)lParam)->code)
				{
				case NM_CUSTOMDRAW:
					SetWindowLong(hwndDlg, DWL_MSGRESULT, CustomDraw((NMLVCUSTOMDRAW*)lParam));
					return TRUE;
				case LVN_GETDISPINFO:
					GetDispInfo((NMLVDISPINFO*)lParam);
					break;
				case NM_DBLCLK:
					DoubleClick((LPNMITEMACTIVATE)lParam);
					break;
				case NM_RCLICK:
					RightClick((LPNMITEMACTIVATE)lParam);
					break;
				case LVN_ITEMCHANGED:
					ItemChanged((LPNMLISTVIEW) lParam);
					break;
				case LVN_ODSTATECHANGED:
					ItemRangeChanged((LPNMLVODSTATECHANGE) lParam);
					break;
					
				}
				break;
			}
			break;
		
		case WM_CLOSE:
		case WM_QUIT:
			ExitTasEdit();
			break;

		case WM_ACTIVATEAPP:
			if((BOOL)wParam)
				TASEdit_focus = true;
			else
				TASEdit_focus = false;
			return DefWindowProc(hwndDlg,uMsg,wParam,lParam);
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			case ID_FILE_NEWPROJECT:
				NewProject();
				break;

			case ID_FILE_OPENPROJECT:
				OpenProject();
				break;
			
			case ACCEL_CTRL_S:
			case ID_FILE_SAVEPROJECT:
				SaveProject();
				break;

			case ID_FILE_SAVEPROJECTAS:
				SaveProjectAs();
				break;

			case ID_FILE_IMPORTFM2:
				Replay_LoadMovie(true);
				//Import(); //adelikat:  Putting the play movie dialog in its place until the import concept is refined.
				break;

			case ID_FILE_EXPORTFM2:
				Export();
				break;

			case ID_TASEDIT_FILE_CLOSE:
				ExitTasEdit();
				break;
		
			case ID_EDIT_SELECTALL:
				SelectAll();
				break;
			
			case ACCEL_CTRL_X:
			case ID_TASEDIT_CUT:
				Cut();
				break;

			case ACCEL_CTRL_C:
			case ID_TASEDIT_COPY:
				Copy();
				break;

			case ACCEL_CTRL_V:
			case ID_TASEDIT_PASTE:
				Paste();
				break;

			case ACCEL_CTRL_DELETE:
			case ID_TASEDIT_DELETE:
			case ID_CONTEXT_SELECTED_DELETEFRAMES:
				if (selectionFrames.size()) DeleteFrames();
				break;

			case ID_EDIT_ADDMARKER:
			case ID_CONTEXT_SELECTED_ADDMARKER:
				AddMarker();
				break;

			case ID_EDIT_REMOVEMARKER:
			case ID_CONTEXT_SELECTED_REMOVEMARKER:
				RemoveMarker();
				break;

			case ACCEL_CTRL_T:
			case ID_EDIT_TRUNCATE:
			case ID_CONTEXT_SELECTED_TRUNCATE:
			case ID_CONTEXT_STRAY_TRUNCATE:
				Truncate();
				break;

			case ID_HELP_TASEDITHELP:
				OpenHelpWindow(tasedithelp);
				//link to TASEdit in help menu
				break;

			case MENU_CONTEXT_STRAY_INSERTFRAMES:
			case ID_CONTEXT_SELECTED_INSERTFRAMES2:
				{
					int frames = 1;
					if(CWin32InputBox::GetInteger("Insert Frames", "How many frames?", frames, hwndDlg) == IDOK)
					{
						if (frames > 0)
						{
							if (selectionFrames.size())
							{
								// insert at selection
								int index = *selectionFrames.begin();
								currMovieData.insertEmpty(index,frames);
								InvalidateGreenZone(index);
							} else
							{
								// insert at playback cursor
								currMovieData.insertEmpty(currFrameCounter,frames);
								InvalidateGreenZone(currFrameCounter);
							}
						}
					}
				}
				break;

			case ACCEL_CTRL_INSERT:
			case ID_CONTEXT_SELECTED_INSERTFRAMES:
				if (selectionFrames.size()) InsertFrames();
				break;

			case TASEDIT_FOWARD:
				//advance 1 frame
				JumpToFrame(currFrameCounter+1);
				FollowPlayback();
				break;
			case TASEDIT_REWIND:
				//rewinds 1 frame
				if (currFrameCounter>0) JumpToFrame(currFrameCounter-1);
				FollowPlayback();
				break;
			case TASEDIT_PLAYSTOP:
				//Pause/Unpses (Play/Stop) movie
				FCEUI_ToggleEmulationPause();
				// also cancel turbo-seeking
				if (FCEUI_EmulationPaused())
				{
					turbo = false;
					pauseframe = -1;
				}
				break;
			case TASEDIT_REWIND_FULL:
				//rewinds to beginning of greenzone
				JumpToFrame(FindBeginningOfGreenZone(0));
				FollowPlayback();
				break;
			case TASEDIT_FOWARD_FULL:
				//moves to the end of greenzone
				JumpToFrame(currMovieData.greenZoneCount-1);
				FollowPlayback();
				break;
			case ACCEL_CTRL_F:
			case ID_VIEW_FOLLOW_PLAYBACK:
				//switch "Follow playback" flag
				TASEdit_follow_playback ^= 1;
				CheckMenuItem(hmenu, ID_VIEW_FOLLOW_PLAYBACK, TASEdit_follow_playback?MF_CHECKED : MF_UNCHECKED);
				// if switched off then jump to selection
				if (TASEdit_follow_playback)
					FollowPlayback();
				else if (selectionFrames.size())
					ListView_EnsureVisible(hwndList,(int)*selectionFrames.begin(),FALSE);
				break;
			case ID_VIEW_SHOW_LAG_FRAMES:
				//switch "Highlight lag frames" flag
				TASEdit_show_lag_frames ^= 1;
				CheckMenuItem(hmenu, ID_VIEW_SHOW_LAG_FRAMES, TASEdit_show_lag_frames?MF_CHECKED : MF_UNCHECKED);
				RedrawList();
				break;
			case ACCEL_CTRL_P:
			case CHECK_AUTORESTORE_PLAYBACK:
				//switch "Auto-restore last playback position" flag
				TASEdit_restore_position ^= 1;
				CheckDlgButton(hwndTasEdit,CHECK_AUTORESTORE_PLAYBACK,TASEdit_restore_position?BST_CHECKED:BST_UNCHECKED);
				break;
			case ID_CONFIG_SETGREENZONECAPACITY:
				{
				//open input dialog
				int new_capacity = TASEdit_greenzone_capacity;
				if(CWin32InputBox::GetInteger("Greenzone capacity", "Keep savestates for how many frames?", new_capacity,  hwndDlg) == IDOK)
				{
					if (new_capacity < GREENZONE_MIN_CAPACITY)
						new_capacity = GREENZONE_MIN_CAPACITY;
					else if (new_capacity > GREENZONE_MAX_CAPACITY)
						new_capacity = GREENZONE_MAX_CAPACITY;
					if (new_capacity < TASEdit_greenzone_capacity)
					{
						TASEdit_greenzone_capacity = new_capacity;
						currMovieData.ClearGreenzoneTail();
						RedrawList();
					} else TASEdit_greenzone_capacity = new_capacity;
				}
				break;
				}

			case ID_CONFIG_MUTETURBO:
				muteTurbo ^= 1;
				CheckMenuItem(hmenu, ID_CONFIG_MUTETURBO, muteTurbo?MF_CHECKED : MF_UNCHECKED);
				break;

			}
		default:
			break;
	}
	return FALSE;
}

int FindBeginningOfGreenZone(int starting_index)
{
	for (int i=starting_index; i<currMovieData.greenZoneCount; ++i)
	{
		if (!currMovieData.savestates[i].empty()) return i;
	}
	return starting_index;
}
void FollowPlayback()
{
	if (TASEdit_follow_playback) ListView_EnsureVisible(hwndList,currFrameCounter,FALSE);
}

void EnterTasEdit()
{
	if(!FCEU_IsValidUI(FCEUI_TASEDIT)) return;

	// init variables
	lastCursor = old_pauseframe = -1;
	old_show_pauseframe = show_pauseframe = false;

	// either start new project or use current movie
	if (movieMode == MOVIEMODE_INACTIVE)
	{
		FCEUI_StopMovie();
		CreateCleanMovie();
		//reset the rom
		poweron(true);
		currFrameCounter = 0;
	}
	else
	{
		//use current movie to create a new project
		FCEUI_StopMovie();
	}
	// pause the emulator and enter tasedit mode
	FCEUI_SetEmulationPaused(1);
	FCEU_DispMessage("Tasedit engaged",0);
	movieMode = MOVIEMODE_TASEDIT;
	currMovieData.TryDumpIncremental();
	// window stuff
	if(!hwndTasEdit) hwndTasEdit = CreateDialog(fceu_hInstance,"TASEDIT",hAppWnd,WndprocTasEdit);
	if(hwndTasEdit)
	{
		// save "eoptions"
		saved_eoptions = eoptions;
		// clear "Run in background"
		eoptions &= ~EO_BGRUN;
		// set "Background TASEdit input"
		KeyboardSetBackgroundAccessBit(KEYBACKACCESS_TASEDIT);
		JoystickSetBackgroundAccessBit(JOYBACKACCESS_TASEDIT);
		// "Set high-priority thread"
		eoptions |= EO_HIGHPRIO;
		DoPriority();
		// clear "Disable speed throttling"
		eoptions &= ~EO_NOTHROTTLE;
		UpdateCheckedMenuItems();

		hmenu = GetMenu(hwndTasEdit);
		hrmenu = LoadMenu(fceu_hInstance,"TASEDITCONTEXTMENUS");
		// check option ticks
		CheckMenuItem(hmenu, ID_VIEW_FOLLOW_PLAYBACK, TASEdit_follow_playback?MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, ID_VIEW_SHOW_LAG_FRAMES, TASEdit_show_lag_frames?MF_CHECKED : MF_UNCHECKED);
		CheckDlgButton(hwndTasEdit,CHECK_AUTORESTORE_PLAYBACK,TASEdit_restore_position?BST_CHECKED:BST_UNCHECKED);
		CheckMenuItem(hmenu, ID_CONFIG_MUTETURBO, muteTurbo?MF_CHECKED : MF_UNCHECKED);

		SetWindowPos(hwndTasEdit,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
	}
}
void ExitTasEdit()
{
	if (!CheckSaveChanges()) return;
	DestroyWindow(hwndTasEdit);
	hwndTasEdit = 0;
	turbo = false;
	pauseframe = -1;
	TASEdit_focus = false;
	// restore "eoptions"
	eoptions = saved_eoptions;
	DoPriority();
	UpdateCheckedMenuItems();
	// clear "Background TASEdit input"
	KeyboardClearBackgroundAccessBit(KEYBACKACCESS_TASEDIT);
	JoystickClearBackgroundAccessBit(JOYBACKACCESS_TASEDIT);
	// release memory
	currMovieData.clearGreenzone();
	movieMode = MOVIEMODE_INACTIVE;
	FCEU_DispMessage("Tasedit disengaged",0);
	CreateCleanMovie();
}
