#pragma once
#include "NES.hpp"

/*
loop:
Game loop occurs here until some game terminating event occurs like an execution error or user quit
*/
void loop(NES, const char *);
