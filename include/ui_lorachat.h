// SquachWatch-CYD — LORA CHATS: what the LoRa listener decoded, one chat per
// network. Meshtastic's open channels in one tab, MeshCore's Public and
// hashtag channels in the other, newest at the top. Read only: the radio
// never transmits (include/lora_sniffer.h). Built only with SQUACH_LORA.
#pragma once
#include <TFT_eSPI.h>
#include <stdint.h>

enum class LoraChatHit : uint8_t { NONE = 0, TAB_MESHTASTIC, TAB_MESHCORE };

void        uiLoraChatInit(TFT_eSPI& t);
void        uiLoraChatTick(TFT_eSPI& t, uint32_t now);
void        uiLoraChatScroll(int delta);   // positive = older
LoraChatHit uiLoraChatHit(TFT_eSPI& t, int x, int y);
void        uiLoraChatSelect(uint8_t tab); // 0 Meshtastic, 1 MeshCore

// Messages decoded since the chats were last open, both networks together,
// for the LORA CHATS row.
uint16_t    uiLoraChatUnread();
