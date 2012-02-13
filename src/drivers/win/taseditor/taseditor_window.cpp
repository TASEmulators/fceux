//Implementation file of TASEDITOR_WINDOW class
#include "taseditor_project.h"
#include "../main.h"
#include "../Win32InputBox.h"
#include "../taseditor.h"
#include <htmlhelp.h>
#include "../../input.h"	// for EMUCMD

extern TASEDITOR_CONFIG taseditor_config;
extern PLAYBACK playback;
extern GREENZONE greenzone;
extern RECORDER recorder;
extern TASEDITOR_PROJECT project;
extern TASEDITOR_LIST list;
extern TASEDITOR_SELECTION selection;
extern SPLICER splicer;
extern MARKERS_MANAGER markers_manager;
extern BOOKMARKS bookmarks;
extern HISTORY history;
extern POPUP_DISPLAY popup_display;

extern bool turbo;
extern bool muteTurbo;
extern bool must_call_manual_lua_function;

extern char* GetKeyComboName(int c);

extern BOOL CALLBACK FindNoteProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK AboutProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK WndprocTasEditor(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Recent Menu
HMENU recent_projects_menu;
char* recent_projects[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const unsigned int MENU_FIRST_RECENT_PROJECT = 55000;
const unsigned int MAX_NUMBER_OF_RECENT_PROJECTS = sizeof(recent_projects)/sizeof(*recent_projects);

// resources
char windowCaptioBase[] = "TAS Editor";
char taseditor_help_filename[] = "\\taseditor.chm";
// all items of the window (used for resising) and their default x,y,w,h
// actual x,y,w,h are calculated at the beginning from screen
static struct 
{
	int id;
	int x;
	int y;
	int width;
	int height;
	char tooltip_text_base[TOOLTIP_TEXT_MAX_LEN];
	char tooltip_text[TOOLTIP_TEXT_MAX_LEN];
	bool static_rect;
	int hotkey_emucmd;
	HWND tooltip_hwnd;
} window_items[TASEDITOR_WINDOW_TOTAL_ITEMS] = {
	IDC_PROGRESS_BUTTON, -1, 0, 0, 0, "Click here whenever you want to abort seeking", "", false, 0, 0,
	IDC_BRANCHES_BUTTON, -1, 0, 0, 0, "Click here to switch between Bookmarks List and Branches Tree", "", false, 0, 0,
	IDC_LIST1, 0, 0, -1, -1, "", "", false, 0, 0,
	IDC_PLAYBACK_BOX, -1, 0, 0, 0, "", "", false, 0, 0,
	IDC_RECORDER_BOX, -1, 0, 0, 0, "", "", false, 0, 0,
	IDC_SPLICER_BOX, -1, 0, 0, 0, "", "", false, 0, 0,
	IDC_LUA_BOX, -1, 0, 0, 0, "", "", false, 0, 0,
	IDC_BOOKMARKS_BOX, -1, 0, 0, 0, "", "", false, 0, 0,
	IDC_HISTORY_BOX, -1, 0, 0, -1, "", "", false, 0, 0,
	TASEDITOR_REWIND_FULL, -1, 0, 0, 0, "Send Playback to previous Marker (hotkey: Shift+PageUp)", "", false, 0, 0,
	TASEDITOR_REWIND, -1, 0, 0, 0, "Rewind one frame", "", false, EMUCMD_TASEDITOR_REWIND, 0,
	TASEDITOR_PLAYSTOP, -1, 0, 0, 0, "Pause/Unpause Emulation", "", false, EMUCMD_PAUSE, 0,
	TASEDITOR_FORWARD, -1, 0, 0, 0, "Advance one frame", "", false, EMUCMD_FRAME_ADVANCE, 0,
	TASEDITOR_FORWARD_FULL, -1, 0, 0, 0, "Send Playback to next Marker (hotkey: Shift+PageDown)", "", false, 0, 0,
	IDC_PROGRESS1, -1, 0, 0, 0, "", "", false, 0, 0,
	CHECK_FOLLOW_CURSOR, -1, 0, 0, 0, "The List will follow Playback cursor movements", "", false, 0, 0,
	CHECK_AUTORESTORE_PLAYBACK, -1, 0, 0, 0, "If you change input above Playback, cursor will run where it was before change", "", false, 0, 0,
	IDC_BOOKMARKSLIST, -1, 0, 0, 0, "Right click = set Bookmark, Left click = jump to Bookmark or load Branch", "", false, 0, 0,
	IDC_HISTORYLIST, -1, 0, 0, -1, "Click to revert movie back to that time", "", false, 0, 0,
	IDC_RADIO_ALL, -1, 0, 0, 0, "", "", false, 0, 0,
	IDC_RADIO_1P, -1, 0, 0, 0, "", "", false, 0, 0,
	IDC_RADIO_2P, -1, 0, 0, 0, "", "", false, 0, 0,
	IDC_RADIO_3P, -1, 0, 0, 0, "", "", false, 0, 0,
	IDC_RADIO_4P, -1, 0, 0, 0, "", "", false, 0, 0,
	IDC_SUPERIMPOSE, -1, 0, 0, 0, "Allows to superimpose old input with new buttons, instead of overwriting", "", false, 0, 0,
	TASEDITOR_PREV_MARKER, -1, -1, 0, -1, "Send Selection to previous Marker (hotkey: Ctrl+PageUp)", "", false, 0, 0,
	TASEDITOR_FIND_BEST_SIMILAR_MARKER, -1, -1, 0, -1, "Auto-search for Marker Note", "", false, 0, 0,
	TASEDITOR_FIND_NEXT_SIMILAR_MARKER, -1, -1, 0, -1, "Continue Auto-search", "", false, 0, 0,
	TASEDITOR_NEXT_MARKER, -1, -1, 0, -1, "Send Selection to next Marker (hotkey: Ctrl+PageDown)", "", false, 0, 0,
	IDC_JUMP_PLAYBACK_BUTTON, 0, 0, 0, 0, "Click here to scroll the List to Playback cursor", "", false, 0, 0,
	IDC_PLAYBACK_MARKER_EDIT, 0, 0, -1, 0, "Click to edit text", "", false, 0, 0,
	IDC_PLAYBACK_MARKER, 0, 0, 0, 0, "", "", false, 0, 0,
	IDC_JUMP_SELECTION_BUTTON, 0, -1, 0, -1, "Click here to scroll the List to Selection", "", false, 0, 0,
	IDC_SELECTION_MARKER_EDIT, 0, -1, -1, -1, "Click to edit text", "", false, 0, 0,
	IDC_SELECTION_MARKER, 0, -1, 0, -1, "", "", false, 0, 0,
	IDC_BRANCHES_BITMAP, -1, 0, 0, 0, "This window visualizes the hierarchy of your Branches", "", false, 0, 0,
	CHECK_TURBO_SEEK, -1, 0, 0, 0, "Uncheck when you need to watch seeking in slow motion", "", false, 0, 0,
	IDC_TEXT_SELECTION, -1, 0, 0, 0, "Current size of Selection", "", true, 0, 0,
	IDC_TEXT_CLIPBOARD, -1, 0, 0, 0, "Current size of Input in the Clipboard", "", true, 0, 0,
	IDC_RECORDING, -1, 0, 0, 0, "Switch Input Recording on/off", "", false, EMUCMD_MOVIE_READONLY_TOGGLE, 0,
	TASEDITOR_RUN_MANUAL, -1, 0, 0, 0, "Press the button to execute Lua Manual Function", "", false, 0, 0,
	IDC_RUN_AUTO, -1, 0, 0, 0, "Enable Lua Auto Function (but first it must be registered by Lua script)", "", false, 0, 0,
};

TASEDITOR_WINDOW::TASEDITOR_WINDOW()
{
	hwndTasEditor = 0;
	hwndFindNote = 0;
	hTaseditorIcon = 0;
	TASEditor_focus = false;
	ready_for_resizing = false;
	min_width = 0;
	min_height = 0;

}

void TASEDITOR_WINDOW::init()
{
	ready_for_resizing = false;
	hTaseditorIcon = (HICON)LoadImage(fceu_hInstance, MAKEINTRESOURCE(IDI_ICON3), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);
	hwndTasEditor = CreateDialog(fceu_hInstance, "TASEDITOR", hAppWnd, WndprocTasEditor);
	SendMessage(hwndTasEditor, WM_SETICON, ICON_SMALL, (LPARAM)hTaseditorIcon);
	CalculateItems();
	// restore position and size from config, also bring the window on top
	SetWindowPos(hwndTasEditor, HWND_TOP, taseditor_config.wndx, taseditor_config.wndy, taseditor_config.wndwidth, taseditor_config.wndheight, SWP_NOOWNERZORDER);
	// menus and checked items
	hmenu = GetMenu(hwndTasEditor);
	hrmenu = LoadMenu(fceu_hInstance,"TASEDITORCONTEXTMENUS");
	UpdateCheckedItems();
	// tooltips
	int x = 0;
	for (int i = 0; i < TASEDITOR_WINDOW_TOTAL_ITEMS; ++i)
	{
		if (window_items[i].tooltip_text_base[0])
		{
			// Create the tooltip. g_hInst is the global instance handle
			window_items[i].tooltip_hwnd = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
									  WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
									  CW_USEDEFAULT, CW_USEDEFAULT,
									  CW_USEDEFAULT, CW_USEDEFAULT,
									  hwndTasEditor, NULL, 
									  fceu_hInstance, NULL);
			if (window_items[i].tooltip_hwnd)
			{
				// Associate the tooltip with the tool
				TOOLINFO toolInfo = {0};
				toolInfo.cbSize = sizeof(toolInfo);
				toolInfo.hwnd = hwndTasEditor;
				toolInfo.uId = (UINT_PTR)GetDlgItem(hwndTasEditor, window_items[i].id);
				if (window_items[i].static_rect)
				{
					// for static text we specify rectangle
					toolInfo.uFlags = TTF_SUBCLASS;
					RECT tool_rect;
					GetWindowRect(GetDlgItem(hwndTasEditor, window_items[i].id), &tool_rect);
					POINT pt;
					pt.x = tool_rect.left;
					pt.y = tool_rect.top;
					ScreenToClient(hwndTasEditor, &pt);
					toolInfo.rect.left = pt.x;
					toolInfo.rect.right = toolInfo.rect.left + (tool_rect.right - tool_rect.left);
					toolInfo.rect.top = pt.y;
					toolInfo.rect.bottom = toolInfo.rect.top + (tool_rect.bottom - tool_rect.top);
				} else
				{
					// for other controls we provide hwnd
					toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
				}
				// add hotkey mapping if needed
				if (window_items[i].hotkey_emucmd && FCEUD_CommandMapping[window_items[i].hotkey_emucmd])
				{
					window_items[i].tooltip_text[0] = 0;
					strcpy(window_items[i].tooltip_text, window_items[i].tooltip_text_base);
					strcat(window_items[i].tooltip_text, " (hotkey: ");
					strncat(window_items[i].tooltip_text, GetKeyComboName(FCEUD_CommandMapping[window_items[i].hotkey_emucmd]), TOOLTIP_TEXT_MAX_LEN - strlen(window_items[i].tooltip_text) - 1);
					strncat(window_items[i].tooltip_text, ")", TOOLTIP_TEXT_MAX_LEN - strlen(window_items[i].tooltip_text) - 1);
					toolInfo.lpszText = window_items[i].tooltip_text;
				} else
				{
					toolInfo.lpszText = window_items[i].tooltip_text_base;
				}
				SendMessage(window_items[i].tooltip_hwnd, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
				SendMessage(window_items[i].tooltip_hwnd, TTM_SETDELAYTIME, TTDT_AUTOPOP, TOOLTIPS_AUTOPOP_TIMEOUT);
			}
		}
	}
	UpdateTooltips();
	// recent projects submenu
	recent_projects_menu = CreateMenu();
	UpdateRecentProjectsMenu();

	reset();
}
void TASEDITOR_WINDOW::exit()
{
	for (int i = 0; i < TASEDITOR_WINDOW_TOTAL_ITEMS; ++i)
	{
		if (window_items[i].tooltip_hwnd)
		{
			DestroyWindow(window_items[i].tooltip_hwnd);
			window_items[i].tooltip_hwnd = 0;
		}
	}
	if (hwndFindNote)
	{
		DestroyWindow(hwndFindNote);
		hwndFindNote = 0;
	}
	if (hwndTasEditor)
	{
		DestroyWindow(hwndTasEditor);
		hwndTasEditor = 0;
		TASEditor_focus = false;
	}
	if (hTaseditorIcon)
	{
		DestroyIcon(hTaseditorIcon);
		hTaseditorIcon = 0;
	}
}
void TASEDITOR_WINDOW::reset()
{

}
void TASEDITOR_WINDOW::update()
{

}
// --------------------------------------------------------------------------------
void TASEDITOR_WINDOW::CalculateItems()
{
	RECT r, main_r;
	POINT p;
	HWND hCtrl;

	// set min size to current size
	GetWindowRect(hwndTasEditor, &main_r);
	min_width = main_r.right - main_r.left;
	min_height = main_r.bottom - main_r.top;
	// check if wndwidth and wndheight weren't initialized
	if (taseditor_config.wndwidth < min_width)
		taseditor_config.wndwidth = min_width;
	if (taseditor_config.wndheight < min_height)
		taseditor_config.wndheight = min_height;
	// find current client area of Taseditor window
	int main_width = main_r.right - main_r.left;
	int main_height = main_r.bottom - main_r.top;

	// calculate current positions for all items
	for (int i = 0; i < TASEDITOR_WINDOW_TOTAL_ITEMS; ++i)
	{
		hCtrl = GetDlgItem(hwndTasEditor, window_items[i].id);

		GetWindowRect(hCtrl, &r);
		p.x = r.left;
		p.y = r.top;
		ScreenToClient(hwndTasEditor, &p);
		if (window_items[i].x < 0)
			// right-aligned
			window_items[i].x = -(main_width - p.x);
		else
			// left-aligned
			window_items[i].x = p.x;
		if (window_items[i].y < 0)
			// bottom-aligned
			window_items[i].y = -(main_height - p.y);
		else
			// top-aligned
			window_items[i].y = p.y;
		if (window_items[i].width < 0)
			// width is right-aligned (may be dynamic width)
			window_items[i].width = -(main_width - (p.x + (r.right - r.left)));
		else
			// normal width
			window_items[i].width = r.right - r.left;
		if (window_items[i].height < 0)
			// height is bottom-aligned (may be dynamic height)
			window_items[i].height = -(main_height - (p.y + (r.bottom - r.top)));
		else
			// normal height
			window_items[i].height = r.bottom - r.top;
	}
	ready_for_resizing = true;
}
void TASEDITOR_WINDOW::ResizeItems()
{
	HWND hCtrl;
	int x, y, width, height;
	for (int i = 0; i < TASEDITOR_WINDOW_TOTAL_ITEMS; ++i)
	{
		hCtrl = GetDlgItem(hwndTasEditor, window_items[i].id);
		if (window_items[i].x < 0)
			// right-aligned
			x = taseditor_config.wndwidth + window_items[i].x;
		else
			// left-aligned
			x = window_items[i].x;
		if (window_items[i].y < 0)
			// bottom-aligned
			y = taseditor_config.wndheight + window_items[i].y;
		else
			// top-aligned
			y = window_items[i].y;
		if (window_items[i].width < 0)
			// width is right-aligned (may be dynamic width)
			width = (taseditor_config.wndwidth + window_items[i].width) - x;
		else
			// normal width
			width = window_items[i].width;
		if (window_items[i].height < 0)
			// height is bottom-aligned (may be dynamic height)
			height = (taseditor_config.wndheight + window_items[i].height) - y;
		else
			// normal height
			height = window_items[i].height;
		SetWindowPos(hCtrl, 0, x, y, width, height, SWP_NOZORDER | SWP_NOOWNERZORDER);
	}
	RedrawTaseditor();
}
void TASEDITOR_WINDOW::WindowMovedOrResized()
{
	RECT wrect;
	GetWindowRect(hwndTasEditor, &wrect);
	taseditor_config.wndx = wrect.left;
	taseditor_config.wndy = wrect.top;
	WindowBoundsCheckNoResize(taseditor_config.wndx, taseditor_config.wndy, wrect.right);
	taseditor_config.wndwidth = wrect.right - wrect.left;
	if (taseditor_config.wndwidth < min_width)
		taseditor_config.wndwidth = min_width;
	taseditor_config.wndheight = wrect.bottom - wrect.top;
	if (taseditor_config.wndheight < min_height)
		taseditor_config.wndheight = min_height;
}

void TASEDITOR_WINDOW::UpdateTooltips()
{
	if (taseditor_config.tooltips)
	{
		for (int i = 0; i < TASEDITOR_WINDOW_TOTAL_ITEMS; ++i)
		{
			if (window_items[i].tooltip_hwnd)
				SendMessage(window_items[i].tooltip_hwnd, TTM_ACTIVATE, true, 0);
		}
	} else
	{
		for (int i = 0; i < TASEDITOR_WINDOW_TOTAL_ITEMS; ++i)
		{
			if (window_items[i].tooltip_hwnd)
				SendMessage(window_items[i].tooltip_hwnd, TTM_ACTIVATE, false, 0);
		}
	}
}

void TASEDITOR_WINDOW::UpdateCaption()
{
	char new_caption[300];
	strcpy(new_caption, windowCaptioBase);
	if (!movie_readonly)
		strcat(new_caption, recorder.GetRecordingCaption());
	// add project name
	std::string projectname = project.GetProjectName();
	if (!projectname.empty())
	{
		strcat(new_caption, " - ");
		strcat(new_caption, projectname.c_str());
	}
	// and * if project has unsaved changes
	if (project.GetProjectChanged())
		strcat(new_caption, "*");
	SetWindowText(hwndTasEditor, new_caption);
}
void TASEDITOR_WINDOW::RedrawTaseditor()
{
	InvalidateRect(hwndTasEditor, 0, FALSE);
}

void TASEDITOR_WINDOW::RightClick(LPNMITEMACTIVATE info)
{
	int index = info->iItem;
	if(index == -1)
		StrayClickMenu(info);
	else if (selection.CheckFrameSelected(index))
		RightClickMenu(info);
}
void TASEDITOR_WINDOW::StrayClickMenu(LPNMITEMACTIVATE info)
{
	POINT pt = info->ptAction;
	ClientToScreen(list.hwndList, &pt);
	HMENU sub = GetSubMenu(hrmenu, CONTEXTMENU_STRAY);
	TrackPopupMenu(sub, 0, pt.x, pt.y, 0, hwndTasEditor, 0);
}
void TASEDITOR_WINDOW::RightClickMenu(LPNMITEMACTIVATE info)
{
	POINT pt = info->ptAction;
	ClientToScreen(list.hwndList, &pt);

	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0)
	{
		StrayClickMenu(info);
		return;
	}
	HMENU sub = GetSubMenu(hrmenu, CONTEXTMENU_SELECTED);
	// inspect current selection and disable inappropriate menu items
	SelectionFrames::iterator current_selection_begin(current_selection->begin());
	SelectionFrames::iterator current_selection_end(current_selection->end());
	bool set_found = false, unset_found = false;
	for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
	{
		if(markers_manager.GetMarker(*it))
			set_found = true;
		else 
			unset_found = true;
	}
	if (set_found)
		EnableMenuItem(sub, ID_SELECTED_REMOVEMARKER, MF_BYCOMMAND | MF_ENABLED);
	else
		EnableMenuItem(sub, ID_SELECTED_REMOVEMARKER, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	if (unset_found)
		EnableMenuItem(sub, ID_SELECTED_SETMARKER, MF_BYCOMMAND | MF_ENABLED);
	else
		EnableMenuItem(sub, ID_SELECTED_SETMARKER, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

	TrackPopupMenu(sub, 0, pt.x, pt.y, 0, hwndTasEditor, 0);
}

void TASEDITOR_WINDOW::UpdateCheckedItems()
{
	// check option ticks
	CheckDlgButton(hwndTasEditor, CHECK_FOLLOW_CURSOR, taseditor_config.follow_playback?MF_CHECKED : MF_UNCHECKED);
	CheckDlgButton(hwndTasEditor,CHECK_AUTORESTORE_PLAYBACK,taseditor_config.restore_position?BST_CHECKED:BST_UNCHECKED);
	if (taseditor_config.superimpose == SUPERIMPOSE_UNCHECKED)
		CheckDlgButton(hwndTasEditor, IDC_SUPERIMPOSE, BST_UNCHECKED);
	else if (taseditor_config.superimpose == SUPERIMPOSE_CHECKED)
		CheckDlgButton(hwndTasEditor, IDC_SUPERIMPOSE, BST_CHECKED);
	else
		CheckDlgButton(hwndTasEditor, IDC_SUPERIMPOSE, BST_INDETERMINATE);
	CheckDlgButton(hwndTasEditor, IDC_RUN_AUTO, taseditor_config.enable_auto_function);
	CheckDlgButton(hwndTasEditor, CHECK_TURBO_SEEK, taseditor_config.turbo_seek?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_VIEW_SHOW_LAG_FRAMES, taseditor_config.show_lag_frames?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_VIEW_SHOW_MARKERS, taseditor_config.show_markers?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_VIEW_SHOWBRANCHSCREENSHOTS, taseditor_config.show_branch_screenshots?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_VIEW_SHOWBRANCHTOOLTIPS, taseditor_config.show_branch_descr?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_VIEW_JUMPWHENMAKINGUNDO, taseditor_config.jump_to_undo?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_VIEW_FOLLOWMARKERNOTECONTEXT, taseditor_config.follow_note_context?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_VIEW_ENABLEHOTCHANGES, taseditor_config.enable_hot_changes?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_CONFIG_BRANCHESRESTOREFULLMOVIE, taseditor_config.branch_full_movie?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_CONFIG_BRANCHESWORKONLYWHENRECORDING, taseditor_config.branch_only_when_rec?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_CONFIG_HUDINBRANCHSCREENSHOTS, taseditor_config.branch_scr_hud?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_CONFIG_BINDMARKERSTOINPUT, taseditor_config.bind_markers?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_CONFIG_EMPTYNEWMARKERNOTES, taseditor_config.empty_marker_notes?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_CONFIG_COMBINECONSECUTIVERECORDINGS, taseditor_config.combine_consecutive_rec?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_CONFIG_USE1PFORRECORDING, taseditor_config.use_1p_rec?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_CONFIG_USEINPUTKEYSFORCOLUMNSET, taseditor_config.columnset_by_keys?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_CONFIG_KEYBOARDCONTROLSINLISTVIEW, taseditor_config.keyboard_for_listview?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_CONFIG_SUPERIMPOSE_AFFECTS_PASTE, taseditor_config.superimpose_affects_paste?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_CONFIG_MUTETURBO, muteTurbo?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_CONFIG_SILENTAUTOSAVE, taseditor_config.silent_autosave?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_HELP_TOOLTIPS, taseditor_config.tooltips?MF_CHECKED : MF_UNCHECKED);

}



// --------------------------------------------------------------------------------------------
void TASEDITOR_WINDOW::UpdateRecentProjectsMenu()
{
	MENUITEMINFO moo;
	int x;
	moo.cbSize = sizeof(moo);
	moo.fMask = MIIM_SUBMENU | MIIM_STATE;
	GetMenuItemInfo(GetSubMenu(hmenu, 0), ID_FILE_RECENT, FALSE, &moo);
	moo.hSubMenu = recent_projects_menu;
	moo.fState = recent_projects[0] ? MFS_ENABLED : MFS_GRAYED;
	SetMenuItemInfo(GetSubMenu(hmenu, 0), ID_FILE_RECENT, FALSE, &moo);

	// Remove all recent files submenus
	for(x = 0; x < MAX_NUMBER_OF_RECENT_PROJECTS; x++)
	{
		RemoveMenu(recent_projects_menu, MENU_FIRST_RECENT_PROJECT + x, MF_BYCOMMAND);
	}
	// Recreate the menus
	for(x = MAX_NUMBER_OF_RECENT_PROJECTS - 1; x >= 0; x--)
	{  
		// Skip empty strings
		if(!recent_projects[x]) continue;

		std::string tmp = recent_projects[x];
		// clamp this string to 128 chars
		if(tmp.size() > 128)
			tmp = tmp.substr(0, 128);

		moo.cbSize = sizeof(moo);
		moo.fMask = MIIM_DATA | MIIM_ID | MIIM_TYPE;
		// Insert the menu item
		moo.cch = tmp.size();
		moo.fType = 0;
		moo.wID = MENU_FIRST_RECENT_PROJECT + x;
		moo.dwTypeData = (LPSTR)tmp.c_str();
		InsertMenuItem(recent_projects_menu, 0, 1, &moo);
	}

	// if recent_projects is empty, "Recent" manu should be grayed
	int i;
	for (i = 0; i < MAX_NUMBER_OF_RECENT_PROJECTS; ++i)
		if (recent_projects[i]) break;
	if (i < MAX_NUMBER_OF_RECENT_PROJECTS)
		EnableMenuItem(hmenu, ID_FILE_RECENT, MF_ENABLED);
	else
		EnableMenuItem(hmenu, ID_FILE_RECENT, MF_GRAYED);

	DrawMenuBar(hwndTasEditor);
}
void TASEDITOR_WINDOW::UpdateRecentProjectsArray(const char* addString)
{
	// find out if the filename is already in the recent files list
	for(unsigned int x = 0; x < MAX_NUMBER_OF_RECENT_PROJECTS; x++)
	{
		if(recent_projects[x])
		{
			if(!strcmp(recent_projects[x], addString))    // Item is already in list
			{
				// If the filename is in the file list don't add it again, move it up in the list instead
				char* tmp = recent_projects[x];			// save pointer
				for(int y = x; y; y--)
					// Move items down.
					recent_projects[y] = recent_projects[y - 1];
				// Put item on top.
				recent_projects[0] = tmp;
				UpdateRecentProjectsMenu();
				return;
			}
		}
	}
	// The filename wasn't found in the list. That means we need to add it.
	// If there's no space left in the recent files list, get rid of the last item in the list
	if(recent_projects[MAX_NUMBER_OF_RECENT_PROJECTS-1])
		free(recent_projects[MAX_NUMBER_OF_RECENT_PROJECTS-1]);
	// Move other items down
	for(unsigned int x = MAX_NUMBER_OF_RECENT_PROJECTS-1; x; x--)
		recent_projects[x] = recent_projects[x-1];
	// Add new item
	recent_projects[0] = (char*)malloc(strlen(addString) + 1);
	strcpy(recent_projects[0], addString);

	UpdateRecentProjectsMenu();
}
void TASEDITOR_WINDOW::RemoveRecentProject(unsigned int which)
{
	if (which >= MAX_NUMBER_OF_RECENT_PROJECTS) return;
	// Remove the item
	if(recent_projects[which])
		free(recent_projects[which]);
	// If the item is not the last one in the list, shift the remaining ones up
	if (which < MAX_NUMBER_OF_RECENT_PROJECTS-1)
	{
		// Move the remaining items up
		for(unsigned int x = which+1; x < MAX_NUMBER_OF_RECENT_PROJECTS; ++x)
		{
			recent_projects[x-1] = recent_projects[x];	// Shift each remaining item up by 1
		}
	}
	recent_projects[MAX_NUMBER_OF_RECENT_PROJECTS-1] = 0;	// Clear out the last item since it is empty now

	UpdateRecentProjectsMenu();
}
void TASEDITOR_WINDOW::LoadRecentProject(int slot)
{
	char*& fname = recent_projects[slot];
	if(fname && AskSaveProject())
	{
		if (!LoadProject(fname))
		{
			int result = MessageBox(hwndTasEditor, "Remove from list?", "Could Not Open Recent Project", MB_YESNO);
			if (result == IDYES)
				RemoveRecentProject(slot);
		}
	}
}


// ====================================================================================================
BOOL CALLBACK WndprocTasEditor(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	extern TASEDITOR_WINDOW taseditor_window;
	switch(uMsg)
	{
		case WM_PAINT:
			break;
		case WM_INITDIALOG:
		{
			if (taseditor_config.wndx == -32000) taseditor_config.wndx = 0;	//Just in case
			if (taseditor_config.wndy == -32000) taseditor_config.wndy = 0;
			break;
		}
		case WM_WINDOWPOSCHANGED:
		{
			WINDOWPOS* windowpos = (WINDOWPOS*)lParam;
			if (!(windowpos->flags & SWP_NOSIZE))
			{
				// window was resized
				if (!IsIconic(hwndDlg))
				{
					taseditor_window.WindowMovedOrResized();
					if (taseditor_window.ready_for_resizing)
						taseditor_window.ResizeItems();
					// also change coordinates of popup display (and move if it's open)
					popup_display.ParentWindowMoved();
				}
			} else if (!(windowpos->flags & SWP_NOMOVE))
			{
				// window was moved
				if (!IsIconic(hwndDlg) && !IsZoomed(hwndDlg))
					taseditor_window.WindowMovedOrResized();
				// also change coordinates of popup display (and move if it's open)
				popup_display.ParentWindowMoved();
			}
			break;
		}
		case WM_GETMINMAXINFO:
		{
			if (taseditor_window.ready_for_resizing)
			{
				((MINMAXINFO*)lParam)->ptMinTrackSize.x = taseditor_window.min_width;
				((MINMAXINFO*)lParam)->ptMinTrackSize.y = taseditor_window.min_height;
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
					SetWindowLong(hwndDlg, DWL_MSGRESULT, list.CustomDraw((NMLVCUSTOMDRAW*)lParam));
					return TRUE;
				case LVN_GETDISPINFO:
					list.GetDispInfo((NMLVDISPINFO*)lParam);
					break;
				case NM_CLICK:
					list.SingleClick((LPNMITEMACTIVATE)lParam);
					break;
				case NM_DBLCLK:
					list.DoubleClick((LPNMITEMACTIVATE)lParam);
					break;
				case NM_RCLICK:
					taseditor_window.RightClick((LPNMITEMACTIVATE)lParam);
					break;
				case LVN_ITEMCHANGED:
					selection.ItemChanged((LPNMLISTVIEW) lParam);
					break;
				case LVN_ODSTATECHANGED:
					selection.ItemRangeChanged((LPNMLVODSTATECHANGE) lParam);
					break;
					/*
				case LVN_ENDSCROLL:
					// redraw upper and lower list rows (fix for known WinXP bug)
					int start = ListView_GetTopIndex(hwndList);
					ListView_RedrawItems(hwndList,start,start);
					int end = start + listItems - 1;
					ListView_RedrawItems(hwndList,end,end);
					break;
					*/
				}
				break;
			case IDC_BOOKMARKSLIST:
				switch(((LPNMHDR)lParam)->code)
				{
				case NM_CUSTOMDRAW:
					SetWindowLong(hwndDlg, DWL_MSGRESULT, bookmarks.CustomDraw((NMLVCUSTOMDRAW*)lParam));
					return TRUE;
				case LVN_GETDISPINFO:
					bookmarks.GetDispInfo((NMLVDISPINFO*)lParam);
					break;
				case NM_CLICK:
				case NM_DBLCLK:
					bookmarks.LeftClick((LPNMITEMACTIVATE)lParam);
					break;
				case NM_RCLICK:
					bookmarks.RightClick((LPNMITEMACTIVATE)lParam);
					break;
				}
				break;
			case IDC_HISTORYLIST:
				switch(((LPNMHDR)lParam)->code)
				{
				case NM_CUSTOMDRAW:
					SetWindowLong(hwndDlg, DWL_MSGRESULT, history.CustomDraw((NMLVCUSTOMDRAW*)lParam));
					return TRUE;
				case LVN_GETDISPINFO:
					history.GetDispInfo((NMLVDISPINFO*)lParam);
					break;
				case NM_CLICK:
				case NM_DBLCLK:
				case NM_RCLICK:
					history.Click((LPNMITEMACTIVATE)lParam);
					break;
				}
				break;
			case TASEDITOR_PLAYSTOP:
				switch(((LPNMHDR)lParam)->code)
				{
				case NM_CLICK:
				case NM_DBLCLK:
					playback.ToggleEmulationPause();
					break;
				}
				break;
			}
			break;
		case WM_CLOSE:
		case WM_QUIT:
			ExitTasEditor();
			break;
		case WM_ACTIVATE:
			if(LOWORD(wParam))
			{
				taseditor_window.TASEditor_focus = true;
				SetTaseditorInput();
			} else
			{
				taseditor_window.TASEditor_focus = false;
				ClearTaseditorInput();
			}
			break;
		case WM_CTLCOLORSTATIC:
			if ((HWND)lParam == playback.hwndPlaybackMarker)
			{
				SetTextColor((HDC)wParam, PLAYBACK_MARKER_COLOR);
				SetBkMode((HDC)wParam, TRANSPARENT);
				return (BOOL)(list.bg_brush);
			} else if ((HWND)lParam == selection.hwndSelectionMarker)
			{
				SetTextColor((HDC)wParam, GetSysColor(COLOR_HIGHLIGHT));
				SetBkMode((HDC)wParam, TRANSPARENT);
				return (BOOL)list.bg_brush;
			}

			break;
		case WM_COMMAND:
			{
				unsigned int loword_wparam = LOWORD(wParam);
				// first check clicking Recent submenu item
				if (loword_wparam >= MENU_FIRST_RECENT_PROJECT && loword_wparam < MENU_FIRST_RECENT_PROJECT + MAX_NUMBER_OF_RECENT_PROJECTS)
				{
					taseditor_window.LoadRecentProject(loword_wparam - MENU_FIRST_RECENT_PROJECT);
					break;
				}
				// finally check all other commands
				switch(loword_wparam)
				{
				case IDC_PLAYBACK_MARKER_EDIT:
					{
						switch (HIWORD(wParam))
						{
						case EN_SETFOCUS:
							{
								markers_manager.marker_note_edit = MARKER_NOTE_EDIT_UPPER;
								// enable editing
								SendMessage(playback.hwndPlaybackMarkerEdit, EM_SETREADONLY, false, 0);
								// disable FCEUX keyboard
								ClearTaseditorInput();
								if (taseditor_config.follow_note_context)
									list.FollowMarker(playback.shown_marker);
								break;
							}
						case EN_KILLFOCUS:
							{
								if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_UPPER)
								{
									markers_manager.UpdateMarkerNote();
									markers_manager.marker_note_edit = MARKER_NOTE_EDIT_NONE;
								}
								// disable editing (make it grayed)
								SendMessage(playback.hwndPlaybackMarkerEdit, EM_SETREADONLY, true, 0);
								// enable FCEUX keyboard
								if (taseditor_window.TASEditor_focus)
									SetTaseditorInput();
								break;
							}
						}
						break;
					}
				case IDC_SELECTION_MARKER_EDIT:
					{
						switch (HIWORD(wParam))
						{
						case EN_SETFOCUS:
							{
								markers_manager.marker_note_edit = MARKER_NOTE_EDIT_LOWER;
								// enable editing
								SendMessage(selection.hwndSelectionMarkerEdit, EM_SETREADONLY, false, 0); 
								// disable FCEUX keyboard
								ClearTaseditorInput();
								if (taseditor_config.follow_note_context)
									list.FollowMarker(selection.shown_marker);
								break;
							}
						case EN_KILLFOCUS:
							{
								if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_LOWER)
								{
									markers_manager.UpdateMarkerNote();
									markers_manager.marker_note_edit = MARKER_NOTE_EDIT_NONE;
								}
								// disable editing (make it grayed)
								SendMessage(selection.hwndSelectionMarkerEdit, EM_SETREADONLY, true, 0); 
								// enable FCEUX keyboard
								if (taseditor_window.TASEditor_focus)
									SetTaseditorInput();
								break;
							}
						}
						break;
					}
				case ID_FILE_NEW:
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
				case ID_FILE_SAVECOMPACT:
					SaveCompact();
					break;
				case ID_FILE_IMPORT:
					Import();
					break;
				case ID_FILE_EXPORTFM2:
						Export();
					break;
				case ID_FILE_CLOSE:
					ExitTasEditor();
					break;
				case ID_EDIT_SELECTALL:
					if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_UPPER)
						SendMessage(playback.hwndPlaybackMarkerEdit, EM_SETSEL, 0, -1); 
					else if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_LOWER)
						SendMessage(selection.hwndSelectionMarkerEdit, EM_SETSEL, 0, -1); 
					else
						selection.SelectAll();
					break;
				case ACCEL_CTRL_X:
				case ID_EDIT_CUT:
					if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_UPPER)
						SendMessage(playback.hwndPlaybackMarkerEdit, WM_CUT, 0, 0); 
					else if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_LOWER)
						SendMessage(selection.hwndSelectionMarkerEdit, WM_CUT, 0, 0); 
					else
						splicer.Cut();
					break;
				case ACCEL_CTRL_C:
				case ID_EDIT_COPY:
					if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_UPPER)
						SendMessage(playback.hwndPlaybackMarkerEdit, WM_COPY, 0, 0); 
					else if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_LOWER)
						SendMessage(selection.hwndSelectionMarkerEdit, WM_COPY, 0, 0); 
					else
						splicer.Copy();
					break;
				case ACCEL_CTRL_V:
				case ID_EDIT_PASTE:
					if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_UPPER)
						SendMessage(playback.hwndPlaybackMarkerEdit, WM_PASTE, 0, 0); 
					else if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_LOWER)
						SendMessage(selection.hwndSelectionMarkerEdit, WM_PASTE, 0, 0); 
					else
						splicer.Paste();
					break;
				case ACCEL_SHIFT_V:
					{
						// hack to allow entering Shift-V into edit control even though accelerator steals the input message
						char insert_v[] = "v";
						char insert_V[] = "V";
						if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_UPPER)
						{
							if (GetKeyState(VK_CAPITAL) & 1)
								SendMessage(playback.hwndPlaybackMarkerEdit, EM_REPLACESEL, true, (LPARAM)insert_v);
							else
								SendMessage(playback.hwndPlaybackMarkerEdit, EM_REPLACESEL, true, (LPARAM)insert_V);
						} else if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_LOWER)
						{
							if (GetKeyState(VK_CAPITAL) & 1)
								SendMessage(selection.hwndSelectionMarkerEdit, EM_REPLACESEL, true, (LPARAM)insert_v);
							else
								SendMessage(selection.hwndSelectionMarkerEdit, EM_REPLACESEL, true, (LPARAM)insert_V);
						} else
							splicer.PasteInsert();
						break;
					}
				case ID_EDIT_PASTEINSERT:
					if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_UPPER)
						SendMessage(playback.hwndPlaybackMarkerEdit, WM_PASTE, 0, 0); 
					else if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_LOWER)
						SendMessage(selection.hwndSelectionMarkerEdit, WM_PASTE, 0, 0); 
					else
						splicer.PasteInsert();
					break;
				case ACCEL_CTRL_DELETE:
				case ID_EDIT_DELETE:
				case ID_CONTEXT_SELECTED_DELETEFRAMES:
					splicer.DeleteFrames();
					break;
				case ID_EDIT_TRUNCATE:
				case ID_CONTEXT_SELECTED_TRUNCATE:
				case ID_STRAY_TRUNCATE:
					splicer.Truncate();
					break;
				case ACCEL_INS:
				case ID_EDIT_INSERT:
				case MENU_CONTEXT_STRAY_INSERTFRAMES:
				case ID_CONTEXT_SELECTED_INSERTFRAMES2:
					splicer.InsertNumFrames();
					break;
				case ACCEL_SHIFT_INS:
				case ID_EDIT_INSERTFRAMES:
				case ID_CONTEXT_SELECTED_INSERTFRAMES:
					splicer.InsertFrames();
					break;
				case ACCEL_DEL:
				case ID_EDIT_CLEAR:
				case ID_CONTEXT_SELECTED_CLEARFRAMES:
					if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_UPPER)
					{
						DWORD sel_start, sel_end;
						SendMessage(playback.hwndPlaybackMarkerEdit, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
						if (sel_start == sel_end)
							SendMessage(playback.hwndPlaybackMarkerEdit, EM_SETSEL, sel_start, sel_start + 1);
						SendMessage(playback.hwndPlaybackMarkerEdit, WM_CLEAR, 0, 0); 
					} else if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_LOWER)
					{
						DWORD sel_start, sel_end;
						SendMessage(selection.hwndSelectionMarkerEdit, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
						if (sel_start == sel_end)
							SendMessage(selection.hwndSelectionMarkerEdit, EM_SETSEL, sel_start, sel_start + 1);
						SendMessage(selection.hwndSelectionMarkerEdit, WM_CLEAR, 0, 0); 
					} else
						splicer.ClearFrames();
					break;
				case TASEDITOR_PLAYSTOP:
					playback.ToggleEmulationPause();
					break;
				case CHECK_FOLLOW_CURSOR:
					//switch "Follow playback" flag
					taseditor_config.follow_playback ^= 1;
					taseditor_window.UpdateCheckedItems();
					// if switched off then maybe jump to target frame
					if (!taseditor_config.follow_playback && playback.pause_frame)
						list.FollowPauseframe();
					break;
				case CHECK_TURBO_SEEK:
					//switch "Turbo seek" flag
					taseditor_config.turbo_seek ^= 1;
					taseditor_window.UpdateCheckedItems();
					// if currently seeking, apply this option immediately
					if (playback.pause_frame)
						turbo = taseditor_config.turbo_seek;
					break;
				case ID_VIEW_SHOW_LAG_FRAMES:
					taseditor_config.show_lag_frames ^= 1;
					taseditor_window.UpdateCheckedItems();
					list.RedrawList();
					bookmarks.RedrawBookmarksList();
					break;
				case ID_VIEW_SHOW_MARKERS:
					taseditor_config.show_markers ^= 1;
					taseditor_window.UpdateCheckedItems();
					list.RedrawList();		// no need to redraw Bookmarks, as Markers are only shown in main list
					break;
				case ID_VIEW_SHOWBRANCHSCREENSHOTS:
					//switch "Show Branch Screenshots" flag
					taseditor_config.show_branch_screenshots ^= 1;
					taseditor_window.UpdateCheckedItems();
					break;
				case ID_VIEW_SHOWBRANCHTOOLTIPS:
					//switch "Show Branch Screenshots" flag
					taseditor_config.show_branch_descr ^= 1;
					taseditor_window.UpdateCheckedItems();
					break;
				case ID_VIEW_ENABLEHOTCHANGES:
					taseditor_config.enable_hot_changes ^= 1;
					taseditor_window.UpdateCheckedItems();
					list.RedrawList();		// redraw buttons text
					break;
				case ID_VIEW_JUMPWHENMAKINGUNDO:
					taseditor_config.jump_to_undo ^= 1;
					taseditor_window.UpdateCheckedItems();
					break;
				case ID_VIEW_FOLLOWMARKERNOTECONTEXT:
					taseditor_config.follow_note_context ^= 1;
					taseditor_window.UpdateCheckedItems();
					break;
				case CHECK_AUTORESTORE_PLAYBACK:
					//switch "Auto-restore last playback position" flag
					taseditor_config.restore_position ^= 1;
					taseditor_window.UpdateCheckedItems();
					break;
				case ID_CONFIG_SETGREENZONECAPACITY:
					{
						int new_capacity = taseditor_config.greenzone_capacity;
						if(CWin32InputBox::GetInteger("Greenzone capacity", "Keep savestates for how many frames?\n(actual limit of savestates can be 5 times more than the number provided)", new_capacity, hwndDlg) == IDOK)
						{
							if (new_capacity < GREENZONE_CAPACITY_MIN)
								new_capacity = GREENZONE_CAPACITY_MIN;
							else if (new_capacity > GREENZONE_CAPACITY_MAX)
								new_capacity = GREENZONE_CAPACITY_MAX;
							if (new_capacity < taseditor_config.greenzone_capacity)
							{
								taseditor_config.greenzone_capacity = new_capacity;
								greenzone.GreenzoneCleaning();
							} else taseditor_config.greenzone_capacity = new_capacity;
						}
						break;
					}
				case ID_CONFIG_SETMAXUNDOLEVELS:
					{
						int new_size = taseditor_config.undo_levels;
						if(CWin32InputBox::GetInteger("Max undo levels", "Keep history of how many changes?", new_size, hwndDlg) == IDOK)
						{
							if (new_size < UNDO_LEVELS_MIN)
								new_size = UNDO_LEVELS_MIN;
							else if (new_size > UNDO_LEVELS_MAX)
								new_size = UNDO_LEVELS_MAX;
							if (new_size != taseditor_config.undo_levels)
							{
								taseditor_config.undo_levels = new_size;
								history.reset();
								selection.reset();
								// hot changes were cleared, so update list
								list.RedrawList();
							}
						}
						break;
					}
				case ID_CONFIG_SETAUTOSAVEPERIOD:
					{
						int new_period = taseditor_config.autosave_period;
						if(CWin32InputBox::GetInteger("Autosave period", "How many minutes may the project stay not saved after being changed?\n(0 = no autosaves)", new_period, hwndDlg) == IDOK)
						{
							if (new_period < AUTOSAVE_PERIOD_MIN)
								new_period = AUTOSAVE_PERIOD_MIN;
							else if (new_period > AUTOSAVE_PERIOD_MAX)
								new_period = AUTOSAVE_PERIOD_MAX;
							taseditor_config.autosave_period = new_period;
							project.SheduleNextAutosave();	
						}
						break;
					}
				case ID_CONFIG_BRANCHESRESTOREFULLMOVIE:
					//switch "Branches restore entire Movie" flag
					taseditor_config.branch_full_movie ^= 1;
					taseditor_window.UpdateCheckedItems();
					break;
				case ID_CONFIG_BRANCHESWORKONLYWHENRECORDING:
					//switch "Branches work only when Recording" flag
					taseditor_config.branch_only_when_rec ^= 1;
					taseditor_window.UpdateCheckedItems();
					bookmarks.RedrawBookmarksCaption();
					break;
				case ID_CONFIG_HUDINBRANCHSCREENSHOTS:
					//switch "HUD in Branch screenshots" flag
					taseditor_config.branch_scr_hud ^= 1;
					taseditor_window.UpdateCheckedItems();
					break;
				case ID_CONFIG_BINDMARKERSTOINPUT:
					taseditor_config.bind_markers ^= 1;
					taseditor_window.UpdateCheckedItems();
					list.RedrawList();
					break;
				case ID_CONFIG_EMPTYNEWMARKERNOTES:
					taseditor_config.empty_marker_notes ^= 1;
					taseditor_window.UpdateCheckedItems();
					break;
				case ID_CONFIG_COMBINECONSECUTIVERECORDINGS:
					//switch "Combine consecutive Recordings" flag
					taseditor_config.combine_consecutive_rec ^= 1;
					taseditor_window.UpdateCheckedItems();
					break;
				case ID_CONFIG_USE1PFORRECORDING:
					//switch "Use 1P keys for single Recordings" flag
					taseditor_config.use_1p_rec ^= 1;
					taseditor_window.UpdateCheckedItems();
					break;
				case ID_CONFIG_USEINPUTKEYSFORCOLUMNSET:
					taseditor_config.columnset_by_keys ^= 1;
					taseditor_window.UpdateCheckedItems();
					break;
				case ID_CONFIG_KEYBOARDCONTROLSINLISTVIEW:
					taseditor_config.keyboard_for_listview ^= 1;
					taseditor_window.UpdateCheckedItems();
					break;
				case ID_CONFIG_SUPERIMPOSE_AFFECTS_PASTE:
					taseditor_config.superimpose_affects_paste ^= 1;
					taseditor_window.UpdateCheckedItems();
					break;
				case ID_CONFIG_MUTETURBO:
					muteTurbo ^= 1;
					taseditor_window.UpdateCheckedItems();
					break;
				case ID_CONFIG_SILENTAUTOSAVE:
					taseditor_config.silent_autosave ^= 1;
					taseditor_window.UpdateCheckedItems();
					break;
				case IDC_PROGRESS_BUTTON:
					playback.ClickOnProgressbar();
					break;
				case IDC_BRANCHES_BUTTON:
					// click on "Bookmarks/Branches" - switch "View Tree of branches"
					taseditor_config.view_branches_tree ^= 1;
					bookmarks.RedrawBookmarksCaption();
					break;
				case IDC_RECORDING:
					// toggle readonly, no need to recheck radiobuttons
					FCEUI_MovieToggleReadOnly();
					CheckDlgButton(taseditor_window.hwndTasEditor, IDC_RECORDING, movie_readonly?BST_UNCHECKED : BST_CHECKED);
					break;
				case IDC_RADIO2:
					recorder.multitrack_recording_joypad = MULTITRACK_RECORDING_ALL;
					break;
				case IDC_RADIO3:
					recorder.multitrack_recording_joypad = MULTITRACK_RECORDING_1P;
					break;
				case IDC_RADIO4:
					recorder.multitrack_recording_joypad = MULTITRACK_RECORDING_2P;
					break;
				case IDC_RADIO5:
					recorder.multitrack_recording_joypad = MULTITRACK_RECORDING_3P;
					break;
				case IDC_RADIO6:
					recorder.multitrack_recording_joypad = MULTITRACK_RECORDING_4P;
					break;
				case IDC_SUPERIMPOSE:
					// 3 states of "Superimpose" checkbox
					if (taseditor_config.superimpose == SUPERIMPOSE_UNCHECKED)
						taseditor_config.superimpose = SUPERIMPOSE_CHECKED;
					else if (taseditor_config.superimpose == SUPERIMPOSE_CHECKED)
						taseditor_config.superimpose = SUPERIMPOSE_INDETERMINATE;
					else taseditor_config.superimpose = SUPERIMPOSE_UNCHECKED;
					taseditor_window.UpdateCheckedItems();
					break;
				case ACCEL_CTRL_A:
					if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_UPPER)
						SendMessage(playback.hwndPlaybackMarkerEdit, EM_SETSEL, 0, -1); 
					else if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_LOWER)
						SendMessage(selection.hwndSelectionMarkerEdit, EM_SETSEL, 0, -1); 
					else
						selection.SelectBetweenMarkers();
					break;
				case ID_EDIT_SELECTMIDMARKERS:
				case ID_SELECTED_SELECTMIDMARKERS:
					selection.SelectBetweenMarkers();
					break;
				case ACCEL_CTRL_INSERT:
				case ID_EDIT_CLONEFRAMES:
				case ID_SELECTED_CLONE:
					splicer.CloneFrames();
					break;
				case ACCEL_CTRL_Z:
				case ID_EDIT_UNDO:
					{
						if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_UPPER)
						{
							SendMessage(playback.hwndPlaybackMarkerEdit, WM_UNDO, 0, 0); 
						} else if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_LOWER)
						{
							SendMessage(selection.hwndSelectionMarkerEdit, WM_UNDO, 0, 0); 
						} else
						{
							int result = history.undo();
							if (result >= 0)
							{
								list.update();
								list.FollowUndo();
								greenzone.InvalidateAndCheck(result);
							}
						}
						break;
					}
				case ACCEL_CTRL_Y:
				case ID_EDIT_REDO:
					{
						int result = history.redo();
						if (result >= 0)
						{
							list.update();
							list.FollowUndo();
							greenzone.InvalidateAndCheck(result);
						}
						break;
					}
				case ID_EDIT_SELECTIONUNDO:
				case ACCEL_CTRL_Q:
					{
						selection.undo();
						list.FollowSelection();
						break;
					}
				case ID_EDIT_SELECTIONREDO:
				case ACCEL_CTRL_W:
					{
						selection.redo();
						list.FollowSelection();
						break;
					}
				case ID_EDIT_RESELECTCLIPBOARD:
				case ACCEL_CTRL_B:
					{
						selection.ReselectClipboard();
						list.FollowSelection();
						break;
					}
				case IDC_JUMP_PLAYBACK_BUTTON:
					{
						list.FollowPlayback();
						break;
					}
				case IDC_JUMP_SELECTION_BUTTON:
					{
						list.FollowSelection();
						break;
					}
				case ID_SELECTED_SETMARKER:
					{
						SelectionFrames* current_selection = selection.MakeStrobe();
						if (current_selection->size())
						{
							SelectionFrames::iterator current_selection_begin(current_selection->begin());
							SelectionFrames::iterator current_selection_end(current_selection->end());
							bool changes_made = false;
							for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
							{
								if(!markers_manager.GetMarker(*it))
								{
									if (markers_manager.SetMarker(*it))
									{
										changes_made = true;
										list.RedrawRow(*it);
									}
								}
							}
							if (changes_made)
							{
								selection.must_find_current_marker = playback.must_find_current_marker = true;
								history.RegisterMarkersChange(MODTYPE_MARKER_SET, *current_selection_begin, *current_selection->rbegin());
							}
						}
						break;
					}
				case ID_SELECTED_REMOVEMARKER:
					{
						SelectionFrames* current_selection = selection.MakeStrobe();
						if (current_selection->size())
						{
							SelectionFrames::iterator current_selection_begin(current_selection->begin());
							SelectionFrames::iterator current_selection_end(current_selection->end());
							bool changes_made = false;
							for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
							{
								if(markers_manager.GetMarker(*it))
								{
									markers_manager.ClearMarker(*it);
									changes_made = true;
									list.RedrawRow(*it);
								}
							}
							if (changes_made)
							{
								selection.must_find_current_marker = playback.must_find_current_marker = true;
								history.RegisterMarkersChange(MODTYPE_MARKER_UNSET, *current_selection_begin, *current_selection->rbegin());
							}
						}
						break;
					}
				case ACCEL_SHIFT_PGUP:
					if (!playback.jump_was_used_this_frame)
						playback.RewindFull();
					break;
				case ACCEL_SHIFT_PGDN:
					if (!playback.jump_was_used_this_frame)
						playback.ForwardFull();
					break;
				case ACCEL_CTRL_PGUP:
					selection.JumpPrevMarker();
					break;
				case ACCEL_CTRL_PGDN:
					selection.JumpNextMarker();
					break;
				case ACCEL_CTRL_F:
				case ID_VIEW_FINDNOTE:
					{
						if (taseditor_window.hwndFindNote)
							SetFocus(GetDlgItem(taseditor_window.hwndFindNote, IDC_NOTE_TO_FIND));
						else
							taseditor_window.hwndFindNote = CreateDialog(fceu_hInstance, MAKEINTRESOURCE(IDD_TASEDITOR_FINDNOTE), taseditor_window.hwndTasEditor, FindNoteProc);
						break;
					}
				case TASEDITOR_FIND_BEST_SIMILAR_MARKER:
					markers_manager.FindSimilar();
					break;
				case TASEDITOR_FIND_NEXT_SIMILAR_MARKER:
					markers_manager.FindNextSimilar();
					break;
				case TASEDITOR_RUN_MANUAL:
					// the function will be called in next window update
					must_call_manual_lua_function = true;
					break;
				case IDC_RUN_AUTO:
					taseditor_config.enable_auto_function ^= 1;
					taseditor_window.UpdateCheckedItems();
					break;
				case ID_HELP_TASEDITHELP:
					{
						std::string helpFileName = BaseDirectory;
						helpFileName.append(taseditor_help_filename);
						HtmlHelp(GetDesktopWindow(), helpFileName.c_str(), HH_DISPLAY_TOPIC, (DWORD)NULL);
						break;
					}
				case ID_HELP_TOOLTIPS:
					taseditor_config.tooltips ^= 1;
					taseditor_window.UpdateCheckedItems();
					taseditor_window.UpdateTooltips();
					break;
				case ID_HELP_ABOUT:
					DialogBox(fceu_hInstance, MAKEINTRESOURCE(IDD_TASEDITOR_ABOUT), taseditor_window.hwndTasEditor, AboutProc);
					break;
				case ID_STRAY_UNPAUSE:
					playback.UnpauseEmulation();
					break;


				}
				break;
			}
		case WM_SYSKEYDOWN:
		{
			if (wParam == VK_F10)
				return 0;
			break;
		}
		default:
			break;
	}
	return FALSE;
}


