#pragma once

#include <cstddef>
#include <cstdint>

enum class EmojiSlot : uint8_t {
  Unknown = 0x80,

  Happy = 0x81,
  Laugh = 0x82,
  Love = 0x83,
  Sad = 0x84,
  Angry = 0x85,
  Surprise = 0x86,
  Confused = 0x87,
  Thinking = 0x88,
  Cool = 0x89,
  Unamused = 0x8A,

  ApprovalYes = 0x8B,
  DisapprovalNo = 0x8C,
  Warning = 0x8D,  // “error/failure-ish”
  Success = 0x8E,
  TimeControl = 0x8F,  // arrows/play/pause/clock-ish

  Question = 0x90,  // “info/help”
  Communication = 0x91,
  Idea = 0x92,
  WorkTools = 0x93,
  TechSystem = 0x94,
  LocationDirection = 0x95,
  PeopleCommunity = 0x96,
  Celebration = 0x97,  // sports/activities/events
  MoneyValue = 0x98,
  SafetySecurity = 0x99,
  NatureEnv = 0x9A,
  Duck = 0x9B,
  Animals = 0x9C,
  Spooky = 0x9D,

  Extra1 = 0x9E,
  Extra2 = 0x9F,
  Extra3 = 0xA0,
};

constexpr uint32_t kInvalidCodepoint = 0xFFFFFFFF;

uint32_t decodeUtf8(const char* src, size_t* advance);
bool isIgnoredCodepoint(uint32_t cp);
bool mapToAscii(uint32_t cp, char* ascii_out);
EmojiSlot emojiSlotFromCodepoint(uint32_t cp);
void translateUTF8ToBlocks(char* dest, const char* src, size_t dest_size);
