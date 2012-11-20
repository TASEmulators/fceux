// Specification file for Playback class

#define PROGRESSBAR_WIDTH 200

#define PAUSEFRAME_BLINKING_PERIOD_WHEN_SEEKING 100
#define PAUSEFRAME_BLINKING_PERIOD_WHEN_PAUSED 250

#define HOLD_REPEAT_DELAY 250			// in milliseconds


class PLAYBACK
{
public:
	PLAYBACK();
	void init();
	void reset();
	void update();

	void EnsurePlaybackIsInsideGreenzone(bool execute_lua = true, bool follow_cursor = true);
	void jump(int frame, bool force_reload = false, bool execute_lua = true, bool follow_cursor = true);

	void UpdateProgressbar();

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

	void SetLostPosition(int frame);
	int GetLostPosition();		// actually returns lost_position_frame-1

	int GetPauseFrame();
	int GetFlashingPauseFrame();

	void SetProgressbar(int a, int b);
	void CancelSeeking();

	bool must_find_current_marker;
	int shown_marker;

	HWND hwndProgressbar, hwndRewind, hwndForward, hwndRewindFull, hwndForwardFull;
	HWND hwndPlaybackMarker, hwndPlaybackMarkerEdit;

private:
	bool SetPlaybackAboveOrToFrame(int frame, bool force_reload = false);

	int pause_frame;
	int lost_position_frame;
	bool lost_position_is_stable;	// for when Greenzone invalidates several times, but the end of current segment must remain the same

	bool must_autopause_at_the_end;
	bool old_emu_paused, emu_paused;
	int old_pauseframe;
	bool old_show_pauseframe, show_pauseframe;
	int lastCursor;		// but for currentCursor we use external variable currFrameCounter

	bool old_rewind_button_state, rewind_button_state;
	bool old_forward_button_state, forward_button_state;
	bool old_rewind_full_button_state, rewind_full_button_state;
	bool old_forward_full_button_state, forward_full_button_state;
	int button_hold_time;
	int seeking_start_frame;

};
