#pragma once

#include "pluginmain.h"

#define PLUGIN_NAME "Force Page Protection"
#define PLUGIN_VERSION 1

#define PLOG(Format, ...) _plugin_logprintf(Format, __VA_ARGS__)

void PluginLog(const char* Format, ...);

bool pluginInit(PLUG_INITSTRUCT* initStruct);
bool pluginStop();
void pluginSetup();
