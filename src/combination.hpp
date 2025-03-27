// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright © 2025 Mansur Mukhametzyanov
#ifndef SLOT_MACHINE_COMBINATION
#define SLOT_MACHINE_COMBINATION

#include "configuration.hpp"
#include "symbol.hpp"

#include <array>
#include <cstdint>
#include <vector>

class Combination
{
public:
  using SymbolRow = std::array<Symbol, g_nreels>;

  struct Range
  {
    uint32_t begin{ 0 };
    uint32_t size{ 0 };
  };
  using ContinuosRanges = std::vector<Range>;
  struct Result
  {
    Range combo_range;
    uint32_t points;
    bool free_speen;
  };

  Combination(SymbolRow row);
  Result get_result();

private:
  static constexpr uint32_t get_symbol_base_value(Symbol s) noexcept;
  static constexpr uint32_t big_multiplier = 5;
  static constexpr uint32_t small_multiplier = 2;
  static Range get_max_equal_range(uint32_t* data, uint32_t size);
  static ContinuosRanges get_ranges(uint32_t* data, uint32_t size);
  // Substitute '?' symbols in some cases
  void apply_questions();
  // Break nearest to 'x' combinations
  void apply_crosses();
  // Enhance combo with adjacent family symbols
  void apply_family_symbols();

  SymbolRow m_row;
  ContinuosRanges m_ranges;
};
#endif
