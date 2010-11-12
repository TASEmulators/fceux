#include <set>
#include <fstream>
#include <sstream>

#include "common.h"
#include "tasedit.h"
#include "taseditlib/taseditproj.h"
#include "fceu.h"
#include "debugger.h"
#include "replay.h"
#include "movie.h"
#include "utils/xstring.h"
#include "Win32InputBox.h"
#include "keyboard.h"
#include "joystick.h"
#include "help.h"
#include "main.h"	//For the GetRomName() function

using namespace std;

//to change header font
//http://forums.devx.com/archive/index.php/t-37234.html

int TasEdit_wndx, TasEdit_wndy;

string tasedithelp = "{16CDE0C4-02B0-4A60-A88D-076319909A4D}"; //Name of TASEdit Help page

HWND hwndTasEdit = 0;

static HMENU hmenu, hrmenu;
static int lastCursor;
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
		case 0:
			if(item.iImage == I_IMAGECALLBACK && item.iItem == currFrameCounter)
				item.iImage = 0;
			else
				item.iImage = -1;
			break;
		case 1:
			U32ToDecStr(item.pszText,item.iItem);
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9: 
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17: 
			{
				int joy = (item.iSubItem - 2)/8;
				int bit = (item.iSubItem - 2)%8;
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
	switch(msg->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;
	case CDDS_SUBITEMPREPAINT:
		SelectObject(msg->nmcd.hdc,debugSystem->hFixedFont);
		if((msg->iSubItem-2)/8==0)
			if((int)msg->nmcd.dwItemSpec < currMovieData.greenZoneCount && 
				!currMovieData.savestates[msg->nmcd.dwItemSpec].empty())
				msg->clrTextBk = RGB(192,255,192);
			else {}
		else if((int)msg->nmcd.dwItemSpec < currMovieData.greenZoneCount && 
			!currMovieData.savestates[msg->nmcd.dwItemSpec].empty())
				msg->clrTextBk = RGB(144,192,144);
			else msg->clrTextBk = RGB(192,192,192);
		return CDRF_DODEFAULT;
	default:
		return CDRF_DODEFAULT;
	}
}

void CreateProject(MovieData data)
{
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
		else if (turbo && (currFrameCounter &0xf))
			return;
	}


	//update the number of items
	int currLVItemCount = ListView_GetItemCount(hwndList);
	if(currMovieData.getNumRecords() != currLVItemCount)
	{
		ListView_SetItemCountEx(hwndList,currMovieData.getNumRecords(),LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
	}

	//update the cursor
	int newCursor = currFrameCounter;
	if(newCursor != lastCursor)
	{
		//unselect all prior rows
		TSelectionFrames oldSelected = selectionFrames;
		for(TSelectionFrames::iterator it(oldSelected.begin()); it != oldSelected.end(); it++)
			ListView_SetItemState(hwndList,*it,0, LVIS_FOCUSED|LVIS_SELECTED);

		//scroll to the row
		ListView_EnsureVisible(hwndList,newCursor,FALSE);
		//select the row
		ListView_SetItemState(hwndList,newCursor,LVIS_FOCUSED|LVIS_SELECTED,LVIS_FOCUSED|LVIS_SELECTED);
		
		//update the old and new rows
		ListView_Update(hwndList,newCursor);
		ListView_Update(hwndList,lastCursor);
		for(TSelectionFrames::iterator it(oldSelected.begin()); it != oldSelected.end(); it++)
			ListView_Update(hwndList,*it);
		
		lastCursor = newCursor;
	}

	static int old_movie_readonly=-1;
	if ((!old_movie_readonly) == movie_readonly) //Originally (old_movie_readonly = movie_readonly)
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

void RedrawList()
{
	InvalidateRect(hwndList,0,FALSE);
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

void LockGreenZone(int newstart)
{
	for (int i=1; i<newstart; ++i)
	{
		currMovieData.savestates[i].clear();
	}
}

void InvalidateGreenZone(int after)
{
	currMovieData.greenZoneCount = std::min(after+1,currMovieData.greenZoneCount);
}

/* A function that tries jumping to a given frame.  If unsuccessful, it than tries to jump to
   a previously good frame and fastforward to it. 

   Returns true if a jump to the frame is made, false if nothing done. 
   */
bool JumpToFrame(int index)
{
	if (index<0) return false; 

	/* Handle jumps outside greenzone. */
	if (index>currMovieData.greenZoneCount)
	{
		if (JumpToFrame(currMovieData.greenZoneCount-1))
		{
			if (FCEUI_EmulationPaused())
				FCEUI_ToggleEmulationPause();

			turbo=currMovieData.greenZoneCount+60<index; // turbo unless close
			pauseframe=index+1;

			return true;
		}
		return false;
	}

	if (static_cast<unsigned int>(index)<currMovieData.records.size() && 
		currMovieData.loadTasSavestate(index))
	{
		currFrameCounter = index;
		return true;
	}
	else 
	{
		/* Disable pause. */
		if (FCEUI_EmulationPaused())
			FCEUI_ToggleEmulationPause();

		int i = index>0? index-1:0;
		if (i>=static_cast<int>(currMovieData.records.size()))
			i=currMovieData.records.size()-1;

		/* Search for an earlier frame, and try warping to the current. */
		for (; i>0; --i)
		{
			if (currMovieData.loadTasSavestate(i))
			{
				currFrameCounter=i;
				turbo=i+60<index; // turbo unless close
				pauseframe=index+1;
				return true;
			}
		}

		poweron(true);
		currFrameCounter=0;
		MovieData::dumpSavestateTo(&currMovieData.savestates[0],0);
		turbo = index>60;
		pauseframe=index+1;
	}

	// Simply do a reset. 
	if (index==0)
	{
		poweron(false);
		currFrameCounter=0;
		MovieData::dumpSavestateTo(&currMovieData.savestates[0],0);
		return true;
	}

	return false;
}

void DoubleClick(LPNMITEMACTIVATE info)
{
	int index = info->iItem;

	//stray click
	if(index == -1)
		return;

	//if the icon or frame columns were double clicked:
	if(info->iSubItem == 0 || info->iSubItem == 1)
	{
		JumpToFrame(index);
	}
	else //if an input column was clicked:
	{
		//toggle the bit
		int joy = (info->iSubItem - 2)/8;
		int bit = (info->iSubItem - 2)%8;

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
			
			ListView_Update(hwndList,index);
		}

		InvalidateGreenZone(index);

		// If the change is in the past, move to it. 
		if(index < currFrameCounter &&
		   index < currMovieData.greenZoneCount)
		{
			JumpToFrame(index);
		}

		//redraw everything to show the reduced green zone
		RedrawList();
	}
}

//removes all selections
static void ClearSelection()
{
	int frameCount = ListView_GetItemCount(hwndList);

	ListView_SetItemState(hwndList,-1,0, LVIS_SELECTED);

	selectionFrames.clear();
	lastCursor=-1;
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

	if (currFrameCounter>=*selectionFrames.begin()) 
		JumpToFrame(*selectionFrames.begin());
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
	InvalidateGreenZone(index);

	//in the particular case of deletion, we need to make sure we reset currFrameCounter to something reasonable
	//why not the current green zone max?
	if (currFrameCounter>=index) 
		JumpToFrame(index);
	ClearSelection();
	UpdateTasEdit();
	RedrawList();
}

//the column set operation, for setting a button for a span of selected values
static void ColumnSet(int column)
{
	int joy = (column-2)/8;
	int button = (column-2)%8;

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
		//we would do this if we wanted to update the affected record. but that results in big operations
		//redrawing once per item set, which causes it to flicker and take forever.
		//so now we rely on the update at the end.
		//ListView_Update(hwndList,*it);
	}

	//reduce the green zone
	InvalidateGreenZone(*selectionFrames.begin());

	//redraw everything to show the reduced green zone
	RedrawList();
}

//Highlights all frames in current input log
static void SelectAll()
{
	ClearSelection();
	for(unsigned int i=0;i<currMovieData.records.size();i++)
		selectionFrames.insert(i);

	UpdateTasEdit();
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
			for (int joy=0; joy<2; ++joy)
			{
				while (currMovieData.records[*it].joysticks[joy] && cjoy<joy) 
				{
					clipString << '|';
					++cjoy;
				}
				for (int bit=0; bit<8; ++bit)
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
						for (int bit=0; bit<8; ++bit)
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

			// Invalidate and redraw.
			InvalidateGreenZone(*selectionFrames.begin());
			RedrawList();
			result=true;
		}

		GlobalUnlock(hGlobal);
	}

	CloseClipboard();
	return result;
}

//pastes the current clipboard selection into a new inputlog
static void PastetoNew()
{
}

//removes the current selection (does not put in clipboard)
static void Delete()
{
	DeleteFrames();
}

//Adds a marker to left column at selected frame (if multiple frames selected, it is placed at end of selection)
void AddMarker()
{
}

//Removes marker from selected frame (if multiple frames selected, all markers in selection removed?
void RemoveMarker()
{
}

//Makes new branch (timeline), takes current frame and creates new input log of all frames before it, new input log will be in focus
void Branch()
{
}


//The subclass wndproc for the listview header
static LRESULT APIENTRY HeaderWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
	case WM_LBUTTONDOWN:
		{
			//perform hit test
			HD_HITTESTINFO info;
			info.pt.x = GET_X_LPARAM(lParam);
			info.pt.y = GET_Y_LPARAM(lParam);
			SendMessage(hWnd,HDM_HITTEST,0,(LPARAM)&info);
			if(info.iItem != -1)
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
	HIMAGELIST himglist = ImageList_Create(12,12,ILC_COLOR32 | ILC_MASK,1,1);
	HBITMAP bmp = LoadBitmap(fceu_hInstance,MAKEINTRESOURCE(IDB_TE_ARROW));
	ImageList_AddMasked(himglist, bmp, RGB(255,0,255));
	DeleteObject(bmp);
	ListView_SetImageList(hwndList,himglist,LVSIL_SMALL);
	//doesnt work well??
	//HIMAGELIST himglist = ImageList_LoadImage(fceu_hInstance,MAKEINTRESOURCE(IDB_TE_ARROW),12,1,RGB(255,0,255),IMAGE_BITMAP,LR_DEFAULTCOLOR);

	//setup columns
	LVCOLUMN lvc;
	int colidx=0;
	lvc.mask = LVCF_WIDTH;
	lvc.cx = 12;
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	lvc.mask = LVCF_WIDTH | LVCF_TEXT;
	lvc.cx = 95;
	lvc.pszText = "Frame#";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	lvc.cx = 20;
	lvc.pszText = "A";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	lvc.pszText = "B";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	lvc.pszText = "S";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	lvc.pszText = "T";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	lvc.pszText = "U";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	lvc.pszText = "D";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	lvc.pszText = "L";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	lvc.pszText = "R";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	lvc.pszText = "A";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	lvc.pszText = "B";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	lvc.pszText = "S";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	lvc.pszText = "T";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	lvc.pszText = "U";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	lvc.pszText = "D";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	lvc.pszText = "L";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	lvc.pszText = "R";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	//-----------------------------

	//the initial update
	UpdateTasEdit();
}

bool CheckSaveChanges()
{
	//TODO: determine if project has changed, and ask to save changes
	return true; 
}

void KillTasEdit()
{
	if (!CheckSaveChanges())
		return;
	DestroyWindow(hwndTasEdit);
	hwndTasEdit = 0;
	turbo=false;
	FCEUMOV_ExitTasEdit();
}

//Creates a new TASEdit Project
static void NewProject()
{
//determine if current project changed
//if so, ask to save changes
//close current project

	if (!CheckSaveChanges())
		return;

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

	if(GetOpenFileName(&ofn)){							//If it is a valid filename
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
		project.SetProjectName(GetRomName());			//Set the project's name to the ROM name
		std::string thisfm2name = project.GetProjectName();
		thisfm2name.append(".fm2");						//Setup the fm2 name
		project.SetFM2Name(thisfm2name);				//Set the project's fm2 name
		project.LoadProject(project.GetProjectFile());
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

	currMovieData.truncateAt(frame+1);
	InvalidateGreenZone(frame);
	currMovieData.TryDumpIncremental();
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
				char temp[128];
				sprintf(temp,"TweakCount: %d\n",currMovieData.tweakCount);
				SetWindowText(GetDlgItem(hwndDlg,IDC_TWEAKCOUNT),temp);
			}
			break;
		case WM_INITDIALOG:
			if (TasEdit_wndx==-32000) TasEdit_wndx=0; //Just in case
			if (TasEdit_wndy==-32000) TasEdit_wndy=0;
			SetWindowPos(hwndDlg,0,TasEdit_wndx,TasEdit_wndy,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER);

			hwndList = GetDlgItem(hwndDlg,IDC_LIST1);
			InitDialog();
			break; 

		case WM_MOVE: {
				if (!IsIconic(hwndDlg)) {
				RECT wrect;
				GetWindowRect(hwndDlg,&wrect);
				TasEdit_wndx = wrect.left;
				TasEdit_wndy = wrect.top;

				#ifdef WIN32
				WindowBoundsCheckNoResize(TasEdit_wndx,TasEdit_wndy,wrect.right);
				#endif
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
			KillTasEdit();
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			case ACCEL_CTRL_N:
			case ID_FILE_NEWPROJECT:
				NewProject();
				break;

			case ACCEL_CTRL_O:
			case ID_FILE_OPENPROJECT:
				OpenProject();
				break;
			
			case ACCEL_CTRL_S:
			case ID_FILE_SAVEPROJECT:
				SaveProject();
				break;

			case ACCEL_CTRL_SHIFT_S:
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
				KillTasEdit();
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

			case ACCEL_CTRL_SHIFT_V:  //Takes selected frames and creates new inputlog files
			case ID_TASEDIT_PASTETONEW:
			case ID_CONTEXT_SELECTED_PASTETONEW:
				PastetoNew();
				break;

			case ACCEL_CTRL_DELETE:
			case ID_TASEDIT_DELETE:
				Delete();
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
			case IDC_HACKY1:
				Truncate();
				break;

			case IDC_HACKY2:
				//hacky2: delete earlier savestates (conserve memory)
				LockGreenZone(currFrameCounter);
				UpdateTasEdit();
				break;

			case ACCEL_CTRL_B:
			case ID_EDIT_BRANCH:
			case ID_CONTEXT_SELECTED_BRANCH:
				Branch();
				break;

			case ID_HELP_TASEDITHELP:
				OpenHelpWindow(tasedithelp);
				//link to TASEdit in help menu
				break;

			case MENU_CONTEXT_STRAY_INSERTFRAMES:
				{
					int frames;
					if(CWin32InputBox::GetInteger("Insert Frames", "How many frames?", frames, hwndDlg) == IDOK)
					{
						currMovieData.insertEmpty(currFrameCounter,frames);
						InvalidateGreenZone(currFrameCounter);
						RedrawList();
					}
				}
				break;

			case ID_CONTEXT_SELECTED_INSERTFRAMES:
				InsertFrames();
				break;

			case ID_CONTEXT_SELECTED_DELETEFRAMES:
				DeleteFrames();
				break;
			
			case TASEDIT_FOWARD:
				//advance 1 frame
				JumpToFrame(currFrameCounter+1);
				break;

			case TASEDIT_REWIND:
				//rewinds 1 frame
				if (currFrameCounter>0)
					JumpToFrame(currFrameCounter-1);
				break;
			
			case TASEDIT_PLAYSTOP:
				//Pause/Unpses (Play/Stop) movie
				FCEUI_ToggleEmulationPause();
				break;

			case TASEDIT_REWIND_FULL:
				//rewinds to beginning of movie
				JumpToFrame(0);
				break;
			case TASEDIT_FOWARD_FULL:
				//moves to the end of the move (or green zone?)
				JumpToFrame(currMovieData.records.size()-1 );
				break;

			}
			break;
	}

	return FALSE;
}

void DoTasEdit()
{
	if(!FCEU_IsValidUI(FCEUI_TASEDIT))
		return;

	if(!hmenu)
	{
		hmenu = LoadMenu(fceu_hInstance,"TASEDITMENU");
		hrmenu = LoadMenu(fceu_hInstance,"TASEDITCONTEXTMENUS");
	}


	lastCursor = -1;
	if(!hwndTasEdit) 
		hwndTasEdit = CreateDialog(fceu_hInstance,"TASEDIT",NULL,WndprocTasEdit);

	if(hwndTasEdit)
	{
		KeyboardSetBackgroundAccessBit(KEYBACKACCESS_TASEDIT);
		JoystickSetBackgroundAccessBit(JOYBACKACCESS_TASEDIT);
		FCEUMOV_EnterTasEdit();
		SetWindowPos(hwndTasEdit,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
	}
}
