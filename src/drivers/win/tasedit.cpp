#include <set>
#include <fstream>

#include "common.h"
#include "tasedit.h"
#include "fceu.h"
#include "debugger.h"
#include "replay.h"
#include "movie.h"
#include "utils/xstring.h"
#include "Win32InputBox.h"


using namespace std;

//to change header font
//http://forums.devx.com/archive/index.php/t-37234.html

HWND hwndTasEdit = 0;

static HMENU hmenu, hrmenu;
static int lastCursor;
static HWND hwndList, hwndHeader;
static WNDPROC hwndHeader_oldWndproc;

typedef std::set<int> TSelectionFrames;
static TSelectionFrames selectionFrames;

//hacky.. we need to think about how to convey information from the driver to the movie code.
//add a new fceud_ function?? blehhh maybe
extern EMOVIEMODE movieMode;

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
		case 9: {
				int bit = (item.iSubItem - 2);
				uint8 data = currMovieData.records[item.iItem].joysticks[0];
				if(data & (1<<bit))
				{
					item.pszText[0] = MovieRecord::mnemonics[bit];
					item.pszText[1] = 0;
				} else 
					item.pszText[1] = 0;
			}
			break;
		}
	}
}

#define CDDS_ITEMPREPAINT       (CDDS_ITEM | CDDS_PREPAINT)
#define CDDS_ITEMPOSTPAINT      (CDDS_ITEM | CDDS_POSTPAINT)
#define CDDS_ITEMPREERASE       (CDDS_ITEM | CDDS_PREERASE)
#define CDDS_ITEMPOSTERASE      (CDDS_ITEM | CDDS_POSTERASE)
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
		if((int)msg->nmcd.dwItemSpec < currMovieData.greenZoneCount)
			msg->clrTextBk = RGB(192,255,192);
		return CDRF_DODEFAULT;
	default:
		return CDRF_DODEFAULT;
	}
}

// called from the rest of the emulator when things happen and the tasedit should change to reflect it
void UpdateTasEdit()
{
	if(!hwndTasEdit) return;

	//update the number of items
	ListView_SetItemCountEx(hwndList,currMovieData.getNumRecords(),LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);

	//update the cursor
	int newCursor = currFrameCounter;
	if(newCursor != lastCursor)
	{
		//scroll to the row
		ListView_EnsureVisible(hwndList,newCursor,FALSE);
		//select the row
		ListView_SetItemState(hwndList,lastCursor,0, LVIS_FOCUSED|LVIS_SELECTED);
		ListView_SetItemState(hwndList,newCursor,LVIS_FOCUSED|LVIS_SELECTED,LVIS_FOCUSED|LVIS_SELECTED);
		
		//update the old and new rows
		ListView_Update(hwndList,lastCursor);
		ListView_Update(hwndList,newCursor);
		
		lastCursor = newCursor;
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

void InvalidateGreenZone(int after)
{
	currMovieData.greenZoneCount = std::min(after+1,currMovieData.greenZoneCount);
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
		//if the row is in the green zone, then move to it
		if(index < currMovieData.greenZoneCount)
		{
			MovieData::loadSavestateFrom(&currMovieData.records[index].savestate);
			currFrameCounter = index;	
		}
	}
	else //if an input column was clicked:
	{
		//toggle the bit
		int bit = (info->iSubItem-2);
		currMovieData.records[index].toggleBit(0,bit);
		
		//update the row
		ListView_Update(hwndList,index);

		InvalidateGreenZone(index);

		//redraw everything to show the reduced green zone
		RedrawList();
	}
}

//insert frames at the currently selected positions.
static void InsertFrames()
{
	int frames = selectionFrames.size();

	//this is going to be slow.

	//to keep this from being even slower than it would otherwise be, go ahead and reserve records
	currMovieData.records.reserve(currMovieData.records.size()+frames);

	//insert frames before each selection
	int ctr=0;
	for(TSelectionFrames::iterator it(selectionFrames.begin()); it != selectionFrames.end(); it++)
	{
		currMovieData.insertEmpty(*it+ctr,1);
		ctr++;
	}

	InvalidateGreenZone(*selectionFrames.begin());

	RedrawList();
}

//the column set operation, for setting a button for a span of selected values
static void ColumnSet(int column)
{
	int button = column-2;

	//inspect the selected frames. count the set and unset rows
	int set=0, unset=0;
	for(TSelectionFrames::iterator it(selectionFrames.begin()); it != selectionFrames.end(); it++)
	{
		if(currMovieData.records[*it].checkBit(0,button))
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
		currMovieData.records[*it].setBitValue(0,button,newValue);
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

//copies the current selection to the clipboard
static void Copy()
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
			HDHITTESTINFO info;
			info.pt.x = GET_X_LPARAM(lParam);
			info.pt.y = GET_Y_LPARAM(lParam);
			SendMessage(hWnd,HDM_HITTEST,0,(LPARAM)&info);
			if(info.iItem != -1)
				ColumnSet(info.iItem);
		}
	}
	return CallWindowProc(hwndHeader_oldWndproc,hWnd,msg,wParam,lParam);
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
	//-----------------------------

	//the initial update
	UpdateTasEdit();
}


void KillTasEdit()
{
	DestroyWindow(hwndTasEdit);
	hwndTasEdit = 0;
	FCEUMOV_ExitTasEdit();
}

static void Export()
{
	const char filter[]="FCEUX Movie File(*.fm2)\0*.fm2\0";
	char fname[2048] = {0};
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Export TAS as...";
	ofn.lpstrFilter=filter;
	ofn.lpstrFile=fname;
	ofn.nMaxFile=256;
	std::string initdir = FCEU_GetPath(FCEUMKF_MOVIE);
	ofn.lpstrInitialDir=initdir.c_str();
	if(GetSaveFileName(&ofn))
	{
		fstream* osRecordingMovie = FCEUD_UTF8_fstream(ofn.lpstrFile, "wb");
		currMovieData.dump(osRecordingMovie,false);
		delete osRecordingMovie;
		osRecordingMovie = 0;
	}
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
		case WM_INITDIALOG:
			hwndList = GetDlgItem(hwndDlg,IDC_LIST1);
			InitDialog();
			break; 

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
			case MENU_INSERTFRAMES:
				InsertFrames();
				break;
			case MENU_STRAY_INSERTFRAMES:
				{
					int frames;
					if(CWin32InputBox::GetInteger("Insert Frames", "How many frames?", frames, hwndDlg) == IDOK)
					{
						currMovieData.insertEmpty(-1,frames);
						RedrawList();
					}
				}
				break;
			case IDC_HACKY1:
				//hacky1: delete all items after the current selection
				currMovieData.records.resize(currFrameCounter+1);
				UpdateTasEdit();
				break;

			case ID_FILE_OPENFM2:
				Replay_LoadMovie(true);
				break;
			
			case ID_FILE_SAVEFM2:
				Export();
				break;

			case ACCEL_CTRL_C:
				Copy();
				break;

			case ACCEL_CTRL_W:
				KillTasEdit();
				break;
			}
			break;
	}

	return FALSE;
}

void DoTasEdit()
{
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
		FCEUMOV_EnterTasEdit();
		SetWindowPos(hwndTasEdit,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
	}
}
