// CoverGate.h - COVER open/close logic

#pragma once

#include "Core/CoverGateDef.h"
void CoverGate_init(void);
void CoverGate_loop(void);
bool CoverGate_intercept(const char* label, bool pressed);