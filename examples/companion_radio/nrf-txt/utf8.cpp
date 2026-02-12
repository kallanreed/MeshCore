#include "utf8.h"

namespace {
bool inRange(uint32_t cp, uint32_t lo, uint32_t hi) {
  return (cp >= lo) && (cp <= hi);
}

bool isIgnoredComponent(uint32_t cp) {
  if (cp == 0xFE0E || cp == 0xFE0F) return true;    // VS15/VS16
  if (cp == 0x200D) return true;                    // ZWJ
  if (inRange(cp, 0x1F3FB, 0x1F3FF)) return true;   // skin tones
  if (cp == 0x20E3) return true;                    // keycap mark
  return false;
}

bool isFaceCodepoint(uint32_t cp) {
  if (inRange(cp, 0x1F600, 0x1F64F)) return true;
  if (inRange(cp, 0x1F910, 0x1F92F)) return true;
  if (inRange(cp, 0x1F970, 0x1F97F)) return true;
  if (inRange(cp, 0x1FAE0, 0x1FAE8)) return true;
  if (cp == 0x263A || cp == 0x2639) return true;
  return false;
}

EmojiSlot classifyFace(uint32_t cp) {
  // LOVE / affection faces
  switch (cp) {
    case 0x1F60D: case 0x1F970: case 0x1F618: case 0x1F617: case 0x1F619: case 0x1F61A:
    case 0x1F63B:
      return EmojiSlot::Love;
    default: break;
  }

  // LAUGH / amused
  switch (cp) {
    case 0x1F602: case 0x1F923: case 0x1F606: case 0x1F603: case 0x1F604: case 0x1F605:
    case 0x1F642: case 0x1F61B: case 0x1F61C: case 0x1F61D: case 0x1F92A: case 0x1F92D:
      return EmojiSlot::Laugh;
    default: break;
  }

  // ANGRY
  switch (cp) {
    case 0x1F620: case 0x1F621: case 0x1F92C: case 0x1F624: case 0x1F63E: case 0x1F47F:
      return EmojiSlot::Angry;
    default: break;
  }

  // SAD
  switch (cp) {
    case 0x1F622: case 0x1F62D: case 0x1F625: case 0x1F614: case 0x1F61E: case 0x1F613:
    case 0x1F62A: case 0x1F629: case 0x1F97A: case 0x2639:  case 0x1F63F:
      return EmojiSlot::Sad;
    default: break;
  }

  // SURPRISE / shock
  switch (cp) {
    case 0x1F62E: case 0x1F62F: case 0x1F632: case 0x1F631: case 0x1F633: case 0x1FAE2: case 0x1FAE3:
      return EmojiSlot::Surprise;
    default: break;
  }

  // THINKING
  switch (cp) {
    case 0x1F914: case 0x1F9D0:
      return EmojiSlot::Thinking;
    default: break;
  }

  // COOL
  if (cp == 0x1F60E) return EmojiSlot::Cool;

  // UNAMUSED / neutral / meh / eye-roll
  switch (cp) {
    case 0x1F610: case 0x1F611: case 0x1F612: case 0x1F62C: case 0x1F636: case 0x1FAE5:
    case 0x1FAE4: case 0x1F928: case 0x1F60F: case 0x1F644:
      return EmojiSlot::Unamused;
    default: break;
  }

  // HAPPY (explicit positives)
  switch (cp) {
    case 0x1F600: case 0x1F601: case 0x1F60A: case 0x1F607: case 0x263A: case 0x1F609:
    case 0x1F60C: case 0x1F929: case 0x1F973:
      return EmojiSlot::Happy;
    default: break;
  }

  // Any other face: bias to UNAMUSED rather than UNKNOWN.
  return EmojiSlot::Unamused;
}

}  // namespace

uint32_t decodeUtf8(const char* src, size_t* advance) {
  uint8_t c0 = static_cast<uint8_t>(src[0]);
  if (c0 < 0x80) {
    *advance = 1;
    return c0;
  }

  if ((c0 & 0xE0) == 0xC0) {
    uint8_t c1 = static_cast<uint8_t>(src[1]);
    if (c1 != 0 && (c1 & 0xC0) == 0x80) {
      *advance = 2;
      return (static_cast<uint32_t>(c0 & 0x1F) << 6) | static_cast<uint32_t>(c1 & 0x3F);
    }
  } else if ((c0 & 0xF0) == 0xE0) {
    uint8_t c1 = static_cast<uint8_t>(src[1]);
    uint8_t c2 = static_cast<uint8_t>(src[2]);
    if (c1 != 0 && c2 != 0 && (c1 & 0xC0) == 0x80 && (c2 & 0xC0) == 0x80) {
      *advance = 3;
      return (static_cast<uint32_t>(c0 & 0x0F) << 12) |
             (static_cast<uint32_t>(c1 & 0x3F) << 6) |
             static_cast<uint32_t>(c2 & 0x3F);
    }
  } else if ((c0 & 0xF8) == 0xF0) {
    uint8_t c1 = static_cast<uint8_t>(src[1]);
    uint8_t c2 = static_cast<uint8_t>(src[2]);
    uint8_t c3 = static_cast<uint8_t>(src[3]);
    if (c1 != 0 && c2 != 0 && c3 != 0 &&
        (c1 & 0xC0) == 0x80 && (c2 & 0xC0) == 0x80 && (c3 & 0xC0) == 0x80) {
      *advance = 4;
      return (static_cast<uint32_t>(c0 & 0x07) << 18) |
             (static_cast<uint32_t>(c1 & 0x3F) << 12) |
             (static_cast<uint32_t>(c2 & 0x3F) << 6) |
             static_cast<uint32_t>(c3 & 0x3F);
    }
  }

  *advance = 1;
  return kInvalidCodepoint;
}

bool isIgnoredCodepoint(uint32_t cp) {
  return isIgnoredComponent(cp);
}

bool mapToAscii(uint32_t cp, char* ascii_out) {
  switch (cp) {
    case 0x2018:
    case 0x2019:
      *ascii_out = '\'';
      return true;
    case 0x201C:
    case 0x201D:
      *ascii_out = '"';
      return true;
    case 0x2013:
    case 0x2014:
    case 0x2212:
      *ascii_out = '-';
      return true;
    case 0x2026:
      *ascii_out = '.';
      return true;
    case 0x00A0:
      *ascii_out = ' ';
      return true;
    default:
      return false;
  }
}

EmojiSlot emojiSlotFromCodepoint(uint32_t cp) {
  if (isIgnoredComponent(cp)) return EmojiSlot::Unknown;

  // SPOOKY (wins early)
  if (cp == 0x1F47B || cp == 0x1F383 || cp == 0x1F52E || cp == 0x2620 ||
      cp == 0x1F480 || cp == 0x1F578 || cp == 0x1F577 ||
      inRange(cp, 0x1F9D9, 0x1F9DF)) { // 🧙..🧟
    return EmojiSlot::Spooky;
  }

  // ANIMALS
  if (cp == 0x1F986) {
    return EmojiSlot::Duck;
  }

  if (cp == 0x1F410) {
    return EmojiSlot::Animals;
  }

  if (inRange(cp, 0x1F400, 0x1F43F) ||
      inRange(cp, 0x1F980, 0x1F997) ||
      inRange(cp, 0x1F9A0, 0x1F9BF) ||
      cp == 0x1F54A) {
    return EmojiSlot::Animals;
  }

  // FACES (comprehensive)
  if (isFaceCodepoint(cp)) return classifyFace(cp);

  // YES / NO (plus a couple common gesture-ish codepoints)
  if (cp == 0x1F44D || cp == 0x2705 || cp == 0x2611 || cp == 0x1F91D) return EmojiSlot::ApprovalYes;
  if (cp == 0x1F44E || cp == 0x274C || cp == 0x1F6AB || cp == 0x26D4 || cp == 0x1F645) return EmojiSlot::DisapprovalNo;

  // WARNING (catch-all for “danger” symbols)
  if (cp == 0x26A0 || cp == 0x1F525 || cp == 0x1F4A5 || cp == 0x1F6A8 || cp == 0x1F6D1) return EmojiSlot::Warning;

  // LOCATION / DIRECTION
  if (inRange(cp, 0x1F680, 0x1F6FF) ||       // transport/map
      cp == 0x1F4CD || cp == 0x1F9ED ||       // 📍 🧭
      cp == 0x1F5FA || cp == 0x1F30D ||       // 🗺 🌍
      cp == 0x1F3D4) {                        // 🏔
    return EmojiSlot::LocationDirection;
  }

  // PEOPLE (aggressive; SPOOKY already excluded above)
  if (inRange(cp, 0x1F466, 0x1F487) ||       // classic people
      inRange(cp, 0x1F9D1, 0x1F9FF) ||       // newer people/body parts/accessories
      inRange(cp, 0x1F440, 0x1F450) ||       // 👀..👐 (eyes/hands)
      inRange(cp, 0x1F590, 0x1F596) ||       // 🖐..🖖
      cp == 0x1F574 || cp == 0x1F57A || cp == 0x1F575 || cp == 0x1F482) {
    return EmojiSlot::PeopleCommunity;
  }

  // SAFETY / SECURITY (keys, locks, shields, IDs, etc.)
  if (inRange(cp, 0x1F510, 0x1F513) ||       // 🔐🔑🔒🔓
      cp == 0x1F5DD ||                        // 🗝 old key
      cp == 0x1F6E1 ||                        // 🛡 shield
      cp == 0x1F4B3 ||                        // 💳 credit card (often “account/security”)
      cp == 0x1F4DB ||                        // 📛 name badge
      cp == 0x1FAA4) {                        // 🪤 trap (arguably “danger/safety”; pick safety)
    return EmojiSlot::SafetySecurity;
  }

  // WORK / TOOLS (be generous)
  if (cp == 0x1F9F9 ||                        // 🧹 broom
      inRange(cp, 0x1F527, 0x1F52B) ||       // 🔧🔨🔩🔪🔫(older pistol is 1F52B) etc.
      cp == 0x2692 || cp == 0x26CF ||         // ⚒ ⛏
      cp == 0x1F6E0 ||                        // 🛠
      cp == 0x1F9F0 ||                        // 🧰 toolbox
      cp == 0x1F9F2 ||                        // 🧲 magnet
      cp == 0x1FA93 ||                        // 🪓 axe
      cp == 0x1FA9A ||                        // 🪚 saw
      cp == 0x1FA9B ||                        // 🪛 screwdriver
      cp == 0x1FA9C ||                        // 🪜 ladder
      cp == 0x1F9F1 ||                        // 🧱 bricks
      cp == 0x1F6A7) {                        // 🚧 barrier (often “work zone”)
    return EmojiSlot::WorkTools;
  }

  // TECH / SYSTEM (also generous)
  if (cp == 0x2699 ||                         // ⚙
      inRange(cp, 0x1F4BB, 0x1F4BF) ||       // 💻..📿 (mixed, but tech cluster includes 💻💽💾📀)
      cp == 0x1F50C || cp == 0x1F50B ||       // 🔌 🔋
      cp == 0x1F4E1 || cp == 0x1F4F6 ||       // 📡 📶
      cp == 0x1F916 ||                        // 🤖
      cp == 0x1F5A5) {                        // 🖥
    return EmojiSlot::TechSystem;
  }

  // COMMUNICATION (mail/phone/speaking; broad object range)
  if (inRange(cp, 0x1F4E0, 0x1F4EF) ||       // 📠..📯 (phones/mail/megaphone)
      cp == 0x1F4AC || cp == 0x1F5E8 ||       // 💬 🗨
      cp == 0x1F4F1 || cp == 0x1F4F2 ||       // 📱 📲
      cp == 0x1F399 || cp == 0x1F3A4) {       // 🎙 🎤
    return EmojiSlot::Communication;
  }

  // MONEY / VALUE (cash, receipts, charts that “read money”)
  if (inRange(cp, 0x1F4B0, 0x1F4BF) ||       // 💰..💿 (includes 💳; ok)
      cp == 0x1FA99 ||                        // 🪙
      cp == 0x1F911 ||                        // 🤑
      cp == 0x1F4C8 || cp == 0x1F4C9) {       // 📈 📉
    return EmojiSlot::MoneyValue;
  }

  // CELEBRATION (include sports + activities so UI stays fun)
  // Sports & activities live heavily in 1F3A0..1F3FF, plus newer sports 1F93A..1F94F, plus some 26xx.
  if (inRange(cp, 0x1F3A0, 0x1F3FF) ||       // 🎠..🏿 (activities, awards, sports, events)
      inRange(cp, 0x1F93A, 0x1F94F) ||       // 🤺..🥏 (sports / competition / medals-ish)
      cp == 0x26BD || cp == 0x26BE ||         // ⚽ ⚾
      cp == 0x1F3C6 || cp == 0x1F3C5 ||       // 🏆 🏅
      cp == 0x1F389 || cp == 0x1F38A ||       // 🎉 🎊
      cp == 0x1F386 || cp == 0x1F387 ||       // 🎆 🎇
      cp == 0x1F942 || cp == 0x1F37E) {       // 🥂 🍾
    return EmojiSlot::Celebration;
  }

  // NATURE / ENV (weather, plants, water, etc.)
  if (inRange(cp, 0x1F300, 0x1F32F) ||       // 🌀..🌯 (weather-ish range; includes some food—acceptable)
      inRange(cp, 0x1F330, 0x1F34F) ||       // 🌰..🍏 (plants/fruit—acceptable for “nature”)
      inRange(cp, 0x2600, 0x26FF)) {         // ☀..⛿ (symbols; lots of weather)
    return EmojiSlot::NatureEnv;
  }

  // TIME / CONTROL (arrows/controls/clocks)
  if (cp == 0x23F3 || cp == 0x231B || cp == 0x23F0 ||
      cp == 0x25B6 || cp == 0x23F8 || cp == 0x23F9 || cp == 0x23FA ||
      cp == 0x27A1 || cp == 0x2B05 || cp == 0x2B06 || cp == 0x2B07) {
    return EmojiSlot::TimeControl;
  }

  // IDEA
  if (cp == 0x1F4A1 || cp == 0x2728 || cp == 0x1F4AD) return EmojiSlot::Idea;


  // QUESTION / INFO umbrella
  if (cp == 0x2753 || cp == 0x2754 || cp == 0x2139 || cp == 0x1F4DD) return EmojiSlot::Question;

  // SUCCESS (non-sports specific)
  if (cp == 0x2B50 || cp == 0x1F31F || cp == 0x1F4AF) return EmojiSlot::Success;

  // LOVE symbols (non-face)
  if (cp == 0x2764 || inRange(cp, 0x1F493, 0x1F49F) || cp == 0x1F48C) return EmojiSlot::Love;

  // If it’s an emoji-ish pictograph and we got here, bias to something non-UNKNOWN:
  // Most remaining symbols read like “info/intent”.
  if (inRange(cp, 0x1F300, 0x1FAFF)) return EmojiSlot::Question;

  return EmojiSlot::Unknown;
}

void translateUTF8ToBlocks(char* dest, const char* src, size_t dest_size) {
  if (!dest || dest_size == 0)
    return;

  size_t j = 0;
  for (size_t i = 0; src && src[i] != 0 && j < dest_size - 1;) {
    uint8_t c0 = static_cast<uint8_t>(src[i]);
    if (c0 < 0x80) {
      dest[j++] = static_cast<char>(c0);
      i++;
      continue;
    }

    size_t advance = 0;
    uint32_t code = decodeUtf8(&src[i], &advance);
    if (code == kInvalidCodepoint) {
      dest[j++] = static_cast<char>(EmojiSlot::Unknown);
      i++;
      continue;
    }

    i += advance;
    if (isIgnoredCodepoint(code))
      continue;

    char ascii_char = 0;
    if (mapToAscii(code, &ascii_char)) {
      dest[j++] = ascii_char;
    } else if (code >= 0xA1 && code <= 0xFF) {
      dest[j++] = static_cast<char>(code);
    } else {
      dest[j++] = static_cast<char>(emojiSlotFromCodepoint(code));
    }
  }

  dest[j] = 0;
}
