// Specification file for SPLICER class

class SPLICER
{
public:
	SPLICER();
	void init();
	void reset();
	void update();

	void CloneFrames();
	void InsertFrames();
	void InsertNumFrames();
	void DeleteFrames();
	void ClearFrames(SelectionFrames* current_selection = 0);
	void Truncate();
	bool Copy(SelectionFrames* current_selection = 0);
	void Cut();
	bool Paste();
	bool PasteInsert();

	void RedrawTextClipboard();

	SelectionFrames& GetClipboardSelection();

	bool must_redraw_selection_text;

private:
	void CheckClipboard();

	SelectionFrames clipboard_selection;
	HWND hwndTextSelection, hwndTextClipboard;

};
