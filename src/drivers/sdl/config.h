#ifndef CONFIG_H_HF128
#define CONFIG_H_HF128

#include "../common/configSys.h"

Config *InitConfig(void);
void UpdateEMUCore(Config *);
int LoadCPalette(const std::string &file);

#endif
