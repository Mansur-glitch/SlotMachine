// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright © 2025 Mansur Mukhametzyanov
#ifndef SLOT_MACHINE_SYMBOL
#define SLOT_MACHINE_SYMBOL

#include <cstdint>

enum class SymbolFamily : uint32_t
{
  special = 0,
  fruit,
  suit,
  jewel,
  number
};
enum class Symbol : uint32_t
{
  lucky_seven = 0, // Biggest base value, doesn't combine with others
  cross,           // Breaks adjacent combos
  respin,          // Auto spin
  question,        // Combines with any other, reduced price
  apple,
  carrot,
  corn,
  grape,
  spade,
  club,
  heart,
  diamond,
  amethyst,
  emerald,
  topaz,
  crystal,
  number
};
constexpr uint32_t g_nsymbols = static_cast<uint32_t>(Symbol::number);
constexpr uint32_t g_nfamilies = static_cast<uint32_t>(SymbolFamily::number);
constexpr uint32_t g_nsy_family = g_nsymbols / g_nfamilies;

constexpr const char* get_symbol_name(Symbol s)
{
  switch (s) {
    case Symbol::lucky_seven:
      return "seven";
    case Symbol::respin:
      return "respin";
    case Symbol::question:
      return "question";
    case Symbol::cross:
      return "cross";
    case Symbol::apple:
      return "apple";
    case Symbol::carrot:
      return "carrot";
    case Symbol::corn:
      return "corn";
    case Symbol::grape:
      return "grape";
    case Symbol::spade:
      return "spade";
    case Symbol::club:
      return "club";
    case Symbol::heart:
      return "heart";
    case Symbol::diamond:
      return "diamond";
    case Symbol::amethyst:
      return "amethyst";
    case Symbol::emerald:
      return "emerald";
    case Symbol::topaz:
      return "topaz";
    case Symbol::crystal:
      return "crystal";
    case Symbol::number:
      return "";
  }
}

constexpr SymbolFamily get_symbol_family(Symbol sy) noexcept
{
  switch (sy) {
    case Symbol::lucky_seven:
    case Symbol::respin:
    case Symbol::question:
    case Symbol::cross:
      return SymbolFamily::special;
    case Symbol::apple:
    case Symbol::carrot:
    case Symbol::corn:
    case Symbol::grape:
      return SymbolFamily::fruit;
    case Symbol::spade:
    case Symbol::club:
    case Symbol::heart:
    case Symbol::diamond:
      return SymbolFamily::suit;
    case Symbol::amethyst:
    case Symbol::emerald:
    case Symbol::topaz:
    case Symbol::crystal:
      return SymbolFamily::jewel;
    case Symbol::number:
      return SymbolFamily::number;
  }
};

#endif
