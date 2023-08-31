// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"
//#include "../bots/bot_includes.h"



void DummyFunc(void)
{
    cvar_t *example;
    
    if(example->integer) {
        gi.Com_PrintFmt("The example value is set to %i. That's great!\n", example->integer);
    } else {
        gi.Com_PrintFmt("The example value is set to %i. That's even better!\n", example->integer);
    }
}
