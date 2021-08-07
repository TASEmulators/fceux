// throttle.h
int SpeedThrottle(void);
void RefreshThrottleFPS(void);
int getTimingMode(void);
int setTimingMode(int mode);


struct frameTimingStat_t
{

	struct {
		double tgt;
		double cur;
		double min;
		double max;
	} frameTimeAbs;

	struct {
		double tgt;
		double cur;
		double min;
		double max;
	} frameTimeDel;

	struct {
		double tgt;
		double cur;
		double min;
		double max;
	} frameTimeWork;

	struct {
		double tgt;
		double cur;
		double min;
		double max;
	} frameTimeIdle;

	struct {
		double tgt;
		double cur;
		double min;
		double max;
	} videoTimeDel;

	unsigned int lateCount;

	bool enabled;
};

void resetFrameTiming(void);
void setFrameTimingEnable( bool enable );
int  getFrameTimingStats( struct frameTimingStat_t *stats );
void videoBufferSwapMark(void);
double getHighPrecTimeStamp(void);
double getFrameRate(void);
double getFrameRateAdjustmentRatio(void);
double getBaseFrameRate(void);

extern bool useIntFrameRate;
