// CoverGate.h - COVER open/close logic

#pragma once

#include "../Mappings.h"
void CoverGate_init(void);
void CoverGate_loop(void);
bool CoverGate_intercept(const char* label, bool pressed);