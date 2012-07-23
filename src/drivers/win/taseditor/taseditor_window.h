// Specification file for TASEDITOR_WINDOW class

#define TASEDITOR_WINDOW_TOTAL_ITEMS 43
#define PIANOROLL_IN_WINDOWITEMS 2

#define TOOLTIP_TEXT_MAX_LEN 127
#define TOOLTIPS_AUTOPOP_TIMEOUT 30000

#define PATTERNS_MENU_POS 5
#define PATTERNS_MAX_VISIBLE_NAME 50

struct Window_items_struct
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
};

class TASEDITOR_WINDOW
{
public:
	TASEDITOR_WINDOW();
	void init();
	void exit();
	void reset();
	void update();

	void ResizeItems();
	void WindowMovedOrResized();

	void UpdateTooltips();

	void UpdateCaption();
	void RedrawTaseditor();

	void UpdateCheckedItems();

	void UpdateRecentProjectsMenu();
	void UpdateRecentProjectsArray(const char* addString);
	void RemoveRecentProject(unsigned int which);
	void LoadRecentProject(int slot);

	void UpdatePatternsMenu();
	void RecheckPatternsMenu();

	HWND hwndTasEditor, hwndFindNote;
	bool TASEditor_focus;
	bool ready_for_resizing;
	int min_width;
	int min_height;

	bool must_update_mouse_cursor;

private:
	void CalculateItems();


	HWND hToolTipWnd;
	HMENU hmenu, patterns_menu;
	HICON hTaseditorIcon;

};
