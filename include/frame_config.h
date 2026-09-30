#pragma once
// Small CYDs need room for WiFi, BLE and FATFS concurrently. Preserve RGB332
// and redraw in two bands instead of keeping 76,800 pixels resident.
#ifndef SQW_BANDED_FRAME
#if defined(DNSP_RUNTIME_DISPLAY) || defined(CYD35)
#define SQW_BANDED_FRAME 1
#else
#define SQW_BANDED_FRAME 0
#endif
#endif
