//Specification file for Bookmark class

#define FLASH_PHASE_MAX 11
#define FLASH_TYPE_SET 0
#define FLASH_TYPE_JUMP 1
#define FLASH_TYPE_UNLEASH 2

#define SCREENSHOT_WIDTH 256
#define SCREENSHOT_HEIGHT 240
#define SCREENSHOT_SIZE SCREENSHOT_WIDTH * SCREENSHOT_HEIGHT

class BOOKMARK
{
public:
	BOOKMARK();
	void init();

	void set();
	void jump();
	void unleashed();

	void save(EMUFILE *os);
	bool load(EMUFILE *is);

	bool not_empty;
	int flash_phase;
	int flash_type;
	INPUT_SNAPSHOT snapshot;
	std::vector<uint8> savestate;
	std::vector<uint8> saved_screenshot;
	int parent_branch;

private:

};
