// Header file for Main TAS Editor file

struct NewProjectParameters
{
	int input_type;
	bool copy_current_input;
	bool copy_current_markers;
	std::wstring author_name;
};

bool EnterTasEditor();
bool ExitTasEditor();
void UpdateTasEditor();

void NewProject();
void OpenProject();
bool LoadProject(const char* fullname);
bool SaveProject();
bool SaveProjectAs();
void SaveCompact();
bool AskSaveProject();

void Import();
void Export();

int GetInputType(MovieData& md);
void SetInputType(MovieData& md, int new_input_type);

void ApplyMovieInputConfig();

bool TaseditorIsRecording();
void Taseditor_RecordInput();

void Taseditor_EMUCMD(int command);
void SetTaseditorInput();
void ClearTaseditorInput();

