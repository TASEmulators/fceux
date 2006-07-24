#define LOG_OPTION_SIZE      10
#define LOG_REGISTERS         1
#define LOG_PROCESSOR_STATUS  2
#define LOG_NEW_INSTRUCTIONS  4
#define LOG_NEW_DATA          8

extern HWND hTracer;
extern int log_update_window;
extern volatile int logtofile, logging;
extern int logging_options;

void EnableTracerMenuItems(void);
void LogInstruction(void);
void DoTracer();
//void PauseLoggingSequence();
void UpdateLogWindow(void);
