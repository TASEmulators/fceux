//Specification file for Playback class

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
	void restorePosition();

	void updateProgressbar();

	void SeekingStart(int finish_frame);
	void SeekingStop();
	void ToggleEmulationPause();
	void PauseEmulation();
	void UnpauseEmulation();

	void RewindFrame();
	void ForwardFrame();
	void RewindFull();
	void ForwardFull();


	void StartFromZero();

	int GetPauseFrame();
	void SetProgressbar(int a, int b);

	int pauseframe;

private:
	bool JumpToFrame(int index);

	int lastCursor;		// for currentCursor we use external variable currFrameCounter
	bool old_emu_paused, emu_paused;
	int old_pauseframe;
	bool old_show_pauseframe, show_pauseframe;
	bool old_rewind_button_state, rewind_button_state;
	bool old_forward_button_state, forward_button_state;
	int button_hold_time;
	int seeking_start_frame;

};
