//Specification file for TASEDITOR_WINDOW class
#define TASEDITOR_WINDOW_TOTAL_ITEMS 42

enum ECONTEXTMENU
{
	CONTEXTMENU_STRAY = 0,
	CONTEXTMENU_SELECTED = 1,
};

class TASEDITOR_WINDOW
{
public:
	TASEDITOR_WINDOW();
	void init();
	void exit();
	void reset();
	void update();

	void CalculateItems();
	void ResizeItems();

	void WindowMovedOrResized();

	void UpdateCaption();
	void RedrawTaseditor();

	void RightClick(LPNMITEMACTIVATE info);
	void StrayClickMenu(LPNMITEMACTIVATE info);
	void RightClickMenu(LPNMITEMACTIVATE info);

	void UpdateCheckedItems();

	void UpdateRecentProjectsMenu();
	void UpdateRecentProjectsArray(const char* addString);
	void RemoveRecentProject(unsigned int which);
	void LoadRecentProject(int slot);

	HWND hwndTasEditor, hwndFindNote;
	HMENU hmenu, hrmenu;
	HICON hTaseditorIcon;
	bool TASEditor_focus;
	bool ready_for_resizing;
	int min_width;
	int min_height;

private:

};
