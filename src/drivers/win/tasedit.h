#define CDDS_SUBITEMPREPAINT       (CDDS_SUBITEM | CDDS_ITEMPREPAINT)
#define CDDS_SUBITEMPOSTPAINT      (CDDS_SUBITEM | CDDS_ITEMPOSTPAINT)
#define CDDS_SUBITEMPREERASE       (CDDS_SUBITEM | CDDS_ITEMPREERASE)
#define CDDS_SUBITEMPOSTERASE      (CDDS_SUBITEM | CDDS_ITEMPOSTERASE)

#define NUM_JOYPADS 4
#define NUM_JOYPAD_BUTTONS 8
#define PROGRESSBAR_WIDTH 200

#define GREENZONE_CAPACITY_DEFAULT 10000
#define GREENZONE_CAPACITY_MIN 1
#define GREENZONE_CAPACITY_MAX 50000

#define UNDO_LEVELS_MIN 1
#define UNDO_LEVELS_MAX 999
#define UNDO_LEVELS_DEFAULT 100

// multitrack
#define MULTITRACK_RECORDING_ALL 0
#define MULTITRACK_RECORDING_1P 1
#define MULTITRACK_RECORDING_2P 2
#define MULTITRACK_RECORDING_3P 3
#define MULTITRACK_RECORDING_4P 4
// listview column names
#define COLUMN_ICONS 0
#define COLUMN_FRAMENUM	 1
#define COLUMN_JOYPAD1_A 2
#define COLUMN_JOYPAD1_B 3
#define COLUMN_JOYPAD1_S 4
#define COLUMN_JOYPAD1_T 5
#define COLUMN_JOYPAD1_U 6
#define COLUMN_JOYPAD1_D 7
#define COLUMN_JOYPAD1_L 8
#define COLUMN_JOYPAD1_R 9
#define COLUMN_JOYPAD2_A 10
#define COLUMN_JOYPAD2_B 11
#define COLUMN_JOYPAD2_S 12
#define COLUMN_JOYPAD2_T 13
#define COLUMN_JOYPAD2_U 14
#define COLUMN_JOYPAD2_D 15
#define COLUMN_JOYPAD2_L 16
#define COLUMN_JOYPAD2_R 17
#define COLUMN_JOYPAD3_A 18
#define COLUMN_JOYPAD3_B 19
#define COLUMN_JOYPAD3_S 20
#define COLUMN_JOYPAD3_T 21
#define COLUMN_JOYPAD3_U 22
#define COLUMN_JOYPAD3_D 23
#define COLUMN_JOYPAD3_L 24
#define COLUMN_JOYPAD3_R 25
#define COLUMN_JOYPAD4_A 26
#define COLUMN_JOYPAD4_B 27
#define COLUMN_JOYPAD4_S 28
#define COLUMN_JOYPAD4_T 29
#define COLUMN_JOYPAD4_U 30
#define COLUMN_JOYPAD4_D 31
#define COLUMN_JOYPAD4_L 32
#define COLUMN_JOYPAD4_R 33
#define COLUMN_FRAMENUM2 34
#define DIGITS_IN_FRAMENUM 7
#define ARROW_IMAGE_ID 20
// listview colors
#define NORMAL_FRAMENUM_COLOR 0xFFFFFF
#define NORMAL_INPUT_COLOR1 0xEDEDED
#define NORMAL_INPUT_COLOR2 0xDEDEDE

#define GREENZONE_FRAMENUM_COLOR 0xDDFFDD
#define GREENZONE_INPUT_COLOR1 0xC8F7C4
#define GREENZONE_INPUT_COLOR2 0xAEE2AE

#define PALE_GREENZONE_FRAMENUM_COLOR 0xE4FFE4
#define PALE_GREENZONE_INPUT_COLOR1 0xD5F9D4
#define PALE_GREENZONE_INPUT_COLOR2 0xBAE6BA

#define LAG_FRAMENUM_COLOR 0xDBDAFF
#define LAG_INPUT_COLOR1 0xCECBEF
#define LAG_INPUT_COLOR2 0xBEBAE4

#define PALE_LAG_FRAMENUM_COLOR 0xE1E1FF
#define PALE_LAG_INPUT_COLOR1 0xD6D3F1
#define PALE_LAG_INPUT_COLOR2 0xC7C4E8

#define CUR_FRAMENUM_COLOR 0xFCF1CE
#define CUR_INPUT_COLOR1 0xF7E9B2
#define CUR_INPUT_COLOR2 0xE4D8A8

#define UNDOHINT_FRAMENUM_COLOR 0xF9DDE6
#define UNDOHINT_INPUT_COLOR1 0xF6CCDD
#define UNDOHINT_INPUT_COLOR2 0xE5B7CC

#define MARKED_FRAMENUM_COLOR 0xC0FCFF
#define CUR_MARKED_FRAMENUM_COLOR 0xDEF7F4
#define MARKED_UNDOHINT_FRAMENUM_COLOR 0xE1E7EC

// greenzone cleaning masks
#define EVERY16TH 0xFFFFFFF0
#define EVERY8TH 0xFFFFFFF8
#define EVERY4TH 0xFFFFFFFC
#define EVERY2ND 0xFFFFFFFE
// -----------------------------
void EnterTasEdit();
void InitDialog();
bool ExitTasEdit();
void UpdateTasEdit();
void UpdateList();
void InputChangedRec();
bool CheckItemVisible(int frame);
void FollowPlayback();
void FollowUndo();
void FollowSelection();
void FollowPauseframe();
void ClearSelection();
void ClearRowSelection(int index);
void AddFourscore();
void RemoveFourscore();
void RedrawWindowCaption();
void RedrawTasedit();
void RedrawList();
void RedrawListAndBookmarks();
void RedrawRow(int index);
void RedrawRowAndBookmark(int index);
void ToggleJoypadBit(int column_index, int row_index, UINT KeyFlags);
void SwitchToReadOnly();
void UncheckRecordingRadioButtons();
void RecheckRecordingRadioButtons();
void OpenProject();
bool SaveProject();
bool SaveProjectAs();
bool AskSaveProject();
void SelectAll();
void SelectMidMarkers();
void CloneFrames();
void InsertFrames();
void DeleteFrames();
void ClearFrames(bool cut = false);
void Truncate();
void ColumnSet(int column);
bool Copy();
void Cut();
bool Paste();
void GotFocus();
void LostFocus();

