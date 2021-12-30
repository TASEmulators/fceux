// Specification file for SELECTION class
#pragma once

#include <set>
#include <map>
#include <vector>
#include <time.h>

#include <QLineEdit>

typedef std::set<int> RowsSelection;

#define SELECTION_ID_LEN 10

class LowerMarkerNoteEdit : public QLineEdit
{
	Q_OBJECT

	public:
		LowerMarkerNoteEdit(QWidget *parent = 0);
		~LowerMarkerNoteEdit(void);	

	protected:
		void focusInEvent(QFocusEvent *event);
		void focusOutEvent(QFocusEvent *event);
		void keyPressEvent(QKeyEvent *event);
		void mousePressEvent(QMouseEvent * event);
};

class SELECTION
{
public:
	SELECTION(void);
	void init(void);
	void free(void);
	void reset(void);
	void reset_vars(void);
	void update(void);

	void updateSelectionSize(void);

	void updateHistoryLogSize(void);

	void redrawMarkerData(void);

	void save(EMUFILE *os, bool really_save = true);
	bool load(EMUFILE *is, unsigned int offset);
	void saveSelection(RowsSelection& selection, EMUFILE *os);
	bool loadSelection(RowsSelection& selection, EMUFILE *is);
	bool skipLoadSelection(EMUFILE *is);

	void noteThatItemRangeChanged(int startItem, int endItem, int newValue);
	void noteThatItemChanged(int itemIndex, int value);

	void addNewSelectionToHistory(void);
	void addCurrentSelectionToHistory(void);

	void undo(void);
	void redo(void);

	bool isRowSelected(int index);

	void clearAllRowsSelection(void);
	void clearSingleRowSelection(int index);
	void clearRegionOfRowsSelection(int start, int end);

	void selectAllRows(void);
	void setRowSelection(int index);
	void setRegionOfRowsSelection(int start, int end);

	void setRegionOfRowsSelectionUsingPattern(int start, int end);
	void selectAllRowsBetweenMarkers(void);

	void reselectClipboard(void);

	void transposeVertically(int shift);

	void jumpToPreviousMarker(int speed = 1);
	void jumpToNextMarker(int speed = 1);

	void jumpToFrame(int frame);

	// getters
	int getCurrentRowsSelectionSize(void);
	int getCurrentRowsSelectionBeginning(void);
	int getCurrentRowsSelectionEnd(void);
	RowsSelection* getCopyOfCurrentRowsSelection(void);

	bool mustFindCurrentMarker;
	int displayedMarkerNumber;

	//HWND hwndPreviousMarkerButton, hwndNextMarkerButton;
	//HWND hwndSelectionMarkerNumber, hwndSelectionMarkerEditField;

private:

	void jumpInTime(int new_pos);
	void enforceRowsSelectionToList();

	RowsSelection& getCurrentRowsSelection();

	bool trackSelectionChanges;
	int lastSelectionBeginning;

	bool previousMarkerButtonState, previousMarkerButtonOldState;
	bool nextMarkerButtonState, nextMarkerButtonOldState;
	clock_t buttonHoldTimer;

	std::vector<RowsSelection> rowsSelectionHistory;

	int historyCursorPos;
	int historyStartPos;
	int historySize;
	int historyTotalItems;

	RowsSelection tempRowsSelection;

	std::map <int, int> selList;
};
