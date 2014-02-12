// FIXME for Windows, make use of Config class the way linux version does
// old config registers variables by placing their addresses into static array, which makes
// many modules expose their internal vars and pollute globals; there is also no way to know when a
// setting was changed by it, which spawns auxillary functions intended to push new values through
void SaveConfig(const char *filename);
void LoadConfig(const char *filename);

extern int InputType[3];
