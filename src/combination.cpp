// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright © 2025 Mansur Mukhametzyanov
#include "combination.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>

using Range = Combination::Range;
using ContinuosRanges = Combination::ContinuosRanges;

constexpr uint32_t Combination::get_symbol_base_value(Symbol s) noexcept
{
  switch (s) {
    case Symbol::lucky_seven:
      return 32;
    case Symbol::cross:
    case Symbol::respin:
    case Symbol::question:
      return 12;
    case Symbol::apple:
    case Symbol::carrot:
    case Symbol::corn:
    case Symbol::grape:
      return 8;
    case Symbol::spade:
    case Symbol::club:
    case Symbol::heart:
    case Symbol::diamond:
      return 16;
    case Symbol::amethyst:
    case Symbol::emerald:
    case Symbol::topaz:
    case Symbol::crystal:
      return 24;
    case Symbol::number:
      return 0;
  }
}


Range Combination::get_max_equal_range(uint32_t* data, uint32_t size)
{
  assert(size != 0);

  Range cur{ 0, 1 };
  Range maximal = cur;
  uint32_t prev_val = data[0];

  for (uint32_t i = 1; i < size; ++i) {
    if (data[i] == prev_val) {
      cur.size += 1;
    } else {
      maximal = cur.size > maximal.size ? cur : maximal;
      cur.begin = i;
      cur.size = 1;
      prev_val = data[i];
    }
  }
  maximal = cur.size > maximal.size ? cur : maximal;

  return maximal;
}

bool operator==(const Range& lhs, const Range& rhs) noexcept
{
  return lhs.begin == rhs.begin && lhs.size == rhs.size;
}

ContinuosRanges Combination::get_ranges(uint32_t* data, uint32_t size)
{
  assert(size != 0);

  // Each symbol is a range in the begining
  // Unit ranges with equal data values by assigning identical begin and size
  // Lastly, remove all duplicate ranges
  ContinuosRanges ranges(size);
  ranges[0] = { 0, 1 };

  for (uint32_t i = 1; i < size; ++i) {
    Range& prev_range = ranges[ranges[i - 1].begin];

    if (data[i] == data[prev_range.begin]) {
      ranges[i].begin = prev_range.begin;
      prev_range.size += 1;
    } else {
      uint32_t range_end = prev_range.begin + prev_range.size;
      for (uint32_t j = prev_range.begin + 1; j < range_end; ++j) {
        ranges[j] = prev_range;
      }
      ranges[i].begin = i;
      ranges[i].size = 1;
    }
  }
  Range& last_range = ranges[ranges[size - 1].begin];
  for (uint32_t j = last_range.begin + 1; j < size; ++j) {
    ranges[j] = last_range;
  }
  auto unique_end_it = std::unique(ranges.begin(), ranges.end());
  uint32_t unique_size = unique_end_it - ranges.begin();
  ranges.resize(unique_size);

  // May contain empty range
  if (auto it = std::find(ranges.begin(), ranges.end(), Range{ 0, 0 });
      it != ranges.end()) {
    ranges.erase(it);
  }

  return ranges;
}

Combination::Combination(SymbolRow row)
  : m_row(row)
  , m_ranges(get_ranges(reinterpret_cast<uint32_t*>(row.data()), row.size()))
{
}

void Combination::apply_questions()
{
  if (m_ranges[0].size == g_nreels) {
    return;
  }

  // Connect m_ranges with '?' in middle
  for (uint32_t i = 1; i < m_ranges.size() - 1;) {
    Symbol s = m_row[m_ranges[i].begin];
    Symbol ls = m_row[m_ranges[i - 1].begin];
    Symbol rs = m_row[m_ranges[i + 1].begin];

    if (s == Symbol::question) {
      if (ls == rs) {
        m_ranges[i - 1].size += m_ranges[i].size + m_ranges[i + 1].size;
        // Need to remove extraneous ranges now to prevent interlaped ranges appear later
        m_ranges.erase(m_ranges.begin() + i, m_ranges.begin() + i + 2);
        --i;
      }
    }
    ++i;
  }

  // Expand m_ranges to left or right side
  for (uint32_t i = 0; i < m_ranges.size(); ++i) {
    Symbol s = m_row[m_ranges[i].begin];
    if (s == Symbol::question) {
      Range dummy{ 0, 0 };
      // Biggest near range
      Range& max_range = i == 0                     ? m_ranges[i + 1]
                         : i == m_ranges.size() - 1 ? m_ranges[i - 1]
                         : m_ranges[i - 1].size < m_ranges[i + 1].size
                           ? m_ranges[i + 1]
                           : m_ranges[i - 1].size > m_ranges[i + 1].size
                           ? m_ranges[i - 1]
                           : dummy; // if ranges are equal don't apply

      if (max_range.size <
          m_ranges[i].size) { // '?' symbols dominate, don't apply multiplier
        continue;
      }
      max_range.size += m_ranges[i].size;
      max_range.begin = std::min(max_range.begin, m_ranges[i].begin);
      m_ranges.erase(m_ranges.begin() + i);
      --i;
    }
  }
}

void Combination::apply_crosses()
{
  for (uint32_t i = 0; i < m_ranges.size(); ++i) {
    Symbol s = m_row[m_ranges[i].begin];

    if (s == Symbol::cross) {
      if (i > 0) {
        m_ranges[i - 1].size = 1;
      }
      if (i < m_ranges.size() - 1) {
        m_ranges[i + 1].size = 1;
      }
    }
  }
}

void Combination::apply_family_symbols()
{
  for (uint32_t i = 1; i < m_ranges.size(); ++i) {
    Range& prev = m_ranges[i - 1];
    Range& cur = m_ranges[i];
    Symbol s = m_row[cur.begin];
    Symbol ps = m_row[cur.begin - 1];

    SymbolFamily sf = get_symbol_family(s);
    SymbolFamily psf = get_symbol_family(ps);
    if (sf != psf || sf == SymbolFamily::special) {
      continue;
    }

    // Connected with 'x' symbol
    bool prev_broken = i > 1 && m_row[m_ranges[i - 2].begin] == Symbol::cross;
    bool cur_broken =
      i < m_ranges.size() - 1 && m_row[m_ranges[i + 1].begin] == Symbol::cross;

    if (prev_broken || cur_broken) {
      continue;
    }
    prev.size += cur.size;
    cur = prev;
  }

  auto unique_end_it = std::unique(m_ranges.begin(), m_ranges.end());
  uint32_t unique_size = unique_end_it - m_ranges.begin();
  m_ranges.resize(unique_size);
}

Combination::Result Combination::get_result()
{
  // In this order
  apply_questions();
  apply_crosses();
  apply_family_symbols();

  uint32_t max_combo_range_index = 0;
  uint32_t max_combo_size = m_ranges[0].size;
  for (uint32_t i = 1; i < m_ranges.size(); ++i) {
    if (m_ranges[i].size > max_combo_size) {
      max_combo_range_index = i;
      max_combo_size = m_ranges[i].size;
    }
  }
  Range combo = m_ranges[max_combo_range_index];

  // Weak combination
  if (combo.size < (g_nreels + 1) / 2) {
    return { combo, 0, false };
  }

  Range equal_symbols = get_max_equal_range(
    reinterpret_cast<uint32_t*>(m_row.data() + combo.begin), combo.size);
  Symbol dominant = m_row[combo.begin + equal_symbols.begin];

  // Have equal size '?' sequence on the left or in middle of real dominant sequence
  if (dominant == Symbol::question && equal_symbols.size < combo.size) {
    uint32_t cmb_beg = combo.begin;
    uint32_t cmb_end = cmb_beg + combo.size;
    uint32_t es_beg = equal_symbols.begin + combo.begin;
    uint32_t es_end = es_beg + equal_symbols.size; 

    bool in_middle = es_beg > cmb_beg && es_end < cmb_end; 
    bool in_middle_dominated = in_middle && m_row[es_beg-1] == m_row[es_end];
    if (in_middle_dominated) {
      dominant = m_row[es_beg-1];
    } else { // '?' sequence on the left
      dominant = m_row[es_end];
      equal_symbols.begin += equal_symbols.size;
    }
  }

  uint32_t multiplier =
    static_cast<uint32_t>(std::pow(big_multiplier, equal_symbols.size));
  multiplier *= static_cast<uint32_t>(
    std::pow(small_multiplier, combo.size - equal_symbols.size));

  Result res;
  res.combo_range = combo;
  res.points = multiplier * get_symbol_base_value(dominant);
  res.free_speen = dominant == Symbol::respin;
  return res;
}
