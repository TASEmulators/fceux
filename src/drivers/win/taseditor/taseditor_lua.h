//Specification file for TASEDITOR_LUA class

class TASEDITOR_LUA
{
public:
	TASEDITOR_LUA();
	void init();
	void reset();
	void update();

	void EnableRunFunction();
	void DisableRunFunction();

	// Taseditor Lua library
	bool engaged();
	bool markedframe(int frame);
	int getmarker(int frame);
	int setmarker(int frame);
	void clearmarker(int frame);
	const char* getnote(int index);
	void setnote(int index, const char* newtext);
	int getcurrentbranch();
	const char* getrecordermode();
	int getplaybacktarget();
	void setplayback(int frame);
	void stopseeking();


private:
	HWND hwndRunFunction;

};
