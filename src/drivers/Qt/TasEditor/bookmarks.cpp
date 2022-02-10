/* ---------------------------------------------------------------------------------
Implementation file of Bookmarks class
Copyright (c) 2011-2013 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Bookmarks/Branches - Manager of Bookmarks
[Single instance]

* stores 10 Bookmarks
* implements all operations with Bookmarks: initialization, setting Bookmarks, jumping to Bookmarks, deploying Branches
* saves and loads the data from a project file. On error: resets all Bookmarks and Branches
* implements the working of Bookmarks List: creating, redrawing, mouseover, clicks
* regularly updates flashings in Bookmarks List
* on demand: updates colors of rows in Bookmarks List, reflecting conditions of respective Piano Roll rows
* stores resources: save id, ids of commands, captions for panel, gradients for flashings, id of default slot
------------------------------------------------------------------------------------ */

#include <QToolTip>

#include <zlib.h>
#include "utils/xstring.h"
#include "Qt/fceuWrapper.h"
#include "Qt/TasEditor/taseditor_project.h"
#include "Qt/TasEditor/TasEditorWindow.h"
#include "Qt/TasEditor/TasColors.h"

// resources
static char bookmarks_save_id[BOOKMARKS_ID_LEN] = "BOOKMARKS";
static char bookmarks_skipsave_id[BOOKMARKS_ID_LEN] = "BOOKMARKX";
// color tables for flashing when saving/loading bookmarks
//COLORREF bookmark_flash_colors[TOTAL_BOOKMARK_COMMANDS][FLASH_PHASE_MAX+1] = {
//	// set
//	//0x122330, 0x1b3541, 0x254753, 0x2e5964, 0x376b75, 0x417e87, 0x4a8f97, 0x53a1a8, 0x5db3b9, 0x66c5cb, 0x70d7dc, 0x79e9ed, 
//	0x0d1241, 0x111853, 0x161e64, 0x1a2575, 0x1f2b87, 0x233197, 0x2837a8, 0x2c3db9, 0x3144cb, 0x354adc, 0x3a50ed, 0x3f57ff, 
//	// jump
//	0x14350f, 0x1c480f, 0x235a0f, 0x2a6c0f, 0x317f10, 0x38910f, 0x3fa30f, 0x46b50f, 0x4dc80f, 0x54da0f, 0x5bec0f, 0x63ff10, 
//	// deploy
//	0x43171d, 0x541d21, 0x652325, 0x762929, 0x872f2c, 0x983530, 0xa93b34, 0xba4137, 0xcb463b, 0xdc4c3f, 0xed5243, 0xff5947 };

BOOKMARKS::BOOKMARKS(QWidget *parent)
	: QWidget(parent)
{
	std::string fontString;

	viewWidth  = 256;
	viewHeight = 256;

	editMode = EDIT_MODE_BOOKMARKS;

	imageItem  = 0;
	imageTimer = new QTimer(this);
	imageTimer->setSingleShot(true);
	imageTimer->setInterval(100);
	connect( imageTimer, SIGNAL(timeout(void)), this, SLOT(showImage(void)) );

	g_config->getOption("SDL.TasBookmarksFont", &fontString);

	if ( fontString.size() > 0 )
	{
		font.fromString( QString::fromStdString( fontString ) );
	}
	else
	{
		font.setFamily("Courier New");
		font.setStyle( QFont::StyleNormal );
		font.setStyleHint( QFont::Monospace );
	}
	font.setBold(true);

	this->setFocusPolicy(Qt::StrongFocus);
	this->setMouseTracking(true);

	calcFontData();

	redrawFlag = false;
	nextFlashUpdateTime = 0;
}

BOOKMARKS::~BOOKMARKS(void)
{
}

void BOOKMARKS::init()
{
	free();

	reset();
	selectedSlot = DEFAULT_SLOT;
	imageItem = 0;
	editMode = EDIT_MODE_BOOKMARKS;

	redrawBookmarksSectionCaption();
}
void BOOKMARKS::free()
{
	bookmarksArray.resize(0);
}
void BOOKMARKS::reset()
{
	// delete all commands if there are any
	commands.resize(0);
	// init bookmarks
	bookmarksArray.resize(0);
	bookmarksArray.resize(TOTAL_BOOKMARKS);
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
	{
		bookmarksArray[i].init();
	}
	reset_vars();
}
void BOOKMARKS::reset_vars()
{
	mouseX = mouseY = -1;
	itemUnderMouse = ITEM_UNDER_MOUSE_NONE;
	mouseOverBranchesBitmap = false;
	mustCheckItemUnderMouse = true;
	bookmarkLeftclicked = bookmarkRightclicked = ITEM_UNDER_MOUSE_NONE;
	nextFlashUpdateTime = getTasEditorTime() + BOOKMARKS_FLASH_TICK;
	imageItem = 0;
}

void BOOKMARKS::setFont( QFont &newFont )
{
	font = newFont;
	font.setBold(true);
	QWidget::setFont( font );
	calcFontData();
}

void BOOKMARKS::calcFontData(void)
{
	int w,h;
	QWidget::setFont(font);
	QFontMetrics metrics(font);
#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
	pxCharWidth = metrics.horizontalAdvance(QLatin1Char('2'));
#else
	pxCharWidth = metrics.width(QLatin1Char('2'));
#endif
	pxCharHeight   = metrics.capHeight();
	pxLineSpacing  = metrics.lineSpacing() * 1.25;
	pxLineLead     = pxLineSpacing - metrics.height();
	pxCursorHeight = metrics.height();
	pxLineTextOfs  = pxLineSpacing - ((pxLineSpacing - pxCharHeight) / 2) + (pxLineSpacing - pxCharHeight + 1) % 2;

	//printf("W:%i  H:%i  LS:%i  \n", pxCharWidth, pxCharHeight, pxLineSpacing );

	viewLines   = (viewHeight / pxLineSpacing) + 1;

	pxWidthCol1     =  3 * pxCharWidth;
	pxWidthFrameCol =  9 * pxCharWidth;
	pxWidthTimeCol  = 10 * pxCharWidth;

	pxLineWidth = pxWidthCol1 + pxWidthFrameCol + pxWidthTimeCol;

	pxStartCol1 =  0;
	pxStartCol2 =  pxWidthCol1;
	pxStartCol3 =  pxWidthCol1 + pxWidthFrameCol;

	w = pxLineWidth;
	h = pxLineSpacing * TOTAL_BOOKMARKS;

	setMinimumWidth( w );
	setMinimumHeight( h );
	resize(w,h);
}

void BOOKMARKS::update()
{
	// execute all commands accumulated during last frame
	for (int i = 0; (i + 1) < (int)commands.size(); )	// FIFO
	{
		int command_id = commands[i++];
		int slot = commands[i++];
		switch (command_id)
		{
			case COMMAND_SET:
				set(slot);
				break;
			case COMMAND_JUMP:
				jump(slot);
				break;
			case COMMAND_DEPLOY:
				deploy(slot);
				break;
			case COMMAND_SELECT:
				if (selectedSlot != slot)
				{
					int old_selected_slot = selectedSlot;
					selectedSlot = slot;
					redrawBookmark(old_selected_slot);
					redrawBookmark(selectedSlot);
				}
				break;
		}
	}
	commands.resize(0);

	// once per 100 milliseconds update bookmark flashes
	if (getTasEditorTime() > nextFlashUpdateTime)
	{
		nextFlashUpdateTime = getTasEditorTime() + BOOKMARKS_FLASH_TICK;
		for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
		{
			if (bookmarkRightclicked == i || bookmarkLeftclicked == i)
			{
				if (bookmarksArray[i].flashPhase != FLASH_PHASE_BUTTONHELD)
				{
					bookmarksArray[i].flashPhase = FLASH_PHASE_BUTTONHELD;
					redrawBookmark(i);
					branches->mustRedrawBranchesBitmap = true;		// because border of branch digit has changed
				}
			} else
			{
				if (bookmarksArray[i].flashPhase > 0)
				{
					bookmarksArray[i].flashPhase--;
					redrawBookmark(i);
					branches->mustRedrawBranchesBitmap = true;		// because border of branch digit has changed
				}
			}
		}
	}

	// controls
	if (mustCheckItemUnderMouse)
	{
		if (editMode == EDIT_MODE_BRANCHES)
		{
			itemUnderMouse = branches->findItemUnderMouse(mouseX, mouseY);
		}
		else if (editMode == EDIT_MODE_BOTH)
		{
			itemUnderMouse = findItemUnderMouse();
		}
		else
		{
			itemUnderMouse = ITEM_UNDER_MOUSE_NONE;
		}
		mustCheckItemUnderMouse = false;
		redrawFlag = true;
	}

	if ( redrawFlag )
	{
		QWidget::update();
		redrawFlag = false;
	}
}

// stores commands in array for update() function
void BOOKMARKS::command(int command_id, int slot)
{
	if (slot < 0)
	{
		slot = selectedSlot;
	}
	if (slot >= 0 && slot < TOTAL_BOOKMARKS)
	{
		commands.push_back(command_id);
		commands.push_back(slot);
	}
}

void BOOKMARKS::set(int slot)
{
	if (slot < 0 || slot >= TOTAL_BOOKMARKS) return;

	// First save changes in edited note (in case it's being currently edited)
	markersManager->updateEditedMarkerNote();

	int previous_frame = bookmarksArray[slot].snapshot.keyFrame;
	if (bookmarksArray[slot].isDifferentFromCurrentMovie())
	{
		BOOKMARK backup_copy(bookmarksArray[slot]);
		bookmarksArray[slot].set();
		// rebuild Branches Tree
		int old_current_branch = branches->getCurrentBranch();
		branches->handleBookmarkSet(slot);
		if (slot != old_current_branch && old_current_branch != ITEM_UNDER_MOUSE_CLOUD)
		{
			// current_branch was switched to slot, redraw Bookmarks List to change the color of digits
			//pianoRoll.redrawRow(bookmarksArray[old_current_branch].snapshot.keyFrame);
			redrawChangedBookmarks(bookmarksArray[old_current_branch].snapshot.keyFrame);
		}
		// also redraw List rows
		if (previous_frame >= 0 && previous_frame != currFrameCounter)
		{
			//pianoRoll.redrawRow(previous_frame);
			redrawChangedBookmarks(previous_frame);
		}
		//pianoRoll.redrawRow(currFrameCounter);
		redrawChangedBookmarks(currFrameCounter);
		// if screenshot of the slot is currently shown - reinit and redraw the picture
		//if (popupDisplay.currentlyDisplayedBookmark == slot)
		//{
		//	popupDisplay.currentlyDisplayedBookmark = ITEM_UNDER_MOUSE_NONE;
		//}

		history->registerBookmarkSet(slot, backup_copy, old_current_branch);
		mustCheckItemUnderMouse = true;
		FCEU_DispMessage("Branch %d saved.", 0, slot);
	}
}

void BOOKMARKS::jump(int slot)
{
	if (slot < 0 || slot >= TOTAL_BOOKMARKS) return;
	if (bookmarksArray[slot].notEmpty)
	{
		int frame = bookmarksArray[slot].snapshot.keyFrame;
		playback->jump(frame);
		bookmarksArray[slot].handleJump();
	}
}

void BOOKMARKS::deploy(int slot)
{
	recorder->stateWasLoadedInReadWriteMode = true;
	if (taseditorConfig->oldControlSchemeForBranching && movie_readonly)
	{
		jump(slot);
		return;
	}
	if (slot < 0 || slot >= TOTAL_BOOKMARKS) return;
	if (!bookmarksArray[slot].notEmpty) return;

	int keyframe = bookmarksArray[slot].snapshot.keyFrame;
	bool markers_changed = false;
	// revert Markers to the Bookmarked state
	if (bookmarksArray[slot].snapshot.areMarkersDifferentFromCurrentMarkers())
	{
		bookmarksArray[slot].snapshot.copyToCurrentMarkers();
		markers_changed = true;
	}
	// revert current movie data to the Bookmarked state
	if (taseditorConfig->branchesRestoreEntireMovie)
	{
		bookmarksArray[slot].snapshot.inputlog.toMovie(currMovieData);
	} else
	{
		// restore movie up to and not including bookmarked frame (simulating old TASing method)
		if (keyframe)
			bookmarksArray[slot].snapshot.inputlog.toMovie(currMovieData, 0, keyframe - 1);
		else
			currMovieData.truncateAt(0);
		// add empty frame at the end (at keyframe)
		currMovieData.insertEmpty(-1, 1);
	}

	int first_change = history->registerBranching(slot, markers_changed);	// this also reverts Greenzone's LagLog if needed
	if (first_change >= 0)
	{
		selection->mustFindCurrentMarker = playback->mustFindCurrentMarker = true;
		//pianoRoll.updateLinesCount();
		greenzone->invalidate(first_change);
		bookmarksArray[slot].handleDeploy();
	} else if (markers_changed)
	{
		selection->mustFindCurrentMarker = playback->mustFindCurrentMarker = true;
		bookmarksArray[slot].handleDeploy();
	} else
	{
		// didn't change anything in current movie
		bookmarksArray[slot].handleJump();
	}

	// jump to the target (bookmarked frame)
	if (greenzone->isSavestateEmpty(keyframe))
	{
		greenzone->writeSavestateForFrame(keyframe, bookmarksArray[slot].savestate);
	}
	playback->jump(keyframe, true);
	// switch current branch to this branch
	int old_current_branch = branches->getCurrentBranch();
	branches->handleBookmarkDeploy(slot);
	if (slot != old_current_branch && old_current_branch != ITEM_UNDER_MOUSE_CLOUD)
	{
		//pianoRoll.redrawRow(bookmarksArray[old_current_branch].snapshot.keyFrame);
		redrawChangedBookmarks(bookmarksArray[old_current_branch].snapshot.keyFrame);
		//pianoRoll.redrawRow(keyframe);
		redrawChangedBookmarks(keyframe);
	}
	FCEU_DispMessage("Branch %d loaded.", 0, slot);
	//pianoRoll.redraw();	// even though the Greenzone invalidation most likely have already sent the command to redraw
}

void BOOKMARKS::save(EMUFILE *os, bool really_save)
{
	if (really_save)
	{
		setTasProjectProgressBarText("Saving Bookmarks...");
		setTasProjectProgressBar( 0, TOTAL_BOOKMARKS );

		// write "BOOKMARKS" string
		os->fwrite(bookmarks_save_id, BOOKMARKS_ID_LEN);
		// write all 10 bookmarks
		for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
		{
			setTasProjectProgressBar( i, TOTAL_BOOKMARKS );
			bookmarksArray[i].save(os);
		}
		setTasProjectProgressBar( TOTAL_BOOKMARKS, TOTAL_BOOKMARKS );

		// write branches
		branches->save(os);
	} else
	{
		// write "BOOKMARKX" string
		os->fwrite(bookmarks_skipsave_id, BOOKMARKS_ID_LEN);
	}
}
// returns true if couldn't load
bool BOOKMARKS::load(EMUFILE *is, unsigned int offset)
{
	if (offset)
	{
		if (is->fseek(offset, SEEK_SET)) goto error;
	} else
	{
		reset();
		branches->reset();
		return false;
	}
	// read "BOOKMARKS" string
	char save_id[BOOKMARKS_ID_LEN];
	if ((int)is->fread(save_id, BOOKMARKS_ID_LEN) < BOOKMARKS_ID_LEN) goto error;
	if (!strcmp(bookmarks_skipsave_id, save_id))
	{
		// string says to skip loading Bookmarks
		FCEU_printf("No Bookmarks in the file\n");
		reset();
		branches->reset();
		return false;
	}
	if (strcmp(bookmarks_save_id, save_id)) goto error;		// string is not valid
	// read all 10 bookmarks
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
		if (bookmarksArray[i].load(is)) goto error;
	// read branches
	if (branches->load(is)) goto error;
	// all ok
	reset_vars();
	redrawBookmarksSectionCaption();
	return false;
error:
	FCEU_printf("Error loading Bookmarks\n");
	reset();
	branches->reset();
	return true;
}
// ----------------------------------------------------------
void BOOKMARKS::redrawBookmarksSectionCaption()
{
	int prev_edit_mode = editMode;
	if ( taseditorConfig == NULL )
	{
		return;
	}
	if (taseditorConfig->displayBranchesTree)
	{
		editMode = EDIT_MODE_BRANCHES;
		//ShowWindow(hwndBookmarksList, SW_HIDE);
		//ShowWindow(hwndBranchesBitmap, SW_SHOW);
	}
	else if (taseditorConfig->oldControlSchemeForBranching && movie_readonly)
	{
		editMode = EDIT_MODE_BOOKMARKS;
		//ShowWindow(hwndBranchesBitmap, SW_HIDE);
		//ShowWindow(hwndBookmarksList, SW_SHOW);
		redrawBookmarksList();
	}
	else
	{
		editMode = EDIT_MODE_BOTH;
		//ShowWindow(hwndBranchesBitmap, SW_HIDE);
		//ShowWindow(hwndBookmarksList, SW_SHOW);
		redrawBookmarksList();
	}
	if (prev_edit_mode != editMode)
	{
		mustCheckItemUnderMouse = true;
	}
	//SetWindowText(hwndBookmarks, bookmarksCaption[editMode]);
}
void BOOKMARKS::redrawBookmarksList(bool eraseBG)
{
	if (editMode != EDIT_MODE_BRANCHES)
	{
		//InvalidateRect(hwndBookmarksList, 0, eraseBG);
	}
	redrawFlag = true;
}
void BOOKMARKS::redrawChangedBookmarks(int frame)
{
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
	{
		if (bookmarksArray[i].snapshot.keyFrame == frame)
		{
			redrawBookmark(i);
		}
	}
}
void BOOKMARKS::redrawBookmark(int bookmarkNumber)
{
	redrawFlag = true;
}
void BOOKMARKS::redrawBookmarksListRow(int rowIndex)
{
	redrawFlag = true;
}

void BOOKMARKS::resizeEvent(QResizeEvent *event)
{
	viewWidth  = event->size().width();
	viewHeight = event->size().height();
}

void BOOKMARKS::paintEvent(QPaintEvent *event)
{
	FCEU_CRITICAL_SECTION( emuLock );
	QPainter painter(this);
	int x, y, item, cell_y;
	QColor white(255,255,255), black(0,0,0), blkColor;
	char txt[256];
	bool timeColBgDone = false;

	painter.setFont(font);
	//viewWidth  = event->rect().width();
	//viewHeight = event->rect().height();

	// Draw Background
	painter.fillRect( 0, 0, viewWidth, viewHeight, white );

	// Draw text
	y = 0;
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
	{
		cell_y = item = (i+1) % TOTAL_BOOKMARKS;

		timeColBgDone = taseditorConfig->oldControlSchemeForBranching && movie_readonly;

		if (bookmarksArray[cell_y].notEmpty)
		{
			// frame number
			//SelectObject(msg->nmcd.hdc, pianoRoll.hMainListFont);
			int frame = bookmarksArray[cell_y].snapshot.keyFrame;
			if (frame == currFrameCounter || frame == (playback->getFlashingPauseFrame() - 1))
			{
				// current frame
				blkColor = QColor( CUR_FRAMENUM_COLOR );
			}
			else if (frame < greenzone->getSize())
			{
				if (!greenzone->isSavestateEmpty(frame))
				{
					if (greenzone->lagLog.getLagInfoAtFrame(frame) == LAGGED_YES)
					{
						blkColor = QColor( LAG_FRAMENUM_COLOR );
					}
					else
					{
						blkColor = QColor( GREENZONE_FRAMENUM_COLOR );
					}
				}
				else if (!greenzone->isSavestateEmpty(cell_y & EVERY16TH)
					|| !greenzone->isSavestateEmpty(cell_y & EVERY8TH)
					|| !greenzone->isSavestateEmpty(cell_y & EVERY4TH)
					|| !greenzone->isSavestateEmpty(cell_y & EVERY2ND))
				{
					if (greenzone->lagLog.getLagInfoAtFrame(frame) == LAGGED_YES)
					{
						blkColor = QColor( PALE_LAG_FRAMENUM_COLOR );
					}
					else
					{
						blkColor = QColor( PALE_GREENZONE_FRAMENUM_COLOR );
					}
				}
				else
				{
					blkColor = QColor( NORMAL_FRAMENUM_COLOR );
				}
			}
			else
			{
				blkColor = QColor( NORMAL_FRAMENUM_COLOR );
			}
		}
		else
		{
			blkColor = QColor( NORMAL_BACKGROUND_COLOR );	// empty bookmark
		}
		painter.fillRect( pxStartCol2, y, pxWidthFrameCol, pxLineSpacing, blkColor );

		if (timeColBgDone)
		{
			painter.fillRect( pxStartCol3, y, pxWidthTimeCol, pxLineSpacing, blkColor );
		}
		else
		{
			if (bookmarksArray[cell_y].notEmpty)
			{
				// frame number
				//SelectObject(msg->nmcd.hdc, pianoRoll.hMainListFont);
				int frame = bookmarksArray[cell_y].snapshot.keyFrame;
				if (frame == currFrameCounter || frame == (playback->getFlashingPauseFrame() - 1))
				{
					// current frame
					blkColor = QColor( CUR_INPUT_COLOR1 );
				}
				else if (frame < greenzone->getSize())
				{
					if (!greenzone->isSavestateEmpty(frame))
					{
						if (greenzone->lagLog.getLagInfoAtFrame(frame) == LAGGED_YES)
						{
							blkColor = QColor( LAG_INPUT_COLOR1 );
						}
						else
						{
							blkColor = QColor( GREENZONE_INPUT_COLOR1 );
						}
					}
					else if (!greenzone->isSavestateEmpty(cell_y & EVERY16TH)
						|| !greenzone->isSavestateEmpty(cell_y & EVERY8TH)
						|| !greenzone->isSavestateEmpty(cell_y & EVERY4TH)
						|| !greenzone->isSavestateEmpty(cell_y & EVERY2ND))
					{
						if (greenzone->lagLog.getLagInfoAtFrame(frame) == LAGGED_YES)
						{
							blkColor = QColor( PALE_LAG_INPUT_COLOR1 );
						}
						else
						{
							blkColor = QColor( PALE_GREENZONE_INPUT_COLOR1 );
						}
					}
					else
					{
						blkColor = QColor( NORMAL_INPUT_COLOR1 );
					}
				}
				else
				{
					blkColor = QColor( NORMAL_INPUT_COLOR1 );
				}
			}
			else
			{
				blkColor = QColor( NORMAL_BACKGROUND_COLOR );	// empty bookmark
			}
			painter.fillRect( pxStartCol3, y, pxWidthTimeCol, pxLineSpacing, blkColor );
		}

		x = pxStartCol1 + pxCharWidth;
		sprintf( txt, "%i", item );

		painter.drawText( x, y+pxLineTextOfs, tr(txt) );

		if (bookmarksArray[item].notEmpty)
		{
			x = pxStartCol2 + pxCharWidth;

			U32ToDecStr( txt, bookmarksArray[item].snapshot.keyFrame, 7);

			painter.drawText( x, y+pxLineTextOfs, tr(txt) );

			x = pxStartCol3 + pxCharWidth;

			strcpy(txt, bookmarksArray[item].snapshot.description);

			painter.drawText( x, y+pxLineTextOfs, tr(txt) );
		}
		y += pxLineSpacing;
	}

	// Draw Grid
	painter.drawLine( pxStartCol1, 0, pxStartCol1, viewHeight );
	painter.drawLine( pxStartCol2, 0, pxStartCol2, viewHeight );
	painter.drawLine( pxStartCol3, 0, pxStartCol3, viewHeight );

	painter.drawLine( pxLineWidth-1, 0, pxLineWidth-1, viewHeight );

	y = 0;
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
	{
		painter.drawLine( 0, y, viewWidth, y );

		y += pxLineSpacing;
	}
	painter.drawLine( 0, viewHeight-1, viewWidth, viewHeight-1);
}

//----------------------------------------------------------------------------
QPoint BOOKMARKS::convPixToCursor( QPoint p )
{
	QPoint c(0,0);

	if ( p.x() < 0 )
	{
		c.setX(0);
	}
	else
	{
		float x = (float)(p.x()) / pxCharWidth;

		c.setX( (int)x );
	}

	if ( p.y() < 0 )
	{
		c.setY( -1 );
	}
	else 
	{
		float py = ( (float)p.y() ) /  (float)pxLineSpacing;

		c.setY( (int)py );
	}
	return c;
}
//----------------------------------------------------------------------------
int  BOOKMARKS::calcColumn( int px )
{
	int col = -1;

	if ( px < pxStartCol2 )
	{
		col = BOOKMARKSLIST_COLUMN_ICON;
	}
	else if ( px < pxStartCol3 )
	{
		col = BOOKMARKSLIST_COLUMN_FRAME;
	}
	else
	{
		col = BOOKMARKSLIST_COLUMN_TIME;
	}
	return col;
}

void BOOKMARKS::mousePressEvent(QMouseEvent * event)
{
	FCEU_CRITICAL_SECTION( emuLock );
	int item, row_under_mouse, item_valid;
	QPoint c = convPixToCursor( event->pos() );

	row_under_mouse = c.y();
	columnClicked = calcColumn( event->pos().x() );

	item = (row_under_mouse + 1) % TOTAL_BOOKMARKS;
	item_valid = (item >= 0) && (item < TOTAL_BOOKMARKS);

	if ( item_valid && (event->button() & Qt::LeftButton) )
	{
		bookmarkLeftclicked  = item;

		if ( (columnClicked <= BOOKMARKSLIST_COLUMN_FRAME) || (taseditorConfig->oldControlSchemeForBranching && movie_readonly))
		{
			bookmarksArray[bookmarkLeftclicked].flashType = FLASH_TYPE_JUMP;
		}
		else if ( (columnClicked == BOOKMARKSLIST_COLUMN_TIME) && (!taseditorConfig->oldControlSchemeForBranching || !movie_readonly))
		{
			bookmarksArray[bookmarkLeftclicked].flashType = FLASH_TYPE_DEPLOY;
		}
	}

	if ( item_valid && (event->button() & Qt::RightButton) )
	{
		bookmarkRightclicked  = item;
		bookmarksArray[bookmarkRightclicked].flashType = FLASH_TYPE_SET;
	}

	if ( event->button() & Qt::MiddleButton )
	{
		playback->handleMiddleButtonClick();
	}
	//printf("Mouse Button Pressed: 0x%x (%i,%i)\n", event->button(), c.x(), c.y() );
	
	redrawFlag = true;
}

void BOOKMARKS::mouseReleaseEvent(QMouseEvent * event)
{
	FCEU_CRITICAL_SECTION( emuLock );
	//QPoint c = convPixToCursor( event->pos() );

	//printf("Mouse Button Released: 0x%x (%i,%i)\n", event->button(), c.x(), c.y() );
	if ( event->button() & Qt::LeftButton )
	{
		if (bookmarkLeftclicked >= 0)
		{
			handleLeftClick();
		}
		bookmarkLeftclicked = ITEM_UNDER_MOUSE_NONE;
	}

	if ( event->button() & Qt::RightButton )
	{
		if (bookmarkRightclicked >= 0)
		{
			handleRightClick();
		}
		bookmarkRightclicked = ITEM_UNDER_MOUSE_NONE;
	}
	redrawFlag = true;
}

void BOOKMARKS::showImage(void)
{
	FCEU_CRITICAL_SECTION( emuLock ); 

	bool item_valid = (imageItem >= 0) && (imageItem < TOTAL_BOOKMARKS);

	if ( item_valid && (imageItem != bookmarkPreviewPopup::currentIndex()) )
	{
		bookmarkPreviewPopup *popup = bookmarkPreviewPopup::currentInstance();

		if ( popup == NULL )
		{
			popup = new bookmarkPreviewPopup(imageItem, this);

			connect( this, SIGNAL(imageIndexChanged(int)), popup, SLOT(imageIndexChanged(int)) );

			popup->show();
		}
		else
		{
			popup->reloadImage(imageItem);
		}
	}
}

void BOOKMARKS::mouseMoveEvent(QMouseEvent * event)
{
	FCEU_CRITICAL_SECTION( emuLock );
	int item, row_under_mouse, item_valid, column;

	QPoint c = convPixToCursor( event->pos() );

	//printf("Mouse Move: 0x%x (%i,%i)\n", event->button(), c.x(), c.y() );
	
	row_under_mouse = c.y();
	column = calcColumn( event->pos().x() );

	item = (row_under_mouse + 1) % TOTAL_BOOKMARKS;
	item_valid = (item >= 0) && (item < TOTAL_BOOKMARKS);

	if ( item_valid && (column == BOOKMARKSLIST_COLUMN_TIME) && bookmarks->bookmarksArray[item].notEmpty)
	{
		if ( item != imageItem )
		{
			emit imageIndexChanged(item);
		}
		imageItem = item;
		imageTimer->start();
		QToolTip::hideText();
	}
	else
	{
		item = -1;
		if ( item != imageItem )
		{
			emit imageIndexChanged(item);
		}
		imageItem = item;
		imageTimer->stop();
	}
}

void BOOKMARKS::focusOutEvent(QFocusEvent *event)
{
	int item = -1;
	if ( item != imageItem )
	{
		emit imageIndexChanged(item);
	}
	imageItem = item;
}

void BOOKMARKS::leaveEvent(QEvent *event)
{
	int item = -1;
	if ( item != imageItem )
	{
		emit imageIndexChanged(item);
	}
	imageItem = item;
}

bool BOOKMARKS::event(QEvent *event)
{
	if (event->type() == QEvent::ToolTip)
	{
		FCEU_CRITICAL_SECTION( emuLock );
		int item, row_under_mouse, item_valid, column;
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);

		QPoint c = convPixToCursor( helpEvent->pos() );

		row_under_mouse = c.y();
		column = calcColumn( helpEvent->pos().x() );

		item = (row_under_mouse + 1) % TOTAL_BOOKMARKS;
		item_valid = (item >= 0) && (item < TOTAL_BOOKMARKS);

		if ( item_valid && (column == BOOKMARKSLIST_COLUMN_TIME) && bookmarks->bookmarksArray[item].notEmpty)
		{
			//static_cast<bookmarkPreviewPopup*>(fceuCustomToolTipShow( helpEvent, new bookmarkPreviewPopup(item, this) ));
			//QToolTip::showText(helpEvent->globalPos(), tr(stmp), this );
			QToolTip::hideText();
			event->ignore();
		}
		else if ( taseditorConfig && taseditorConfig->tooltipsEnabled )
		{
			QToolTip::showText(helpEvent->globalPos(), tr("Right click = set Bookmark, Left click = jump to Bookmark or load Branch"), this );
		}
		else
		{
			QToolTip::hideText();
		}
		return true;
	}
	return QWidget::event(event);
}

void BOOKMARKS::handleMouseMove(int newX, int newY)
{
	mouseX = newX;
	mouseY = newY;
	mustCheckItemUnderMouse = true;
}
int BOOKMARKS::findItemUnderMouse()
{
	int item = ITEM_UNDER_MOUSE_NONE;
//	RECT wrect;
//	GetClientRect(hwndBookmarksList, &wrect);
//	if (mouseX >= listRowLeft && mouseX < wrect.right - wrect.left && mouseY >= listTopMargin && mouseY < wrect.bottom - wrect.top)
//	{
//		int row_under_mouse = (mouseY - listTopMargin) / listRowHeight;
//		if (row_under_mouse >= 0 && row_under_mouse < TOTAL_BOOKMARKS)
//			item = (row_under_mouse + 1) % TOTAL_BOOKMARKS;
//	}
	return item;
}

int BOOKMARKS::getSelectedSlot()
{
	return selectedSlot;
}
// ----------------------------------------------------------------------------------------
//void BOOKMARKS::getDispInfo(NMLVDISPINFO* nmlvDispInfo)
//{
//	LVITEM& item = nmlvDispInfo->item;
//	if (item.mask & LVIF_TEXT)
//	{
//		switch(item.iSubItem)
//		{
//			case BOOKMARKSLIST_COLUMN_ICON:
//			{
//				if ((item.iItem + 1) % TOTAL_BOOKMARKS == branches->getCurrentBranch())
//					item.iImage = ((item.iItem + 1) % TOTAL_BOOKMARKS) + TOTAL_BOOKMARKS;
//				else
//					item.iImage = (item.iItem + 1) % TOTAL_BOOKMARKS;
//				if (taseditorConfig->oldControlSchemeForBranching)
//				{
//					if ((item.iItem + 1) % TOTAL_BOOKMARKS == selectedSlot)
//						item.iImage += BOOKMARKS_BITMAPS_SELECTED;
//				}
//				break;
//			}
//			case BOOKMARKSLIST_COLUMN_FRAME:
//			{
//				if (bookmarksArray[(item.iItem + 1) % TOTAL_BOOKMARKS].notEmpty)
//					U32ToDecStr(item.pszText, bookmarksArray[(item.iItem + 1) % TOTAL_BOOKMARKS].snapshot.keyFrame, DIGITS_IN_FRAMENUM);
//				break;
//			}
//			case BOOKMARKSLIST_COLUMN_TIME:
//			{
//				if (bookmarksArray[(item.iItem + 1) % TOTAL_BOOKMARKS].notEmpty)
//					strcpy(item.pszText, bookmarksArray[(item.iItem + 1) % TOTAL_BOOKMARKS].snapshot.description);
//			}
//			break;
//		}
//	}
//}

//LONG BOOKMARKS::handleCustomDraw(NMLVCUSTOMDRAW* msg)
//{
//	int cell_x, cell_y;
//	switch(msg->nmcd.dwDrawStage)
//	{
//	case CDDS_PREPAINT:
//		return CDRF_NOTIFYITEMDRAW;
//	case CDDS_ITEMPREPAINT:
//		return CDRF_NOTIFYSUBITEMDRAW;
//	case CDDS_SUBITEMPREPAINT:
//		cell_x = msg->iSubItem;
//		cell_y = (msg->nmcd.dwItemSpec + 1) % TOTAL_BOOKMARKS;
//		
//		// flash with text color when needed
//		if (bookmarksArray[cell_y].flashPhase)
//			msg->clrText = bookmark_flash_colors[bookmarksArray[cell_y].flashType][bookmarksArray[cell_y].flashPhase];
//
//		if (cell_x == BOOKMARKSLIST_COLUMN_FRAME || (taseditorConfig->oldControlSchemeForBranching && movie_readonly && cell_x == BOOKMARKSLIST_COLUMN_TIME))
//		{
//			if (bookmarksArray[cell_y].notEmpty)
//			{
//				// frame number
//				SelectObject(msg->nmcd.hdc, pianoRoll.hMainListFont);
//				int frame = bookmarksArray[cell_y].snapshot.keyFrame;
//				if (frame == currFrameCounter || frame == (playback->getFlashingPauseFrame() - 1))
//				{
//					// current frame
//					msg->clrTextBk = CUR_FRAMENUM_COLOR;
//				} else if (frame < greenzone->getSize())
//				{
//					if (!greenzone->isSavestateEmpty(frame))
//					{
//						if (greenzone->lagLog.getLagInfoAtFrame(frame) == LAGGED_YES)
//							msg->clrTextBk = LAG_FRAMENUM_COLOR;
//						else
//							msg->clrTextBk = GREENZONE_FRAMENUM_COLOR;
//					} else if (!greenzone->isSavestateEmpty(cell_y & EVERY16TH)
//						|| !greenzone->isSavestateEmpty(cell_y & EVERY8TH)
//						|| !greenzone->isSavestateEmpty(cell_y & EVERY4TH)
//						|| !greenzone->isSavestateEmpty(cell_y & EVERY2ND))
//					{
//						if (greenzone->lagLog.getLagInfoAtFrame(frame) == LAGGED_YES)
//							msg->clrTextBk = PALE_LAG_FRAMENUM_COLOR;
//						else
//							msg->clrTextBk = PALE_GREENZONE_FRAMENUM_COLOR;
//					} else msg->clrTextBk = NORMAL_FRAMENUM_COLOR;
//				} else msg->clrTextBk = NORMAL_FRAMENUM_COLOR;
//			} else msg->clrTextBk = NORMAL_BACKGROUND_COLOR;	// empty bookmark
//		} else if (cell_x == BOOKMARKSLIST_COLUMN_TIME)
//		{
//			if (bookmarksArray[cell_y].notEmpty)
//			{
//				// frame number
//				SelectObject(msg->nmcd.hdc, pianoRoll.hMainListFont);
//				int frame = bookmarksArray[cell_y].snapshot.keyFrame;
//				if (frame == currFrameCounter || frame == (playback->getFlashingPauseFrame() - 1))
//				{
//					// current frame
//					msg->clrTextBk = CUR_INPUT_COLOR1;
//				} else if (frame < greenzone->getSize())
//				{
//					if (!greenzone->isSavestateEmpty(frame))
//					{
//						if (greenzone->lagLog.getLagInfoAtFrame(frame) == LAGGED_YES)
//							msg->clrTextBk = LAG_INPUT_COLOR1;
//						else
//							msg->clrTextBk = GREENZONE_INPUT_COLOR1;
//					} else if (!greenzone->isSavestateEmpty(cell_y & EVERY16TH)
//						|| !greenzone->isSavestateEmpty(cell_y & EVERY8TH)
//						|| !greenzone->isSavestateEmpty(cell_y & EVERY4TH)
//						|| !greenzone->isSavestateEmpty(cell_y & EVERY2ND))
//					{
//						if (greenzone->lagLog.getLagInfoAtFrame(frame) == LAGGED_YES)
//							msg->clrTextBk = PALE_LAG_INPUT_COLOR1;
//						else
//							msg->clrTextBk = PALE_GREENZONE_INPUT_COLOR1;
//					} else msg->clrTextBk = NORMAL_INPUT_COLOR1;
//				} else msg->clrTextBk = NORMAL_INPUT_COLOR1;
//			} else msg->clrTextBk = NORMAL_BACKGROUND_COLOR;		// empty bookmark
//		}
//	default:
//		return CDRF_DODEFAULT;
//	}
//}

void BOOKMARKS::handleLeftClick(void)
{
	if (columnClicked <= BOOKMARKSLIST_COLUMN_FRAME || (taseditorConfig->oldControlSchemeForBranching && movie_readonly))
	{
		command(COMMAND_JUMP, bookmarkLeftclicked);
	}
	else if (columnClicked == BOOKMARKSLIST_COLUMN_TIME && (!taseditorConfig->oldControlSchemeForBranching || !movie_readonly))
	{
		command(COMMAND_DEPLOY, bookmarkLeftclicked);
	}
}
void BOOKMARKS::handleRightClick(void)
{
	if (bookmarkRightclicked >= 0)
	{
		command(COMMAND_SET, bookmarkRightclicked);
	}
}

int BOOKMARKS::findBookmarkAtFrame(int frame)
{
	int cur_bookmark = branches->getCurrentBranch();
	if (cur_bookmark != ITEM_UNDER_MOUSE_CLOUD && bookmarksArray[cur_bookmark].snapshot.keyFrame == frame)
		return cur_bookmark + TOTAL_BOOKMARKS;	// blue digit has highest priority when drawing
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
	{
		cur_bookmark = (i + 1) % TOTAL_BOOKMARKS;
		if (bookmarksArray[cur_bookmark].notEmpty && bookmarksArray[cur_bookmark].snapshot.keyFrame == frame)
			return cur_bookmark;	// green digit
	}
	return -1;		// no Bookmarks at the frame
}
// ----------------------------------------------------------------------------------------
//LRESULT APIENTRY BookmarksListWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
//{
//	extern BOOKMARKS bookmarks;
//	switch(msg)
//	{
//		case WM_CHAR:
//		case WM_KEYDOWN:
//		case WM_KEYUP:
//		case WM_SETFOCUS:
//		case WM_KILLFOCUS:
//			return 0;
//		case WM_MOUSEMOVE:
//		{
//			if (!bookmarks.mouseOverBookmarksList)
//			{
//				bookmarks.mouseOverBookmarksList = true;
//				bookmarks.tmeList.hwndTrack = hWnd;
//				TrackMouseEvent(&bookmarks.tmeList);
//			}
//			bookmarks.handleMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
//			break;
//		}
//		case WM_MOUSELEAVE:
//		{
//			bookmarks.mouseOverBookmarksList = false;
//			bookmarks.handleMouseMove(-1, -1);
//			break;
//		}
//		case WM_LBUTTONDOWN:
//		case WM_LBUTTONDBLCLK:
//		{
//			if (GetFocus() != hWnd)
//				SetFocus(hWnd);
//			LVHITTESTINFO info;
//			info.pt.x = GET_X_LPARAM(lParam);
//			info.pt.y = GET_Y_LPARAM(lParam);
//			ListView_SubItemHitTest(hWnd, (LPARAM)&info);
//			if (info.iItem >= 0 && bookmarks.bookmarkRightclicked < 0)
//			{
//				bookmarks.bookmarkLeftclicked = (info.iItem + 1) % TOTAL_BOOKMARKS;
//				bookmarks.columnClicked = info.iSubItem;
//				if (bookmarks.columnClicked <= BOOKMARKSLIST_COLUMN_FRAME || (taseditorConfig->oldControlSchemeForBranching && movie_readonly))
//					bookmarks.bookmarksArray[bookmarks.bookmarkLeftclicked].flashType = FLASH_TYPE_JUMP;
//				else if (bookmarks.columnClicked == BOOKMARKSLIST_COLUMN_TIME && (!taseditorConfig->oldControlSchemeForBranching || !movie_readonly))
//					bookmarks.bookmarksArray[bookmarks.bookmarkLeftclicked].flashType = FLASH_TYPE_DEPLOY;
//				SetCapture(hWnd);
//			}
//			return 0;
//		}
//		case WM_LBUTTONUP:
//		{
//			LVHITTESTINFO info;
//			info.pt.x = GET_X_LPARAM(lParam);
//			info.pt.y = GET_Y_LPARAM(lParam);
//			RECT wrect;
//			GetClientRect(hWnd, &wrect);
//			if (info.pt.x >= 0 && info.pt.x < wrect.right - wrect.left && info.pt.y >= 0 && info.pt.y < wrect.bottom - wrect.top)
//			{
//				ListView_SubItemHitTest(hWnd, (LPARAM)&info);
//				if (bookmarks.bookmarkLeftclicked == (info.iItem + 1) % TOTAL_BOOKMARKS && bookmarks.columnClicked == info.iSubItem)
//					bookmarks.handleLeftClick();
//			}
//			ReleaseCapture();
//			bookmarks.bookmarkLeftclicked = -1;
//			return 0;
//		}
//		case WM_RBUTTONDOWN:
//		case WM_RBUTTONDBLCLK:
//		{
//			if (GetFocus() != hWnd)
//				SetFocus(hWnd);
//			LVHITTESTINFO info;
//			info.pt.x = GET_X_LPARAM(lParam);
//			info.pt.y = GET_Y_LPARAM(lParam);
//			ListView_SubItemHitTest(hWnd, (LPARAM)&info);
//			if (info.iItem >= 0 && bookmarks.bookmarkLeftclicked < 0)
//			{
//				bookmarks.bookmarkRightclicked = (info.iItem + 1) % TOTAL_BOOKMARKS;
//				bookmarks.columnClicked = info.iSubItem;
//				bookmarks.bookmarksArray[bookmarks.bookmarkRightclicked].flashType = FLASH_TYPE_SET;
//				SetCapture(hWnd);
//			}
//			return 0;
//		}
//		case WM_RBUTTONUP:
//		{
//			LVHITTESTINFO info;
//			info.pt.x = GET_X_LPARAM(lParam);
//			info.pt.y = GET_Y_LPARAM(lParam);
//			RECT wrect;
//			GetClientRect(hWnd, &wrect);
//			if (info.pt.x >= 0 && info.pt.x < wrect.right - wrect.left && info.pt.y >= 0 && info.pt.y < wrect.bottom - wrect.top)
//			{
//				ListView_SubItemHitTest(hWnd, (LPARAM)&info);
//				if (bookmarks.bookmarkRightclicked == (info.iItem + 1) % TOTAL_BOOKMARKS && bookmarks.columnClicked == info.iSubItem)
//					bookmarks.handleRightClick();
//			}
//			ReleaseCapture();
//			bookmarks.bookmarkRightclicked = ITEM_UNDER_MOUSE_NONE;
//			return 0;
//		}
//		case WM_MBUTTONDOWN:
//		case WM_MBUTTONDBLCLK:
//		{
//			if (GetFocus() != hWnd)
//				SetFocus(hWnd);
//			playback->handleMiddleButtonClick();
//			return 0;
//		}
//		case WM_MOUSEWHEEL:
//		{
//			bookmarks.bookmarkRightclicked = ITEM_UNDER_MOUSE_NONE;	// ensure that accidental rightclick on BookmarksList won't set Bookmarks when user does rightbutton + wheel
//			return SendMessage(pianoRoll.hwndList, msg, wParam, lParam);
//		}
//	}
//	return CallWindowProc(hwndBookmarksList_oldWndProc, hWnd, msg, wParam, lParam);
//}

