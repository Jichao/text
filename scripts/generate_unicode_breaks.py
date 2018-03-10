#!/usr/bin/env python

cpp_file_form = decls = '''\
// Warning! This file is autogenerated.
#include <boost/text/{3}.hpp>

#include <algorithm>
#include <array>
#include <unordered_map>


namespace boost {{ namespace text {{

struct {0}_interval
{{
    uint32_t lo_;
    uint32_t hi_;
    {0}_t prop_;
}};

bool operator<({0}_interval lhs, {0}_interval rhs) noexcept
{{ return lhs.hi_ <= rhs.lo_; }}

static constexpr std::array<{0}_interval, {1}> g_{0}_intervals = {{{{
{2}
}}}};

static const std::unordered_map<uint32_t, {0}_t> g_{0}_map = {{
{4}
}};

{0}_t {0}(uint32_t cp) noexcept
{{
    auto const it = g_{0}_map.find(cp);
    if (it == g_{0}_map.end()) {{
        auto const it2 = std::lower_bound(g_{0}_intervals.begin(),
                                          g_{0}_intervals.end(),
                                          {0}_interval{{cp, cp + 1}});
        if (it2 == g_{0}_intervals.end() || cp < it2->lo_ || it2->hi_ <= cp)
            return {0}_t::{5};
        return it2->prop_;
    }}
    return it->second;
}}

}}}}
'''

bidi_header_form = decls = '''\
// Warning! This file is autogenerated.
#ifndef BOOST_TEXT_DETAIL_BIDIRECTIONAL_HPP
#define BOOST_TEXT_DETAIL_BIDIRECTIONAL_HPP

#include <boost/text/bidirectional_fwd.hpp>

#include <algorithm>
#include <array>

#include <stdint.h>


namespace boost {{ namespace text {{ namespace detail {{

enum class bidi_bracket_type {{
    open,
    close
}};

struct bidi_bracket_data
{{
    explicit operator bool() const {{ return cp_ != 0; }}

    uint32_t cp_;
    uint32_t paired_bracket_;
    bidi_bracket_type type_;
}};

inline bidi_bracket_data bidi_bracket(uint32_t cp) noexcept
{{
    constexpr std::array<bidi_bracket_data, {1}> brackets = {{{{
{0}
    }}}};

    auto const it = std::lower_bound(
        brackets.begin(), brackets.end(), bidi_bracket_data{{cp}},
        [](bidi_bracket_data lhs, bidi_bracket_data rhs){{
            return lhs.cp_ < rhs.cp_;
        }});
    if (it == brackets.end() || it->cp_ != cp)
        return bidi_bracket_data{{0}};
    return *it;
}}

struct bidi_mirroring_data
{{
    uint32_t cp_;
    int index_; // within bidi_mirroreds()
}};

inline int bidi_mirroring(uint32_t cp) noexcept
{{
    constexpr std::array<bidi_mirroring_data, {3}> mirrorings = {{{{
{2}
    }}}};

    auto const it = std::lower_bound(
        mirrorings.begin(), mirrorings.end(), bidi_mirroring_data{{cp}},
        [](bidi_mirroring_data lhs, bidi_mirroring_data rhs){{
            return lhs.cp_ < rhs.cp_;
        }});
    if (it == mirrorings.end() || it->cp_ != cp)
        return -1;
    return it->index_;
}}

inline std::array<uint32_t, {3}> const & bidi_mirroreds() noexcept
{{
    static std::array<uint32_t, 364> const retval = {{{{
{4}
    }}}};
    return retval;
}}

}}}}}}

#endif
'''


def extract_break_properties(filename, prop_):
    intervals = []
    prop_enum = prop_ + '_t'
    break_prop_lines = open(filename, 'r').readlines()
    for line in break_prop_lines:
        line = line[:-1]
        if not line.startswith('#') and len(line) != 0:
            comment_start = line.find('#')
            comment = ''
            if comment_start != -1:
                comment = line[comment_start + 1:].strip()
                line = line[:comment_start]
            fields = map(lambda x: x.strip(), line.split(';'))
            prop = fields[1]
            code_points = fields[0]
            if '..' in code_points:
                cps = code_points.split('.')
                interval = (int(cps[0], 16), int(cps[2], 16) + 1, prop)
            else:
                cp = int(code_points, 16)
                interval = (cp, cp + 1, prop)
            if 'line' not in prop_ or 'SG' not in interval[2]: # Skip surrogates.
                intervals.append(interval)

    intervals = sorted(intervals)
    intervals_list = ''
    intervals_map = ''
    num_intervals = 0
    for interval in intervals:
        if 128 < interval[1] - interval[0]:
            num_intervals += 1
            intervals_list += '    {}_interval{{{}, {}, {}::{}}},\n'.format(
                prop_, hex(interval[0]), hex(interval[1]), prop_enum, interval[2]
            )
        else:
            for i in range(interval[0], interval[1]):
                intervals_map += '    {{ {}, {}::{} }},\n'.format(
                    hex(i), prop_enum, interval[2]
                )
    return (intervals_list, num_intervals, intervals_map)


def extract_line_break_properties(filename, prop_):
    # https://unicode.org/reports/tr14/#BreakingRules
    # LB1 defines this mapping as the first step of line breaking.  We're
    # doing it in data generation instead.
    # Resolved  Original    General_Category
    # AL        AI, SG, XX  Any
    # CM        SA          Only Mn or Mc
    # AL        SA          Any except Mn and Mc
    # NS        CJ          Any

    unicode_data_lines = open('UnicodeData.txt', 'r').readlines()
    gen_category = {} # code point -> General_Category
    for line in unicode_data_lines:
        tokens = line.split(';')
        gen_category[int(tokens[0], 16)] = tokens[2]

    intervals = []
    prop_enum = prop_ + '_t'
    break_prop_lines = open(filename, 'r').readlines()
    for line in break_prop_lines:
        line = line[:-1]
        if not line.startswith('#') and len(line) != 0:
            comment_start = line.find('#')
            comment = ''
            if comment_start != -1:
                comment = line[comment_start + 1:].strip()
                line = line[:comment_start]
            fields = map(lambda x: x.strip(), line.split(';'))
            prop = fields[1]
            if prop == 'AI' or prop == 'SG' or prop == 'XX':
                prop = 'AL'
            if prop == 'CJ':
                prop = 'NS'
            code_points = fields[0]
            if '..' in code_points:
                cps = code_points.split('.')
                cps[0] = int(cps[0], 16)
                cps[2] = int(cps[2], 16)
                if prop == 'SA':
                    if gen_category[cps[0]] != gen_category[cps[2]]:
                        raise Exception('Oops!  Not all CPs in this range have the same General_Category.')
                    if gen_category[cps[0]] in ['Mn', 'Mc']:
                        prop = 'CM'
                    else:
                        prop = 'AL'
                interval = (cps[0], cps[2] + 1, prop)
            else:
                cp = int(code_points, 16)
                if prop == 'SA':
                    if gen_category[cp] in ['Mn', 'Mc']:
                        prop = 'CM'
                    else:
                        prop = 'AL'
                interval = (cp, cp + 1, prop)
            if 'line' not in prop_ or 'SG' not in interval[2]: # Skip surrogates.
                intervals.append(interval)

    intervals = sorted(intervals)
    intervals_list = ''
    intervals_map = ''
    num_intervals = 0
    for interval in intervals:
        if 128 < interval[1] - interval[0]:
            num_intervals += 1
            intervals_list += '    {}_interval{{{}, {}, {}::{}}},\n'.format(
                prop_, hex(interval[0]), hex(interval[1]), prop_enum, interval[2]
            )
        else:
            for i in range(interval[0], interval[1]):
                intervals_map += '    {{ {}, {}::{} }},\n'.format(
                    hex(i), prop_enum, interval[2]
                )
    return (intervals_list, num_intervals, intervals_map)


def extract_bidi_bracket_properties(filename):
    retval = []
    bidi_brackets_lines = open(filename, 'r').readlines()
    for line in bidi_brackets_lines:
        line = line[:-1]
        if not line.startswith('#') and len(line) != 0:
            comment_start = line.find('#')
            comment = ''
            if comment_start != -1:
                comment = line[comment_start + 1:].strip()
                line = line[:comment_start]
            tokens = map(lambda x: x.strip(), line.split(';'))
            retval.append('{{0x{}, 0x{}, {}}},'.format(tokens[0], tokens[1], tokens[2] == 'o' and 'bidi_bracket_type::open' or 'bidi_bracket_type::close'))
    return retval

def extract_bidi_mirroring_properties(filename):
    values = []
    mapping = []
    bidi_mirroring_lines = open(filename, 'r').readlines()
    for line in bidi_mirroring_lines:
        line = line[:-1]
        if not line.startswith('#') and len(line) != 0:
            comment_start = line.find('#')
            comment = ''
            if comment_start != -1:
                comment = line[comment_start + 1:].strip()
                line = line[:comment_start]
            tokens = map(lambda x: x.strip(), line.split(';'))
            values.append('0x{},'.format(tokens[0]))
    for line in bidi_mirroring_lines:
        line = line[:-1]
        if not line.startswith('#') and len(line) != 0:
            comment_start = line.find('#')
            comment = ''
            if comment_start != -1:
                comment = line[comment_start + 1:].strip()
                line = line[:comment_start]
            tokens = map(lambda x: x.strip(), line.split(';'))
            mapping.append('{{0x{}, {}}},'.format(tokens[0], values.index('0x' + tokens[1] + ',')))
    return mapping, values

(grapheme_break_intervals, num_grapheme_intervals, grapheme_break_intervals_map) = \
    extract_break_properties('GraphemeBreakProperty.txt', 'grapheme_prop')
cpp_file = open('grapheme_break.cpp', 'w')
cpp_file.write(cpp_file_form.format('grapheme_prop', num_grapheme_intervals, grapheme_break_intervals, 'grapheme_break', grapheme_break_intervals_map, 'Other'))

(word_break_intervals, num_word_intervals, word_break_intervals_map) = \
    extract_break_properties('WordBreakProperty.txt', 'word_prop')
cpp_file = open('word_break.cpp', 'w')
cpp_file.write(cpp_file_form.format('word_prop', num_word_intervals, word_break_intervals, 'word_break', word_break_intervals_map, 'Other'))

(sentence_break_intervals, num_sentence_intervals, sentence_break_intervals_map) = \
    extract_break_properties('SentenceBreakProperty.txt', 'sentence_prop')
cpp_file = open('sentence_break.cpp', 'w')
cpp_file.write(cpp_file_form.format('sentence_prop', num_sentence_intervals, sentence_break_intervals, 'sentence_break', sentence_break_intervals_map, 'Other'))

(line_break_intervals, num_line_intervals, line_break_intervals_map) = \
    extract_line_break_properties('LineBreak.txt', 'line_prop')
cpp_file = open('line_break.cpp', 'w')
cpp_file.write(cpp_file_form.format('line_prop', num_line_intervals, line_break_intervals, 'line_break', line_break_intervals_map, 'AL')) # AL in place of XX, due to Rule LB1

(bidi_intervals, num_bidi_intervals, bidi_intervals_map) = \
    extract_break_properties('DerivedBidiClass.txt', 'bidi_prop')
cpp_file = open('bidi.cpp', 'w')
cpp_file.write(cpp_file_form.format('bidi_prop', num_bidi_intervals, bidi_intervals, 'bidirectional', bidi_intervals_map, 'L'))

bidi_bracket_properties = extract_bidi_bracket_properties('BidiBrackets.txt')
bidi_bracket_properties_lines = '        ' + '\n        '.join(bidi_bracket_properties)
bidi_mirroring_mapping, bidi_mirroring_values = extract_bidi_mirroring_properties('BidiMirroring.txt')
bidi_mirroring_mapping_lines = '        ' + '\n        '.join(bidi_mirroring_mapping)
bidi_mirroring_values_lines = '        ' + '\n        '.join(bidi_mirroring_values)
hpp_file = open('bidirectional.hpp', 'w')
hpp_file.write(bidi_header_form.format(bidi_bracket_properties_lines, len(bidi_bracket_properties), bidi_mirroring_mapping_lines, len(bidi_mirroring_mapping), bidi_mirroring_values_lines))
