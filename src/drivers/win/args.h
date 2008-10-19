extern char* MovieToLoad;	//Contains the filename of the savestate specified in the command line argument
extern char* StateToLoad;	//Contains the filename of the movie file specified in the command line argument
extern char* ConfigToLoad;	//Conatins the filename of the config file specified in the command line argument
extern bool replayReadOnlySetting;
extern int replayStopFrameSetting;
extern int PauseAfterLoad;
char *ParseArgies(int argc, char *argv[]);
