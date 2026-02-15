#include "utf8.h"
#include <cstring>

namespace {
bool inRange(uint32_t cp, uint32_t lo, uint32_t hi) {
  return (cp >= lo) && (cp <= hi);
}

#define RetIfFound(expr) do { \
  slot = (expr); \
  if (slot != EmojiSlot::Unknown) return slot; \
} while (0)

struct CodepointSlot {
  uint32_t cp;
  EmojiSlot slot;
};

struct RangeSlot {
  uint32_t lo;
  uint32_t hi;
  EmojiSlot slot;
};

template <size_t N>
EmojiSlot slotFromSingles(const CodepointSlot (&items)[N], uint32_t cp) {
  for (size_t i = 0; i < N; i++) {
    if (items[i].cp == cp)
      return items[i].slot;
  }
  return EmojiSlot::Unknown;
}

template <size_t N>
EmojiSlot slotFromRanges(const RangeSlot (&items)[N], uint32_t cp) {
  for (size_t i = 0; i < N; i++) {
    if (cp >= items[i].lo && cp <= items[i].hi)
      return items[i].slot;
  }
  return EmojiSlot::Unknown;
}

bool isAsciiOnly(const char* src) {
  if (!src)
    return true;
  for (size_t i = 0; src[i] != 0; i++) {
    if (static_cast<uint8_t>(src[i]) >= 0x80)
      return false;
  }
  return true;
}

bool isIgnoredComponent(uint32_t cp) {
  if (cp == 0xFE0E || cp == 0xFE0F) return true;    // VS15/VS16
  if (cp == 0x200D) return true;                    // ZWJ
  if (inRange(cp, 0x1F3FB, 0x1F3FF)) return true;   // skin tones
  if (cp == 0x20E3) return true;                    // keycap mark
  return false;
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

  static const CodepointSlot kSpookySingles[] = {
    {0x1F47B, EmojiSlot::Spooky}, // 👻
    {0x1F383, EmojiSlot::Spooky}, // 🎃
    {0x1F52E, EmojiSlot::Spooky}, // 🔮
    {0x2620, EmojiSlot::Spooky},  // ☠
    {0x1F480, EmojiSlot::Spooky}, // 💀
    {0x1F578, EmojiSlot::Spooky}, // 🕸
    {0x1F577, EmojiSlot::Spooky}  // 🕷
  };
  static const RangeSlot kSpookyRanges[] = {
    {0x1F9D9, 0x1F9DF, EmojiSlot::Spooky} // 🧙-🧟
  };

  static const CodepointSlot kAnimalSingles[] = {
    {0x1F986, EmojiSlot::Duck},    // 🦆
    {0x1F410, EmojiSlot::Animals}, // 🐐
    {0x1F54A, EmojiSlot::Animals}  // 🕊
  };
  static const RangeSlot kAnimalRanges[] = {
    {0x1F400, 0x1F43F, EmojiSlot::Animals}, // 🐀-🐿
    {0x1F980, 0x1F997, EmojiSlot::Animals}, // 🦀-🦗
    {0x1F9A0, 0x1F9BF, EmojiSlot::Animals}  // 🦠-🦿
  };

  static const CodepointSlot kFaceSingles[] = {
    {0x1F60D, EmojiSlot::Love},     // 😍
    {0x1F970, EmojiSlot::Love},     // 🥰
    {0x1F618, EmojiSlot::Love},     // 😘
    {0x1F617, EmojiSlot::Love},     // 😗
    {0x1F619, EmojiSlot::Love},     // 😙
    {0x1F61A, EmojiSlot::Love},     // 😚
    {0x1F63B, EmojiSlot::Love},     // 😻

    {0x1F602, EmojiSlot::Laugh},    // 😂
    {0x1F923, EmojiSlot::Laugh},    // 🤣
    {0x1F606, EmojiSlot::Laugh},    // 😆
    {0x1F603, EmojiSlot::Laugh},    // 😃
    {0x1F604, EmojiSlot::Laugh},    // 😄
    {0x1F605, EmojiSlot::Laugh},    // 😅
    {0x1F642, EmojiSlot::Laugh},    // 🙂
    {0x1F61B, EmojiSlot::Laugh},    // 😛
    {0x1F61C, EmojiSlot::Laugh},    // 😜
    {0x1F61D, EmojiSlot::Laugh},    // 😝
    {0x1F92A, EmojiSlot::Laugh},    // 🤪
    {0x1F92D, EmojiSlot::Laugh},    // 🤭

    {0x1F620, EmojiSlot::Angry},    // 😠
    {0x1F621, EmojiSlot::Angry},    // 😡
    {0x1F92C, EmojiSlot::Angry},    // 🤬
    {0x1F624, EmojiSlot::Angry},    // 😤
    {0x1F63E, EmojiSlot::Angry},    // 😾
    {0x1F47F, EmojiSlot::Angry},    // 👿

    {0x1F622, EmojiSlot::Sad},      // 😢
    {0x1F62D, EmojiSlot::Sad},      // 😭
    {0x1F625, EmojiSlot::Sad},      // 😥
    {0x1F614, EmojiSlot::Sad},      // 😔
    {0x1F61E, EmojiSlot::Sad},      // 😞
    {0x1F613, EmojiSlot::Sad},      // 😓
    {0x1F62A, EmojiSlot::Sad},      // 😪
    {0x1F629, EmojiSlot::Sad},      // 😩
    {0x1F97A, EmojiSlot::Sad},      // 🥺
    {0x2639, EmojiSlot::Sad},       // ☹
    {0x1F63F, EmojiSlot::Sad},      // 😿

    {0x1F62E, EmojiSlot::Surprise}, // 😮
    {0x1F62F, EmojiSlot::Surprise}, // 😯
    {0x1F632, EmojiSlot::Surprise}, // 😲
    {0x1F631, EmojiSlot::Surprise}, // 😱
    {0x1F633, EmojiSlot::Surprise}, // 😳
    {0x1FAE2, EmojiSlot::Surprise}, // 🫢
    {0x1FAE3, EmojiSlot::Surprise}, // 🫣

    {0x1F914, EmojiSlot::Thinking}, // 🤔
    {0x1F9D0, EmojiSlot::Thinking}, // 🧐

    {0x1F60E, EmojiSlot::Cool},     // 😎

    {0x1F610, EmojiSlot::Unamused}, // 😐
    {0x1F611, EmojiSlot::Unamused}, // 😑
    {0x1F612, EmojiSlot::Unamused}, // 😒
    {0x1F62C, EmojiSlot::Unamused}, // 😬
    {0x1F636, EmojiSlot::Unamused}, // 😶
    {0x1FAE5, EmojiSlot::Unamused}, // 🫥
    {0x1FAE4, EmojiSlot::Unamused}, // 🫤
    {0x1F928, EmojiSlot::Unamused}, // 🤨
    {0x1F60F, EmojiSlot::Unamused}, // 😏
    {0x1F644, EmojiSlot::Unamused}, // 🙄

    {0x1F600, EmojiSlot::Happy},    // 😀
    {0x1F601, EmojiSlot::Happy},    // 😁
    {0x1F60A, EmojiSlot::Happy},    // 😊
    {0x1F607, EmojiSlot::Happy},    // 😇
    {0x263A, EmojiSlot::Happy},     // ☺
    {0x1F609, EmojiSlot::Happy},    // 😉
    {0x1F60C, EmojiSlot::Happy},    // 😌
    {0x1F929, EmojiSlot::Happy},    // 🤩
    {0x1F973, EmojiSlot::Happy}     // 🥳
  };
  static const RangeSlot kFaceRanges[] = {
    {0x1F600, 0x1F64F, EmojiSlot::Unamused}, // 😀-🙏
    {0x1F910, 0x1F92F, EmojiSlot::Unamused}, // 🤐-🤯
    {0x1F970, 0x1F97A, EmojiSlot::Unamused}, // 🥰-🥺
    {0x1FAE0, 0x1FAE8, EmojiSlot::Unamused}  // 🫠-🫨
  };

  static const CodepointSlot kYesNoSingles[] = {
    {0x1F44D, EmojiSlot::ApprovalYes},     // 👍
    {0x2705, EmojiSlot::ApprovalYes},      // ✅
    {0x2611, EmojiSlot::ApprovalYes},      // ☑
    {0x1F91D, EmojiSlot::ApprovalYes},     // 🤝
    {0x1F44E, EmojiSlot::DisapprovalNo},   // 👎
    {0x274C, EmojiSlot::DisapprovalNo},    // ❌
    {0x1F6AB, EmojiSlot::DisapprovalNo},   // 🚫
    {0x26D4, EmojiSlot::DisapprovalNo},    // ⛔
    {0x1F645, EmojiSlot::DisapprovalNo}    // 🙅
  };

  static const CodepointSlot kWarningSingles[] = {
    {0x26A0, EmojiSlot::Warning}, // ⚠
    {0x1F525, EmojiSlot::Warning}, // 🔥
    {0x1F4A5, EmojiSlot::Warning}, // 💥
    {0x1F6A8, EmojiSlot::Warning}, // 🚨
    {0x1F6D1, EmojiSlot::Warning}  // 🛑
  };

  static const CodepointSlot kLocationSingles[] = {
    {0x1F4CD, EmojiSlot::LocationDirection}, // 📍
    {0x1F9ED, EmojiSlot::LocationDirection}, // 🧭
    {0x1F5FA, EmojiSlot::LocationDirection}, // 🗺
    {0x1F30D, EmojiSlot::LocationDirection}, // 🌍
    {0x1F3D4, EmojiSlot::LocationDirection}  // 🏔
  };
  static const RangeSlot kLocationRanges[] = {
    {0x1F680, 0x1F6FF, EmojiSlot::LocationDirection} // 🚀-🛿
  };

  static const CodepointSlot kWorkToolSingles[] = {
    {0x1F9F9, EmojiSlot::WorkTools}, // 🧹
    {0x2692, EmojiSlot::WorkTools},  // ⚒
    {0x26CF, EmojiSlot::WorkTools},  // ⛏
    {0x1F6E0, EmojiSlot::WorkTools}, // 🛠
    {0x1F9F0, EmojiSlot::WorkTools}, // 🧰
    {0x1F9F2, EmojiSlot::WorkTools}, // 🧲
    {0x1FA93, EmojiSlot::WorkTools}, // 🪓
    {0x1FA9A, EmojiSlot::WorkTools}, // 🪚
    {0x1FA9B, EmojiSlot::WorkTools}, // 🪛
    {0x1FA9C, EmojiSlot::WorkTools}, // 🪜
    {0x1F9F1, EmojiSlot::WorkTools}, // 🧱
    {0x1F6A7, EmojiSlot::WorkTools}  // 🚧
  };
  static const RangeSlot kWorkToolRanges[] = {
    {0x1F527, 0x1F52B, EmojiSlot::WorkTools} // 🔧-🔫
  };

  static const CodepointSlot kTechSingles[] = {
    {0x2699, EmojiSlot::TechSystem},  // ⚙
    {0x1F50C, EmojiSlot::TechSystem}, // 🔌
    {0x1F50B, EmojiSlot::TechSystem}, // 🔋
    {0x1F50D, EmojiSlot::TechSystem}, // 🔍
    {0x1F50E, EmojiSlot::TechSystem}, // 🔎
    {0x1F4E1, EmojiSlot::TechSystem}, // 📡
    {0x1F4E0, EmojiSlot::TechSystem}, // 📠
    {0x1F4DE, EmojiSlot::TechSystem}, // 📞
    {0x260E, EmojiSlot::TechSystem},  // ☎
    {0x1F4F1, EmojiSlot::TechSystem}, // 📱
    {0x1F4F2, EmojiSlot::TechSystem}, // 📲
    {0x1F4FB, EmojiSlot::TechSystem}, // 📻
    {0x1F4F6, EmojiSlot::TechSystem}, // 📶
    {0x1F4FA, EmojiSlot::TechSystem}, // 📺
    {0x1F4F7, EmojiSlot::TechSystem}, // 📷
    {0x1F4F8, EmojiSlot::TechSystem}, // 📸
    {0x1F4F9, EmojiSlot::TechSystem}, // 📹
    {0x1F5A7, EmojiSlot::TechSystem}, // 🖧
    {0x1F5A8, EmojiSlot::TechSystem}, // 🖨
    {0x1F916, EmojiSlot::TechSystem}, // 🤖
    {0x1F5A5, EmojiSlot::TechSystem}, // 🖥
    {0x1F5B1, EmojiSlot::TechSystem}, // 🖱
    {0x1F5B2, EmojiSlot::TechSystem}, // 🖲
    {0x1F5B3, EmojiSlot::TechSystem}, // 🖳
    {0x1F9EE, EmojiSlot::TechSystem}  // 🧮
  };
  static const RangeSlot kTechRanges[] = {
    {0x1F4BB, 0x1F4BF, EmojiSlot::TechSystem} // 💻-📿
  };

  static const CodepointSlot kPeopleSingles[] = {
    {0x1F574, EmojiSlot::PeopleCommunity}, // 🕴
    {0x1F57A, EmojiSlot::PeopleCommunity}, // 🕺
    {0x1F575, EmojiSlot::PeopleCommunity}, // 🕵
    {0x1F482, EmojiSlot::PeopleCommunity}  // 💂
  };
  static const RangeSlot kPeopleRanges[] = {
    {0x1F466, 0x1F487, EmojiSlot::PeopleCommunity}, // 👦-💇
    {0x1F9D1, 0x1F9FF, EmojiSlot::PeopleCommunity}, // 🧑-🧿
    {0x1F440, 0x1F450, EmojiSlot::PeopleCommunity}, // 👀-👐
    {0x1F590, 0x1F596, EmojiSlot::PeopleCommunity}  // 🖐-🖖
  };

  static const CodepointSlot kSafetySingles[] = {
    {0x1F5DD, EmojiSlot::SafetySecurity}, // 🗝
    {0x1F6E1, EmojiSlot::SafetySecurity}, // 🛡
    {0x1F4B3, EmojiSlot::SafetySecurity}, // 💳
    {0x1F4DB, EmojiSlot::SafetySecurity}, // 📛
    {0x1FAA4, EmojiSlot::SafetySecurity}  // 🪤
  };
  static const RangeSlot kSafetyRanges[] = {
    {0x1F510, 0x1F513, EmojiSlot::SafetySecurity} // 🔐-🔓
  };

  static const CodepointSlot kCommunicationSingles[] = {
    {0x1F4AC, EmojiSlot::Communication}, // 💬
    {0x1F5E8, EmojiSlot::Communication}, // 🗨
    {0x1F4F1, EmojiSlot::Communication}, // 📱
    {0x1F4F2, EmojiSlot::Communication}, // 📲
    {0x1F399, EmojiSlot::Communication}, // 🎙
    {0x1F3A4, EmojiSlot::Communication}  // 🎤
  };
  static const RangeSlot kCommunicationRanges[] = {
    {0x1F4E0, 0x1F4EF, EmojiSlot::Communication} // 📠-📯
  };

  static const CodepointSlot kMoneySingles[] = {
    {0x1FA99, EmojiSlot::MoneyValue}, // 🪙
    {0x1F911, EmojiSlot::MoneyValue}, // 🤑
    {0x1F4C8, EmojiSlot::MoneyValue}, // 📈
    {0x1F4C9, EmojiSlot::MoneyValue}  // 📉
  };
  static const RangeSlot kMoneyRanges[] = {
    {0x1F4B0, 0x1F4BF, EmojiSlot::MoneyValue} // 💰-💿
  };

  static const CodepointSlot kCelebrationSingles[] = {
    {0x26BD, EmojiSlot::Celebration}, // ⚽
    {0x26BE, EmojiSlot::Celebration}, // ⚾
    {0x1F3C6, EmojiSlot::Celebration}, // 🏆
    {0x1F3C5, EmojiSlot::Celebration}, // 🏅
    {0x1F389, EmojiSlot::Celebration}, // 🎉
    {0x1F38A, EmojiSlot::Celebration}, // 🎊
    {0x1F386, EmojiSlot::Celebration}, // 🎆
    {0x1F387, EmojiSlot::Celebration}, // 🎇
    {0x1F942, EmojiSlot::Celebration}, // 🥂
    {0x1F37E, EmojiSlot::Celebration}  // 🍾
  };
  static const RangeSlot kCelebrationRanges[] = {
    {0x1F3A0, 0x1F3FF, EmojiSlot::Celebration}, // 🎠-🏿
    {0x1F93A, 0x1F94F, EmojiSlot::Celebration}  // 🤺-🥏
  };

  static const RangeSlot kNatureRanges[] = {
    {0x1F300, 0x1F32F, EmojiSlot::NatureEnv}, // 🌀-🌯
    {0x1F330, 0x1F34F, EmojiSlot::NatureEnv}, // 🌰-🍏
    {0x2600, 0x26FF, EmojiSlot::NatureEnv}    // ☀-⛿
  };

  static const CodepointSlot kTimeSingles[] = {
    {0x23F3, EmojiSlot::TimeControl}, // ⏳
    {0x231B, EmojiSlot::TimeControl}, // ⌛
    {0x23F0, EmojiSlot::TimeControl}, // ⏰
    {0x25B6, EmojiSlot::TimeControl}, // ▶
    {0x23F8, EmojiSlot::TimeControl}, // ⏸
    {0x23F9, EmojiSlot::TimeControl}, // ⏹
    {0x23FA, EmojiSlot::TimeControl}, // ⏺
    {0x27A1, EmojiSlot::TimeControl}, // ➡
    {0x2B05, EmojiSlot::TimeControl}, // ⬅
    {0x2B06, EmojiSlot::TimeControl}, // ⬆
    {0x2B07, EmojiSlot::TimeControl}  // ⬇
  };

  static const CodepointSlot kIdeaSingles[] = {
    {0x1F4A1, EmojiSlot::Idea}, // 💡
    {0x2728, EmojiSlot::Idea},  // ✨
    {0x1F4AD, EmojiSlot::Idea}  // 💭
  };

  static const CodepointSlot kQuestionSingles[] = {
    {0x2753, EmojiSlot::Question}, // ❓
    {0x2754, EmojiSlot::Question}, // ❔
    {0x2139, EmojiSlot::Question}, // ℹ
    {0x1F4DD, EmojiSlot::Question} // 📝
  };

  static const CodepointSlot kSuccessSingles[] = {
    {0x2B50, EmojiSlot::Success}, // ⭐
    {0x1F31F, EmojiSlot::Success}, // 🌟
    {0x1F4AF, EmojiSlot::Success}  // 💯
  };

  static const CodepointSlot kLoveSingles[] = {
    {0x2764, EmojiSlot::Love}, // ❤
    {0x1F48C, EmojiSlot::Love} // 💌
  };
  static const RangeSlot kLoveRanges[] = {
    {0x1F493, 0x1F49F, EmojiSlot::Love} // 💓-💟
  };

  EmojiSlot slot = slotFromSingles(kSpookySingles, cp);
  RetIfFound(slot);
  RetIfFound(slotFromRanges(kSpookyRanges, cp));

  RetIfFound(slotFromSingles(kAnimalSingles, cp));
  RetIfFound(slotFromRanges(kAnimalRanges, cp));

  RetIfFound(slotFromSingles(kYesNoSingles, cp));

  RetIfFound(slotFromSingles(kWarningSingles, cp));

  RetIfFound(slotFromSingles(kWorkToolSingles, cp));
  RetIfFound(slotFromRanges(kWorkToolRanges, cp));

  RetIfFound(slotFromSingles(kLocationSingles, cp));
  RetIfFound(slotFromRanges(kLocationRanges, cp));

  RetIfFound(slotFromSingles(kTechSingles, cp));
  RetIfFound(slotFromRanges(kTechRanges, cp));

  RetIfFound(slotFromSingles(kPeopleSingles, cp));
  RetIfFound(slotFromRanges(kPeopleRanges, cp));

  RetIfFound(slotFromSingles(kSafetySingles, cp));
  RetIfFound(slotFromRanges(kSafetyRanges, cp));

  RetIfFound(slotFromSingles(kCommunicationSingles, cp));
  RetIfFound(slotFromRanges(kCommunicationRanges, cp));

  RetIfFound(slotFromSingles(kMoneySingles, cp));
  RetIfFound(slotFromRanges(kMoneyRanges, cp));

  RetIfFound(slotFromSingles(kCelebrationSingles, cp));
  RetIfFound(slotFromRanges(kCelebrationRanges, cp));

  RetIfFound(slotFromRanges(kNatureRanges, cp));

  RetIfFound(slotFromSingles(kTimeSingles, cp));

  RetIfFound(slotFromSingles(kIdeaSingles, cp));

  RetIfFound(slotFromSingles(kQuestionSingles, cp));

  RetIfFound(slotFromSingles(kSuccessSingles, cp));

  RetIfFound(slotFromSingles(kLoveSingles, cp));
  RetIfFound(slotFromRanges(kLoveRanges, cp));

  RetIfFound(slotFromSingles(kFaceSingles, cp));
  RetIfFound(slotFromRanges(kFaceRanges, cp));

  // Catch-all: remaining emoji-ish pictographs fall back to Question.
  if (inRange(cp, 0x1F300, 0x1FAFF)) return EmojiSlot::Question;

  return EmojiSlot::Unknown;
}

void translateUTF8ToBlocks(char* dest, const char* src, size_t dest_size) {
  if (!dest || dest_size == 0)
    return;

  if (isAsciiOnly(src)) {
    strncpy(dest, src ? src : "", dest_size);
    dest[dest_size - 1] = 0;
    return;
  }

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
