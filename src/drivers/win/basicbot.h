#include "stdbool.h"
void UpdateBasicBot();
void CreateBasicBot();
extern char *BasicBotDir;
static int EvaluateFormula(char * formula, char **nextposition, bool ret);
static int ParseFormula(bool ret);
static void StopBasicBot();
static void StartBasicBot();
void debug(int n);
