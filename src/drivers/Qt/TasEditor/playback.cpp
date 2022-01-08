/* ---------------------------------------------------------------------------------
Implementation file of Playback class
Copyright (c) 2011-2013 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Playback - Player of emulation states
[Single instance]

* implements the working of movie player: show any frame (jump), run/cancel seekng. pause, rewinding
* regularly tracks and controls emulation process, prompts redrawing of Piano Roll List rows, finishes seeking when reaching target frame, animates target frame, makes Piano Roll follow Playback cursor, detects if Playback cursor moved to another Marker and updates Note in the upper text field
* implements the working of upper buttons << and >> (jumping on Markers)
* implements the working of buttons < and > (frame-by-frame movement)
* implements the working of button || (pause) and middle mouse button, also reacts on external changes of emulation pause
* implements the working of progressbar: init, reset, set value, click (cancel seeking)
* also here's the code of upper text field (for editing Marker Notes)
* stores resources: upper text field prefix, timings of target frame animation, response times of GUI buttons, progressbar scale
------------------------------------------------------------------------------------ */

#include <QApplication>

#include "fceu.h"
#include "driver.h"
#include "Qt/TasEditor/taseditor_project.h"
#include "Qt/TasEditor/TasEditorWindow.h"

#ifdef _S9XLUA_H
extern void ForceExecuteLuaFrameFunctions();
#endif

extern bool mustRewindNow;
extern bool turbo;

// resources
static char upperMarkerText[] = "Marker ";

PLAYBACK::PLAYBACK()
{
}

void PLAYBACK::init()
{
	reset();
}
void PLAYBACK::reset()
{
	mustAutopauseAtTheEnd = true;
	mustFindCurrentMarker = true;
	displayedMarkerNumber = 0;
	lastCursorPos = currFrameCounter;
	lastPositionFrame = pauseFrame = oldPauseFrame = 0;
	lastPositionIsStable = oldStateOfShowPauseFrame = showPauseFrame = false;
	rewindButtonOldState = rewindButtonState = false;
	forwardButtonOldState = forwardButtonState = false;
	rewindFullButtonOldState = rewindFullButtonState = false;
	forwardFullButtonOldState = forwardFullButtonState = false;
	emuPausedOldState = emuPausedState = true;
	stopSeeking();
	turbo = false;
}
void PLAYBACK::update()
{
	// controls:
	// update < and > buttons
	rewindButtonOldState = rewindButtonState;
	rewindButtonState = tasWin->rewindFrmBtn->isDown();
	if (rewindButtonState)
	{
		if (!rewindButtonOldState)
		{
			buttonHoldTimer = getTasEditorTime();
			//handleRewindFrame();
		}
		else if (buttonHoldTimer + BUTTON_HOLD_REPEAT_DELAY < getTasEditorTime())
		{
			handleRewindFrame();
		}
	}
	forwardButtonOldState = forwardButtonState;
	forwardButtonState = tasWin->advFrmBtn->isDown();
	if (forwardButtonState && !rewindButtonState)
	{
		if (!forwardButtonOldState)
		{
			buttonHoldTimer = getTasEditorTime();
			//handleForwardFrame();
		}
		else if (buttonHoldTimer + BUTTON_HOLD_REPEAT_DELAY < getTasEditorTime())
		{
			handleForwardFrame();
		}
	}
	// update << and >> buttons
	rewindFullButtonOldState = rewindFullButtonState;
	rewindFullButtonState = tasWin->rewindMkrBtn->isDown();
	if (rewindFullButtonState && !rewindButtonState && !forwardButtonState)
	{
		if (!rewindFullButtonOldState)
		{
			buttonHoldTimer = getTasEditorTime();
			//handleRewindFull();
		}
		else if (buttonHoldTimer + BUTTON_HOLD_REPEAT_DELAY < getTasEditorTime())
		{
			handleRewindFull();
		}
	}
	forwardFullButtonOldState = forwardFullButtonState;
	forwardFullButtonState = tasWin->advMkrBtn->isDown();
	if (forwardFullButtonState && !rewindButtonState && !forwardButtonState && !rewindFullButtonState)
	{
		if (!forwardFullButtonOldState)
		{
			buttonHoldTimer = getTasEditorTime();
			//handleForwardFull();
		}
		else if (buttonHoldTimer + BUTTON_HOLD_REPEAT_DELAY < getTasEditorTime())
		{
			handleForwardFull();
		}
	}

	// update the Playback cursor
	if (currFrameCounter != lastCursorPos)
	{
		// update gfx of the old and new rows
		//pianoRoll.redrawRow(lastCursorPos);
		bookmarks->redrawChangedBookmarks(lastCursorPos);
		//pianoRoll.redrawRow(currFrameCounter);
		bookmarks->redrawChangedBookmarks(currFrameCounter);
		lastCursorPos = currFrameCounter;
		// follow the Playback cursor, but in case of seeking don't follow it
		tasWin->pianoRoll->followPlaybackCursorIfNeeded(false);	//pianoRoll.updatePlaybackCursorPositionInPianoRoll();	// an unfinished experiment
		// enforce redrawing now
		//UpdateWindow(pianoRoll.hwndList);
		// lazy update of "Playback's Marker text"
		int current_marker = markersManager->getMarkerAboveFrame(currFrameCounter);
		if (displayedMarkerNumber != current_marker)
		{
			markersManager->updateEditedMarkerNote();
			displayedMarkerNumber = current_marker;
			redrawMarkerData();
			mustFindCurrentMarker = false;
		}
	}
	// [non-lazy] update "Playback's Marker text" if needed
	if (mustFindCurrentMarker)
	{
		markersManager->updateEditedMarkerNote();
		displayedMarkerNumber = markersManager->getMarkerAboveFrame(currFrameCounter);
		redrawMarkerData();
		mustFindCurrentMarker = false;
	}

	// pause when seeking hits pause_frame
	if (pauseFrame && currFrameCounter + 1 >= pauseFrame)
	{
		stopSeeking();
	}
	else if (currFrameCounter >= getLastPosition() && currFrameCounter >= currMovieData.getNumRecords() - 1 && mustAutopauseAtTheEnd && taseditorConfig->autopauseAtTheEndOfMovie && !isTaseditorRecording())
	{
		// pause at the end of the movie
		pauseEmulation();
	}

	// update flashing pauseframe
	if (oldPauseFrame != pauseFrame && oldPauseFrame)
	{
		// pause_frame was changed, clear old_pauseframe gfx
		//pianoRoll.redrawRow(oldPauseFrame-1);
		bookmarks->redrawChangedBookmarks(oldPauseFrame-1);
	}
	oldPauseFrame = pauseFrame;
	oldStateOfShowPauseFrame = showPauseFrame;
	if (pauseFrame)
	{
		if (emuPausedState)
			showPauseFrame = (int)(getTasEditorTime() / PAUSEFRAME_BLINKING_PERIOD_WHEN_PAUSED) & 1;
		else
			showPauseFrame = (int)(getTasEditorTime() / PAUSEFRAME_BLINKING_PERIOD_WHEN_SEEKING) & 1;
	}
	else
	{
		showPauseFrame = false;
	}
	if (oldStateOfShowPauseFrame != showPauseFrame)
	{
		// update pauseframe gfx
		//pianoRoll.redrawRow(pauseFrame - 1);
		bookmarks->redrawChangedBookmarks(pauseFrame - 1);
	}

	// update seeking progressbar
	emuPausedOldState = emuPausedState;
	emuPausedState = (FCEUI_EmulationPaused() != 0);
	if (pauseFrame)
	{
		if (oldStateOfShowPauseFrame != showPauseFrame)		// update progressbar from time to time
		{
			// display seeking progress
			setProgressbar(currFrameCounter - seekingBeginningFrame, pauseFrame - seekingBeginningFrame);
		}
	}
	else if (emuPausedOldState != emuPausedState)
	{
		// emulator got paused/unpaused externally
		if (emuPausedOldState && !emuPausedState)
		{
			// externally unpaused - show empty progressbar
			setProgressbar(0, 1);
		}
		else
		{
			// externally paused - progressbar should be full
			setProgressbar(1, 1);
		}
	}

	// prepare to stop at the end of the movie in case user unpauses emulator
	if (emuPausedState)
	{
		if (currFrameCounter < currMovieData.getNumRecords() - 1)
		{
			mustAutopauseAtTheEnd = true;
		}
		else
		{
			mustAutopauseAtTheEnd = false;
		}
	}

	// this little statement is very important for adequate work of the "green arrow" and "Restore last position"
	if (!emuPausedState)
	{
		// when emulating, lost_position_frame becomes unstable (which means that it's probably not equal to the end of current segment anymore)
		lastPositionIsStable = false;
	}
}

// called after saving the project, because saving uses the progressbar for itself
void PLAYBACK::updateProgressbar()
{
	if (pauseFrame)
	{
		setProgressbar(currFrameCounter - seekingBeginningFrame, pauseFrame - seekingBeginningFrame);
	} else
	{
		if (emuPausedState)
			// full progressbar
			setProgressbar(1, 1);
		else
			// cleared progressbar
			setProgressbar(0, 1);
	}
	//RedrawWindow(hwndProgressbar, NULL, NULL, RDW_INVALIDATE);
}

void PLAYBACK::toggleEmulationPause()
{
	if (FCEUI_EmulationPaused())
		unpauseEmulation();
	else
		pauseEmulation();
}
void PLAYBACK::pauseEmulation()
{
	FCEUI_SetEmulationPaused(EMULATIONPAUSED_PAUSED);
}
void PLAYBACK::unpauseEmulation()
{
	FCEUI_SetEmulationPaused(0);
}
void PLAYBACK::restoreLastPosition()
{
	if (getLastPosition() > currFrameCounter)
	{
		if (emuPausedState)
			startSeekingToFrame(getLastPosition());
		else
			pauseEmulation();
	}
}
void PLAYBACK::handleMiddleButtonClick()
{
	if (emuPausedState)
	{
		int msState     = QApplication::mouseButtons();
		int kbModifiers = QApplication::keyboardModifiers();
		//bool   alt_down = (kbModifiers & Qt::AltModifier    ) ? 1 : 0;
		bool shift_down = (kbModifiers & Qt::ShiftModifier  ) ? 1 : 0;
		bool  ctrl_down = (kbModifiers & Qt::ControlModifier) ? 1 : 0;
		bool rgtMsBtnDown = (msState & Qt::RightButton) ? 1 : 0;

		// Unpause or start seeking
		// works only when right mouse button is released
		if (!rgtMsBtnDown)
		{
			if (shift_down)
			{
				// if Shift is held, seek to nearest Marker
				int last_frame = markersManager->getMarkersArraySize() - 1;	// the end of movie Markers
				int target_frame = currFrameCounter + 1;
				for (; target_frame <= last_frame; ++target_frame)
					if (markersManager->getMarkerAtFrame(target_frame)) break;
				if (target_frame <= last_frame)
					startSeekingToFrame(target_frame);
			}
			else if (ctrl_down)
			{
				// if Ctrl is held, seek to Selection cursor or replay from Selection cursor
				int selection_beginning = selection->getCurrentRowsSelectionBeginning();
				if (selection_beginning > currFrameCounter)
				{
					startSeekingToFrame(selection_beginning);
				}
				else if (selection_beginning < currFrameCounter)
				{
					int saved_currFrameCounter = currFrameCounter;
					if (selection_beginning < 0)
						selection_beginning = 0;
					jump(selection_beginning);
					startSeekingToFrame(saved_currFrameCounter);
				}
			}
			else if (getPauseFrame() < 0 && getLastPosition() >= greenzone->getSize())
			{
				restoreLastPosition();
			}
			else
			{
				unpauseEmulation();
			}
		}
	}
	else
	{
		pauseEmulation();
	}
}

void PLAYBACK::startSeekingToFrame(int frame)
{
	if ((pauseFrame - 1) != frame)
	{
		seekingBeginningFrame = currFrameCounter;
		pauseFrame = frame + 1;
	}
	if (taseditorConfig->turboSeek)
	{
		//printf("Turbo seek on\n");
		turbo = true;
	}
	unpauseEmulation();
}
void PLAYBACK::stopSeeking()
{
	pauseFrame = 0;
	//if ( turbo )
	//{
	//	printf("Turbo seek off\n");
	//}
	turbo = false;
	pauseEmulation();
	setProgressbar(1, 1);
}

void PLAYBACK::handleRewindFrame()
{
	if (pauseFrame && !emuPausedState) return;
	if (currFrameCounter > 0)
	{
		jump(currFrameCounter - 1);
	}
	else
	{
		// cursor is at frame 0 - can't rewind, but still must make cursor visible if needed
		tasWin->pianoRoll->followPlaybackCursorIfNeeded(true);
	}
	if (!pauseFrame)
	{
		pauseEmulation();
	}
}
void PLAYBACK::handleForwardFrame()
{
	if (pauseFrame && !emuPausedState) return;
	jump(currFrameCounter + 1);
	if (!pauseFrame)
	{
		pauseEmulation();
	}
	turbo = false;
}
void PLAYBACK::handleRewindFull(int speed)
{
	int index = currFrameCounter - 1;
	// jump trough "speed" amount of previous Markers
	while (speed > 0)
	{
		for (; index >= 0; index--)
			if (markersManager->getMarkerAtFrame(index)) break;
		speed--;
	}
	if (index >= 0)
		jump(index);						// jump to the Marker
	else
		jump(0);							// jump to the beginning of Piano Roll
}
void PLAYBACK::handleForwardFull(int speed)
{
	int last_frame = markersManager->getMarkersArraySize() - 1;	// the end of movie Markers
	int index = currFrameCounter + 1;
	// jump trough "speed" amount of next Markers
	while (speed > 0)
	{
		for (; index <= last_frame; ++index)
			if (markersManager->getMarkerAtFrame(index)) break;
		speed--;
	}
	if (index <= last_frame)
		jump(index);								// jump to Marker
	else
		jump(currMovieData.getNumRecords() - 1);	// jump to the end of Piano Roll
}

void PLAYBACK::redrawMarkerData()
{
	// redraw Marker num
	char new_text[MAX_NOTE_LEN] = {0};
	if (displayedMarkerNumber <= 9999)		// if there's too many digits in the number then don't show the word "Marker" before the number
	{
		strcpy(new_text, upperMarkerText);
	}
	char num[16];
	sprintf( num, "%i", displayedMarkerNumber);
	strcat(new_text, num);
	strcat(new_text, " ");
	tasWin->upperMarkerLabel->setText( QObject::tr(new_text) );
	// change Marker Note
	strcpy(new_text, markersManager->getNoteCopy(displayedMarkerNumber).c_str());
	tasWin->upperMarkerNote->setText( QObject::tr(new_text) );
	// reset search_similar_marker, because source Marker changed
	markersManager->currentIterationOfFindSimilar = 0;
}

void PLAYBACK::restartPlaybackFromZeroGround()
{
	poweron(true);
	FCEUMOV_ClearCommands();		// clear POWER SWITCH command caused by poweron()
	currFrameCounter = 0;
	// if there's no frames in current movie, create initial frame record
	if (currMovieData.getNumRecords() == 0)
		currMovieData.insertEmpty(-1, 1);
}

void PLAYBACK::ensurePlaybackIsInsideGreenzone(bool executeLua)
{
	// set the Playback cursor to the frame or at least above the frame
	if (setPlaybackAboveOrToFrame(greenzone->getSize() - 1))
	{
		// since the game state was changed by this jump, we must update possible Lua callbacks and other tools that would normally only update in FCEUI_Emulate
		if (executeLua)
		{
			ForceExecuteLuaFrameFunctions();
		}
		//Update_RAM_Search(); // Update_RAM_Watch() is also called.
	}
	// follow the Playback cursor, but in case of seeking don't follow it
	tasWin->pianoRoll->followPlaybackCursorIfNeeded(false);
}

// an interface for sending Playback cursor to any frame
void PLAYBACK::jump(int frame, bool forceStateReload, bool executeLua, bool followPauseframe)
{
	if (frame < 0) return;

	int lastCursor = currFrameCounter;

	// 1 - set the Playback cursor to the frame or at least above the frame
	if (setPlaybackAboveOrToFrame(frame, forceStateReload))
	{
		// since the game state was changed by this jump, we must update possible Lua callbacks and other tools that would normally only update in FCEUI_Emulate
		if (executeLua)
			ForceExecuteLuaFrameFunctions();
		//Update_RAM_Search(); // Update_RAM_Watch() is also called.
	}

	// 2 - seek from the current frame if we still aren't at the needed frame
	if (frame > currFrameCounter)
	{
		startSeekingToFrame(frame);
	} else
	{
		// the Playback is already at the needed frame
		if (pauseFrame)	// if Playback was seeking, pause emulation right here
		{
			stopSeeking();
		}
	}

	// follow the Playback cursor, and optionally follow pauseframe (if seeking was launched)
	tasWin->pianoRoll->followPlaybackCursorIfNeeded(followPauseframe);

	// redraw respective Piano Roll lines if needed
	if (lastCursor != currFrameCounter)
	{
		// redraw row where Playback cursor was (in case there's two or more drags before playback.update())
		//pianoRoll.redrawRow(lastCursor);
		bookmarks->redrawChangedBookmarks(lastCursor);
	}
}

// returns true if the game state was changed (loaded)
bool PLAYBACK::setPlaybackAboveOrToFrame(int frame, bool forceStateReload)
{
	bool state_changed = false;
	// search backwards for an earlier frame with valid savestate
	int i = greenzone->getSize() - 1;
	if (i > frame)
	{
		i = frame;
	}
	for (; i >= 0; i--)
	{
		if (!forceStateReload && !state_changed && i == currFrameCounter)
		{
			// we can remain at current game state
			break;
		} else if (!greenzone->isSavestateEmpty(i))
		{
			state_changed = true;	// after we once tried loading a savestate, we cannot use currFrameCounter state anymore, because the game state might have been corrupted by this loading attempt
			if (greenzone->loadSavestateOfFrame(i))
				break;
		}
	}
	if (i < 0)
	{
		// couldn't find a savestate
		restartPlaybackFromZeroGround();
		state_changed = true;
	}
	return state_changed;
}

void PLAYBACK::setLastPosition(int frame)
{
	if ((lastPositionFrame - 1 < frame) || (lastPositionFrame - 1 >= frame && !lastPositionIsStable))
	{
		if (lastPositionFrame)
		{
			//pianoRoll.redrawRow(lastPositionFrame - 1);
		}
		lastPositionFrame = frame + 1;
		lastPositionIsStable = true;
	}
}
int PLAYBACK::getLastPosition()
{
	return lastPositionFrame - 1;
}

int PLAYBACK::getPauseFrame()
{
	return pauseFrame - 1;
}
int PLAYBACK::getFlashingPauseFrame()
{
	if (showPauseFrame)
		return pauseFrame;
	else
		return 0;
}

void PLAYBACK::setProgressbar(int a, int b)
{
	// TODO
	//SendMessage(hwndProgressbar, PBM_SETPOS, PROGRESSBAR_WIDTH * a / b, 0);
}
void PLAYBACK::cancelSeeking()
{
	if (pauseFrame)
		stopSeeking();
}
// -------------------------------------------------------------------------
UpperMarkerNoteEdit::UpperMarkerNoteEdit( QWidget *parent )
	: QLineEdit(parent)
{
	setEchoMode( QLineEdit::Normal );
}
// -------------------------------------------------------------------------
UpperMarkerNoteEdit::~UpperMarkerNoteEdit(void)
{
}
// -------------------------------------------------------------------------
void UpperMarkerNoteEdit::focusInEvent(QFocusEvent *event)
{
	markersManager->markerNoteEditMode = MARKER_NOTE_EDIT_UPPER;
	QLineEdit::focusInEvent(event);
}
// -------------------------------------------------------------------------
void UpperMarkerNoteEdit::focusOutEvent(QFocusEvent *event)
{
	// if we were editing, save and finish editing
	if (markersManager->markerNoteEditMode == MARKER_NOTE_EDIT_UPPER)
	{
		markersManager->updateEditedMarkerNote();
		markersManager->markerNoteEditMode = MARKER_NOTE_EDIT_NONE;
	}
	QLineEdit::focusOutEvent(event);
}
// -------------------------------------------------------------------------
void UpperMarkerNoteEdit::keyPressEvent(QKeyEvent *event)
{
	//printf("Key Press: 0x%x \n", event->key() );

	if ( event->key() == Qt::Key_Escape)
	{
		setText( QString::fromStdString(markersManager->getNoteCopy(playback->displayedMarkerNumber)) );
		event->accept();
	}
	else if ( (event->key() == Qt::Key_Enter) || (event->key() == Qt::Key_Return) )
	{
		if (markersManager->markerNoteEditMode == MARKER_NOTE_EDIT_UPPER)
		{
			markersManager->updateEditedMarkerNote();
			markersManager->markerNoteEditMode = MARKER_NOTE_EDIT_NONE;
		}
		tasWin->pianoRoll->setFocus();
		event->accept();
	}
	else
	{
		QLineEdit::keyPressEvent(event);
	}
}
// -------------------------------------------------------------------------
void UpperMarkerNoteEdit::mousePressEvent(QMouseEvent * event)
{
	if ( event->button() == Qt::MiddleButton )
	{
		playback->handleMiddleButtonClick();
	}
	else if ( (event->button() == Qt::LeftButton) || (event->button() == Qt::RightButton) )
	{
		// scroll to the Marker
		if (taseditorConfig->followMarkerNoteContext)
		{
			tasWin->pianoRoll->followMarker(playback->displayedMarkerNumber);
		}
	}
}
// -------------------------------------------------------------------------
