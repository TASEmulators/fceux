//Specification file for TASEDITOR_WINDOW class

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

	HWND hwndTasEditor, hwndFindNote;
	HMENU hmenu, hrmenu;
	HICON hTaseditorIcon;
	bool TASEditor_focus;

	void RedrawCaption();
	void RedrawWindow();

	void RightClick(LPNMITEMACTIVATE info);
	void StrayClickMenu(LPNMITEMACTIVATE info);
	void RightClickMenu(LPNMITEMACTIVATE info);

	void UpdateCheckedItems();

	void UpdateRecentProjectsMenu();
	void UpdateRecentProjectsArray(const char* addString);
	void RemoveRecentProject(unsigned int which);
	void LoadRecentProject(int slot);


private:

};
