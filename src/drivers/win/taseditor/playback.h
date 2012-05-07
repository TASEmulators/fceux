// Specification file for Playback class

#define PROGRESSBAR_WIDTH 200

#define PAUSEFRAME_BLINKING_PERIOD_SEEKING 100
#define PAUSEFRAME_BLINKING_PERIOD_PAUSED 250

#define HOLD_REPEAT_DELAY 250			// in milliseconds


class PLAYBACK
{
public:
	PLAYBACK();
	void init();
	void reset();
	void update();

	void jump(int frame);

	void updateProgressbar();

	void SeekingStart(int finish_frame);
	void SeekingStop();
	void ToggleEmulationPause();
	void PauseEmulation();
	void UnpauseEmulation();
	void RestorePosition();
	void MiddleButtonClick();

	void RewindFrame();
	void ForwardFrame();
	void RewindFull(int speed = 1);
	void ForwardFull(int speed = 1);

	void RedrawMarker();

	void StartFromZero();

	int GetFlashingPauseFrame();
	void SetProgressbar(int a, int b);
	void CancelSeeking();

	bool JumpToFrame(int index);

	int lastCursor;		// but for currentCursor we use external variable currFrameCounter
	int lost_position_frame;
	bool lost_position_must_be_fixed;	// when Greenzone invalidates several times, but the end of current segment must remain the same
	int pause_frame;
	bool must_find_current_marker;
	int shown_marker;

	HWND hwndProgressbar, hwndRewind, hwndForward, hwndRewindFull, hwndForwardFull;
	HWND hwndPlaybackMarker, hwndPlaybackMarkerEdit;

private:
	bool autopause_at_the_end;
	bool old_emu_paused, emu_paused;
	int old_pauseframe;
	bool old_show_pauseframe, show_pauseframe;
	bool old_rewind_button_state, rewind_button_state;
	bool old_forward_button_state, forward_button_state;
	bool old_rewind_full_button_state, rewind_full_button_state;
	bool old_forward_full_button_state, forward_full_button_state;
	int button_hold_time;
	int seeking_start_frame;

};
