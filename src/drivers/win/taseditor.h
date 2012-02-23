// Header file for Main TAS Editor file

struct NewProjectParameters
{
	int input_type;
	bool copy_current_input;
	bool copy_current_markers;
	std::wstring author_name;
};

bool EnterTasEditor();
bool ReadString(EMUFILE *is, std::string& dest);
bool ExitTasEditor();
void UpdateTasEditor();

void NewProject();
void OpenProject();
bool LoadProject(char* fullname);
bool SaveProject();
bool SaveProjectAs();
void SaveCompact();
bool AskSaveProject();

void Import();
void Export();

int GetInputType(MovieData& md);
void SetInputType(MovieData& md, int new_input_type);

bool TaseditorIsRecording();

