#include "px_debug.h"

int debug_enabled = 0;

int isDebugEnabled(){
	return debug_enabled;
}

void enable_debug(){
	debug_enabled = 1;
}

	
void disable_debug(){
	debug_enabled = 0;
}
