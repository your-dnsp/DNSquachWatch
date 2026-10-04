#pragma once
constexpr int ESP_OK=0;
inline int ff_diskio_get_drive(uint8_t* drive){*drive=0;return ESP_OK;}
