extern bool isFDS;
void FDSSoundReset(void);

void FCEU_FDSInsert(void);
//void FCEU_FDSEject(void);
void FCEU_FDSSelect(void);

// Lua-friendly accessors
bool FCEU_FDSIsInserted(void);
int  FCEU_FDSGetSelectedSide(void); // 0-based; -1 when no FDS loaded
