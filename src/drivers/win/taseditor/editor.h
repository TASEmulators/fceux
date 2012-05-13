// Specification file for EDITOR class

class EDITOR
{
public:
	EDITOR();
	void init();
	void free();
	void reset();
	void update();

	void InputToggle(int start, int end, int joy, int button, int consecutive_tag = 0);
	void InputSetPattern(int start, int end, int joy, int button, int consecutive_tag = 0);

	bool FrameColumnSet();
	bool FrameColumnSetPattern();
	bool InputColumnSet(int joy, int button);
	bool InputColumnSetPattern(int joy, int button);
	void SetMarkers();
	void RemoveMarkers();

	std::vector<std::string> autofire_patterns_names;
	std::vector<std::vector<uint8>> autofire_patterns;

private:
	bool ReadString(EMUFILE *is, std::string& dest);

};
