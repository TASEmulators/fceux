//Specification file for Bookmark class

#define FLASH_PHASE_MAX 11
#define FLASH_TYPE_SET 0
#define FLASH_TYPE_JUMP 1
#define FLASH_TYPE_UNLEASH 2

#define SCREENSHOT_WIDTH 256
#define SCREENSHOT_HEIGHT 240
#define SCREENSHOT_SIZE SCREENSHOT_WIDTH * SCREENSHOT_HEIGHT

#define PARENT_RELATION_SAME 0			// Yellow-blue: this child has absolutely the same input as its parent, although maybe different jump_frame. Input sizes must be equal too.
#define PARENT_RELATION_DIRECT 1		// Green-blue: the child has the same input at least until parent's jump_frame. Child's jump_frame can be higher or lower than parent's jump_frame (doesn't matter), but if child's input is shorter than parent's input, it is considered different if parent has non-null input after child's input ends.
#define PARENT_RELATION_ORPHAN 2		// Red-blue: child has different input before parent's jump_frame, so their inheritance may be illogical.
#define PARENT_RELATION_ATTACHED 3		// Blue: normal parent was lost due to different input before its jump_frame, so new parent was found. Essentially this is the same as PARENT_RELATION_DIRECT, but this also adds small notification that parent was inherited by autofinding system rather that by normal chain of user-made events (this notification may be useful for something).

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
	int parent_relation;

private:

};
