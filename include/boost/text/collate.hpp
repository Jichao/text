#ifndef BOOST_TEXT_COLLATE_HPP
#define BOOST_TEXT_COLLATE_HPP

#include <boost/text/algorithm.hpp>
#include <boost/text/collation_fwd.hpp>
#include <boost/text/normalize.hpp>
#include <boost/text/normalize_string.hpp>
#include <boost/text/string.hpp>
#include <boost/text/detail/collation_data.hpp>

#include <boost/algorithm/cxx14/equal.hpp>
#include <boost/algorithm/cxx14/mismatch.hpp>
#include <boost/container/small_vector.hpp>

#include <vector>

#include <iostream> // TODO

#ifndef BOOST_TEXT_DOXYGEN

#ifndef BOOST_TEXT_COLLATE_INSTRUMENTATION
#define BOOST_TEXT_COLLATE_INSTRUMENTATION 0
#endif

#endif


namespace boost { namespace text { inline namespace v1 {

    /** A collation sort key.  Consists of a sequence of 32-bit values. */
    struct text_sort_key
    {
        using const_iterator = std::vector<uint32_t>::const_iterator;
        using iterator = const_iterator;
        using value_type = uint32_t;

        text_sort_key() {}
        explicit text_sort_key(std::vector<uint32_t> bytes) :
            storage_(std::move(bytes))
        {}

        std::size_t size() const noexcept { return storage_.size(); }

        const_iterator begin() const noexcept { return storage_.begin(); }
        const_iterator end() const noexcept { return storage_.end(); }

    private:
        std::vector<uint32_t> storage_;
    };

#if BOOST_TEXT_COLLATE_INSTRUMENTATION
    inline std::ostream & operator<<(std::ostream & os, text_sort_key const & k)
    {
        os << std::hex << "[";
        for (auto x : k) {
            os << " " << x;
        }
        os << " ]" << std::dec;
        return os;
    }
#endif

    /** Returns 0 if the given sort keys are equal, a value < 0 if `lhs` is
        less than `rhs`, and a value > 0 otherwise. */
    inline int
    compare(text_sort_key const & lhs, text_sort_key const & rhs) noexcept
    {
        auto const pair =
            algorithm::mismatch(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        if (pair.first == lhs.end()) {
            if (pair.second == rhs.end())
                return 0;
            return -1;
        } else {
            if (pair.second == rhs.end())
                return 1;
            auto const lhs_element = *pair.first;
            auto const rhs_element = *pair.second;
            if (lhs_element < rhs_element)
                return -1;
            if (rhs_element < lhs_element)
                return 1;
            return 0;
        }
    }

    inline bool operator==(text_sort_key const & lhs, text_sort_key const & rhs)
    {
        return algorithm::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    inline bool operator!=(text_sort_key const & lhs, text_sort_key const & rhs)
    {
        return !(lhs == rhs);
    }

    inline bool operator<(text_sort_key const & lhs, text_sort_key const & rhs)
    {
        return std::lexicographical_compare(
            lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    inline bool operator<=(text_sort_key const & lhs, text_sort_key const & rhs)
    {
        return boost::text::v1::compare(lhs, rhs) < 0;
    }

    inline bool operator>(text_sort_key const & lhs, text_sort_key const & rhs)
    {
        return std::lexicographical_compare(
            rhs.begin(), rhs.end(), lhs.begin(), lhs.end());
    }

    inline bool operator>=(text_sort_key const & lhs, text_sort_key const & rhs)
    {
        return 0 < compare(lhs, rhs);
    }

    // The code in this file implements the UCA as described in
    // http://www.unicode.org/reports/tr10/#Main_Algorithm .  The numbering
    // and some variable naming comes from there.
    namespace detail {

        // http://www.unicode.org/reports/tr10/#Derived_Collation_Elements
        template<typename OutIter, typename LeadByteFunc>
        inline OutIter add_derived_elements(
            uint32_t cp,
            variable_weighting weighting,
            OutIter out,
            detail::collation_trie_t const & trie,
            collation_element const * collation_elements_first,
            LeadByteFunc const & lead_byte,
            collation_strength strength,
            retain_case_bits_t retain_case_bits)
        {
            // Core Han Unified Ideographs
            std::array<uint32_t, 12> const CJK_Compatibility_Ideographs = {
                {0xFA0E,
                 0xFA0F,
                 0xFA11,
                 0xFA13,
                 0xFA14,
                 0xFA1F,
                 0xFA21,
                 0xFA23,
                 0xFA24,
                 0xFA27,
                 0xFA28,
                 0xFA29}};

            std::array<uint32_t, 222> const CJK_Unified_Ideographs_Extension_D =
                {{0x2B740, 0x2B741, 0x2B742, 0x2B743, 0x2B744, 0x2B745, 0x2B746,
                  0x2B747, 0x2B748, 0x2B749, 0x2B74A, 0x2B74B, 0x2B74C, 0x2B74D,
                  0x2B74E, 0x2B74F, 0x2B750, 0x2B751, 0x2B752, 0x2B753, 0x2B754,
                  0x2B755, 0x2B756, 0x2B757, 0x2B758, 0x2B759, 0x2B75A, 0x2B75B,
                  0x2B75C, 0x2B75D, 0x2B75E, 0x2B75F, 0x2B760, 0x2B761, 0x2B762,
                  0x2B763, 0x2B764, 0x2B765, 0x2B766, 0x2B767, 0x2B768, 0x2B769,
                  0x2B76A, 0x2B76B, 0x2B76C, 0x2B76D, 0x2B76E, 0x2B76F, 0x2B770,
                  0x2B771, 0x2B772, 0x2B773, 0x2B774, 0x2B775, 0x2B776, 0x2B777,
                  0x2B778, 0x2B779, 0x2B77A, 0x2B77B, 0x2B77C, 0x2B77D, 0x2B77E,
                  0x2B77F, 0x2B780, 0x2B781, 0x2B782, 0x2B783, 0x2B784, 0x2B785,
                  0x2B786, 0x2B787, 0x2B788, 0x2B789, 0x2B78A, 0x2B78B, 0x2B78C,
                  0x2B78D, 0x2B78E, 0x2B78F, 0x2B790, 0x2B791, 0x2B792, 0x2B793,
                  0x2B794, 0x2B795, 0x2B796, 0x2B797, 0x2B798, 0x2B799, 0x2B79A,
                  0x2B79B, 0x2B79C, 0x2B79D, 0x2B79E, 0x2B79F, 0x2B7A0, 0x2B7A1,
                  0x2B7A2, 0x2B7A3, 0x2B7A4, 0x2B7A5, 0x2B7A6, 0x2B7A7, 0x2B7A8,
                  0x2B7A9, 0x2B7AA, 0x2B7AB, 0x2B7AC, 0x2B7AD, 0x2B7AE, 0x2B7AF,
                  0x2B7B0, 0x2B7B1, 0x2B7B2, 0x2B7B3, 0x2B7B4, 0x2B7B5, 0x2B7B6,
                  0x2B7B7, 0x2B7B8, 0x2B7B9, 0x2B7BA, 0x2B7BB, 0x2B7BC, 0x2B7BD,
                  0x2B7BE, 0x2B7BF, 0x2B7C0, 0x2B7C1, 0x2B7C2, 0x2B7C3, 0x2B7C4,
                  0x2B7C5, 0x2B7C6, 0x2B7C7, 0x2B7C8, 0x2B7C9, 0x2B7CA, 0x2B7CB,
                  0x2B7CC, 0x2B7CD, 0x2B7CE, 0x2B7CF, 0x2B7D0, 0x2B7D1, 0x2B7D2,
                  0x2B7D3, 0x2B7D4, 0x2B7D5, 0x2B7D6, 0x2B7D7, 0x2B7D8, 0x2B7D9,
                  0x2B7DA, 0x2B7DB, 0x2B7DC, 0x2B7DD, 0x2B7DE, 0x2B7DF, 0x2B7E0,
                  0x2B7E1, 0x2B7E2, 0x2B7E3, 0x2B7E4, 0x2B7E5, 0x2B7E6, 0x2B7E7,
                  0x2B7E8, 0x2B7E9, 0x2B7EA, 0x2B7EB, 0x2B7EC, 0x2B7ED, 0x2B7EE,
                  0x2B7EF, 0x2B7F0, 0x2B7F1, 0x2B7F2, 0x2B7F3, 0x2B7F4, 0x2B7F5,
                  0x2B7F6, 0x2B7F7, 0x2B7F8, 0x2B7F9, 0x2B7FA, 0x2B7FB, 0x2B7FC,
                  0x2B7FD, 0x2B7FE, 0x2B7FF, 0x2B800, 0x2B801, 0x2B802, 0x2B803,
                  0x2B804, 0x2B805, 0x2B806, 0x2B807, 0x2B808, 0x2B809, 0x2B80A,
                  0x2B80B, 0x2B80C, 0x2B80D, 0x2B80E, 0x2B80F, 0x2B810, 0x2B811,
                  0x2B812, 0x2B813, 0x2B814, 0x2B815, 0x2B816, 0x2B817, 0x2B818,
                  0x2B819, 0x2B81A, 0x2B81B, 0x2B81C, 0x2B81D}};

            double const spacing = implicit_weights_spacing_times_ten / 10.0;

            for (auto seg : make_implicit_weights_segments()) {
                if (seg.first_ <= cp && cp < seg.last_) {
                    if (seg.first_ == CJK_Compatibility_Ideographs[0] &&
                        ((cp & ~OR_CJK_Compatibility_Ideographs) ||
                         !std::count(
                             CJK_Compatibility_Ideographs.begin(),
                             CJK_Compatibility_Ideographs.end(),
                             cp))) {
                        continue;
                    }

                    if (seg.first_ == CJK_Unified_Ideographs_Extension_D[0] &&
                        ((cp & ~OR_CJK_Unified_Ideographs_Extension_D) ||
                         !std::binary_search(
                             CJK_Unified_Ideographs_Extension_D.begin(),
                             CJK_Unified_Ideographs_Extension_D.end(),
                             cp))) {
                        continue;
                    }

                    uint32_t const primary_weight_low_bits =
                        seg.primary_offset_ + (cp - seg.first_) * spacing;
                    BOOST_ASSERT(
                        (primary_weight_low_bits & 0xfffff) ==
                        primary_weight_low_bits);
                    uint32_t const bytes[4] = {
                        implicit_weights_first_lead_byte,
                        ((primary_weight_low_bits >> 12) & 0xfe) | 0x1,
                        ((primary_weight_low_bits >> 5) & 0xfe) | 0x1,
                        (primary_weight_low_bits >> 0) & 0x3f};
                    uint32_t const primary = bytes[0] << 24 | bytes[1] << 16 |
                                             bytes[2] << 8 | bytes[3] << 0;
                    collation_element ce{primary, 0x0500, 0x0500, 0x0};

                    ce.l1_ = detail::replace_lead_byte(ce.l1_, lead_byte(ce));

                    *out++ = ce;
                    return out;
                }
            }

            // This is not tailorable, so we won't use lead_byte here.
            *out++ = collation_element{
                (implicit_weights_final_lead_byte << 24) | (cp & 0xffffff),
                0x0500,
                0x0500,
                0x0};
            return out;
        }

        inline bool variable(collation_element ce) noexcept
        {
            auto const lo = min_variable_collation_weight;
            auto const hi = max_variable_collation_weight;
            return lo <= ce.l1_ && ce.l1_ <= hi;
        }

        inline bool ignorable(collation_element ce) noexcept
        {
            return ce.l1_ == 0;
        }

        // http://www.unicode.org/reports/tr10/#Variable_Weighting
        template<typename CEIter>
        inline bool s2_3(
            CEIter first,
            CEIter last,
            collation_strength strength,
            variable_weighting weighting,
            bool after_variable,
            retain_case_bits_t retain_case_bits)
        {
            if (retain_case_bits == retain_case_bits_t::no) {
                for (auto it = first; it != last; ++it) {
                    auto & ce = *it;
                    // The top two bits in each byte in FractionalUCA.txt's L3
                    // weights are for the case level.
                    // http://www.unicode.org/reports/tr35/tr35-collation.html#File_Format_FractionalUCA_txt
                    uint16_t const l3 = ce.l3_ & disable_case_level_mask;

                    ce.l3_ = l3;
                }
            }

            if (weighting == variable_weighting::non_ignorable)
                return after_variable;

            if (strength == collation_strength::primary)
                return after_variable;

            // http://www.unicode.org/reports/tr10/#Implicit_Weights says: "If
            // a fourth or higher weights are used, then the same pattern is
            // followed for those weights. They are set to a non-zero value in
            // the first collation element and zero in the second."
            //
            // Even though this appears in the section on implicit weights
            // that "do not have explicit mappings in the DUCET", this
            // apparently applies to any pair of collation elements that
            // matches the pattern produced by the derived weight algorithm,
            // since that's what CollationTest_SHIFTED.txt expects.
            bool second_of_implicit_weight_pair = false;

            for (auto it = first; it != last; ++it) {
                auto & ce = *it;
                if (after_variable && detail::ignorable(ce)) {
                    ce.l1_ = 0;
                    ce.l2_ = 0;
                    ce.l3_ = 0;
                    ce.l4_ = 0;
                } else if (!ce.l1_) {
                    if (!ce.l2_ && !ce.l3_) {
                        ce.l4_ = 0x0000;
                    } else if (ce.l3_) {
                        if (after_variable)
                            ce.l4_ = 0x0000;
                        else
                            ce.l4_ = 0xffffffff;
                    }
                    after_variable = false;
                } else if (detail::variable(ce)) {
                    ce.l4_ = ce.l1_;
                    ce.l1_ = 0;
                    ce.l2_ = 0;
                    ce.l3_ = 0;
                    after_variable = true;
                } else {
                    if (ce.l1_)
                        ce.l4_ = 0xffffffff;
                    after_variable = false;
                }
                if (second_of_implicit_weight_pair) {
                    ce.l4_ = 0;
                    second_of_implicit_weight_pair = false;
                }
#if 0
                 // Not necessary with FractionalUCA.txt-derived data, in
                 // which each implicit weight before the unassigned code
                 // points is only one CE.
                second_of_implicit_weight_pair =
                    implicit_weights_first_lead_byte <= l1 &&
                    l1 <= implicit_weights_final_lead_byte;
#endif
            }

            return after_variable;
        }

        template<
            typename CPIter,
            typename CPOutIter,
            typename LeadByteFunc,
            typename SizeOutIter = std::ptrdiff_t *>
        auto
        s2(CPIter first,
           CPIter last,
           CPOutIter out,
           detail::collation_trie_t const & trie,
           collation_element const * collation_elements_first,
           LeadByteFunc const & lead_byte,
           collation_strength strength,
           variable_weighting weighting,
           retain_case_bits_t retain_case_bits,
           SizeOutIter * size_out = nullptr)
            -> detail::cp_iter_ret_t<CPOutIter, CPIter>
        {
            std::array<bool, 256> derived_element_high_two_bytes = {{}};
            // TODO: Pull out into its own func.
            for (auto seg : make_implicit_weights_segments()) {
                for (uint32_t i = (seg.first_ >> 12),
                              end = (seg.last_ >> 12) + 1;
                     i != end;
                     ++i) {
                    BOOST_ASSERT(i < 256u);
                    derived_element_high_two_bytes[i] = true;
                }
            }

            bool after_variable = false;
            while (first != last) {
                // S2.1 Find longest prefix that results in a collation trie
                // match.
                trie_match_t collation_;
                collation_ = trie.longest_match(first, last);
                if (!collation_.match) {
                    // S2.2
                    uint32_t cp = *first++;
                    if (detail::hangul_syllable(cp)) {
                        auto cps = detail::decompose_hangul_syllable<3>(cp);
                        out =
                            s2(cps.begin(),
                               cps.end(),
                               out,
                               trie,
                               collation_elements_first,
                               lead_byte,
                               strength,
                               weighting,
                               retain_case_bits);
                        if (size_out)
                            *(*size_out)++ = 1;
                        continue;
                    }

                    if (!derived_element_high_two_bytes[cp >> 12]) {
                        // This is not tailorable, so we won't use lead_byte
                        // here.
                        *out++ = collation_element{
                            (implicit_weights_final_lead_byte << 24) |
                                (cp & 0xffffff),
                            0x0500,
                            0x0500,
                            0x0};
                        if (size_out)
                            *(*size_out)++ = 1;
                        continue;
                    }

                    collation_element derived_ces[32];
                    auto const derived_ces_end = detail::add_derived_elements(
                        cp,
                        weighting,
                        derived_ces,
                        trie,
                        collation_elements_first,
                        lead_byte,
                        strength,
                        retain_case_bits);
                    after_variable = detail::s2_3(
                        derived_ces,
                        derived_ces_end,
                        strength,
                        weighting,
                        after_variable,
                        retain_case_bits);
                    out = std::copy(derived_ces, derived_ces_end, out);
                    if (size_out)
                        *(*size_out)++ = 1;
                    continue;
                }
                first += collation_.size;

                // S2.1.1 Process any nonstarters following S.
                auto nonstarter_last = first;
                if (!collation_.leaf) {
                    nonstarter_last =
                        std::find_if(first, last, [](uint32_t cp) {
                            return detail::ccc(cp) == 0;
                        });
                }

                // S2.1.2
                auto nonstarter_first = first;
                while (!collation_.leaf &&
                       nonstarter_first != nonstarter_last &&
                       detail::ccc(*(nonstarter_first - 1)) <
                           detail::ccc(*nonstarter_first)) {
                    auto const cp = *nonstarter_first;
                    auto coll = trie.extend_subsequence(collation_, cp);
                    // S2.1.3
                    if (coll.match && collation_.size < coll.size) {
                        std::copy_backward(
                            first, nonstarter_first, nonstarter_first + 1);
                        *first++ = cp;
                        collation_ = coll;
                    }
                    ++nonstarter_first;
                }

                auto const collation_it = const_trie_iterator_t(collation_);

                // S2.4
                auto const initial_out = out;
                out = std::copy(
                    collation_it->value.begin(collation_elements_first),
                    collation_it->value.end(collation_elements_first),
                    out);

                // S2.3
                after_variable = detail::s2_3(
                    initial_out,
                    out,
                    strength,
                    weighting,
                    after_variable,
                    retain_case_bits);

                if (size_out) {
                    *(*size_out)++ = collation_it->value.size();
                    for (int i = 1; i < collation_.size; ++i) {
                        *(*size_out)++ = 0;
                    }
                }
            }

            return out;
        }

        using level_sort_key_values_t = container::small_vector<uint32_t, 256>;
        using level_sort_key_bytes_t = container::small_vector<uint8_t, 1024>;

        // In-place compression of values such that 8-bit byte values are
        // packed into a 32-bit dwords (e.g. 0x0000XX00, 0x0000YYZZ ->
        // 0x00XXYYZZ), based on
        // https://www.unicode.org/reports/tr10/#Reducing_Sort_Key_Lengths
        // 9.1.3.
        inline level_sort_key_bytes_t
        pack_words(level_sort_key_values_t const & values)
        {
            level_sort_key_bytes_t retval;

            // We cannot treat the inputs naively as a sequence of bytes,
            // because we don't know the endianness.
            for (auto x : values) {
                uint8_t const bytes[4] = {
                    uint8_t(x >> 24),
                    uint8_t((x >> 16) & 0xff),
                    uint8_t((x >> 8) & 0xff),
                    uint8_t(x & 0xff),
                };
                if (bytes[0])
                    retval.push_back(bytes[0]);
                if (bytes[1])
                    retval.push_back(bytes[1]);
                if (bytes[2])
                    retval.push_back(bytes[2]);
                if (bytes[3])
                    retval.push_back(bytes[3]);
            }

            return retval;
        }

        // In-place run-length encoding, based on
        // https://www.unicode.org/reports/tr10/#Reducing_Sort_Key_Lengths
        // 9.1.4.
        inline void
        rle(level_sort_key_bytes_t & bytes,
            uint8_t min_,
            uint8_t common,
            uint8_t max_)
        {
            uint8_t const min_top = (common - 1) - (min_ - 1);
            uint8_t const max_bottom = (common + 1) + (0xff - max_);
            int const bound = (min_top + max_bottom) / 2;

            auto it = bytes.begin();
            auto const end = bytes.end();
            auto out = bytes.begin();
            while (it != end) {
                if (*it == common) {
                    auto const last_common =
                        boost::text::v1::find_not(it, end, common);
                    auto const size = last_common - it;
                    if (last_common == end || *last_common < common) {
                        int const synthetic_low = min_top + size;
                        if (bound <= synthetic_low) {
                            auto const max_compressible_copies =
                                (bound - 1) - min_top;
                            auto const repetitions =
                                size / max_compressible_copies;
                            out = std::fill_n(out, repetitions, bound - 1);
                            auto const remainder =
                                size % max_compressible_copies;
                            if (remainder)
                                *out++ = min_top + remainder;
                        } else {
                            *out++ = synthetic_low;
                        }
                    } else {
                        int const synthetic_high = max_bottom - size;
                        if (synthetic_high < bound) {
                            auto const max_compressible_copies =
                                max_bottom - bound;
                            auto const repetitions =
                                size / max_compressible_copies;
                            out = std::fill_n(out, repetitions, bound);
                            auto const remainder =
                                size % max_compressible_copies;
                            if (remainder)
                                *out++ = max_bottom - remainder;
                        } else {
                            *out++ = synthetic_high;
                        }
                    }
                    it = last_common;
                } else {
                    if (min_ <= *it && *it < common)
                        *out = *it - (min_ - 1);
                    else if (common < *it && *it <= max_)
                        *out = *it + 0xff - max_;
                    else
                        *out = *it;
                    ++it;
                    ++out;
                }
            }

            bytes.erase(out, end);
        }

        inline void pad_words(level_sort_key_bytes_t & bytes)
        {
            int remainder = bytes.size() % 4;
            if (remainder) {
                remainder = 4 - remainder;
                bytes.resize(bytes.size() + remainder, 0);
            }
        }

        inline void level_bytes_to_values(
            level_sort_key_bytes_t const & bytes,
            level_sort_key_values_t & values)
        {
            BOOST_ASSERT(bytes.size() % 4 == 0);

            values.resize(bytes.size() / 4);

            auto out = values.begin();
            for (auto it = bytes.begin(), end = bytes.end(); it != end;
                 it += 4) {
                uint32_t const x = *(it + 0) << 24 | *(it + 1) << 16 |
                                   *(it + 2) << 8 | *(it + 3) << 0;
                *out++ = x;
            }
        }

        // https://www.unicode.org/reports/tr35/tr35-collation.html#Case_Weights
        inline collation_element modify_for_case(
            collation_element ce,
            collation_strength strength,
            case_first case_1st,
            case_level case_lvl) noexcept
        {
            if (case_1st == case_first::off && case_lvl == case_level::off) {
                ce.l3_ &= disable_case_level_mask;
                return ce;
            }

            uint16_t c = 0; // Set 1, 2, or 3 below.
            auto const case_bits = ce.l3_ & case_level_bits_mask;

            if (case_1st == case_first::upper) {
                c = (case_bits == upper_case_bits)
                        ? 1
                        : ((case_bits == mixed_case_bits) ? 2 : 3);
            } else {
                c = (case_bits == upper_case_bits)
                        ? 3
                        : ((case_bits == mixed_case_bits) ? 2 : 1);
            }

            if (case_lvl == case_level::on) {
                if (strength == collation_strength::primary) {
                    // Ensure we use values >= min_secondary_byte.
                    c += min_secondary_byte - 1;
                    if (!ce.l1_)
                        ce.l2_ = 0;
                    else
                        ce.l2_ = c << 8; // Shift bits into lead L2 byte.
                    ce.l3_ = 0;
                } else {
                    ce.l4_ = ce.l3_ & disable_case_level_mask;
                    // Ensure we use values >= min_tertiary_byte.
                    c += min_tertiary_byte - 1;
                    if (!ce.l1_ && !ce.l2_)
                        ce.l3_ = 0;
                    else
                        ce.l3_ = c << 8; // Shift into L3 lead byte.
                }
            } else {
                ce.l3_ &= disable_case_level_mask;
                if (ce.l2_)
                    ce.l3_ |= c << 14; // Shift into high 2 bits of L3.
                else if (ce.l3_)
                    ce.l3_ |= 3 << 14; // Shift into high 2 bits of L3.
            }

            return ce;
        }

        template<
            typename CEIter,
            typename CPIter,
            typename Sentinel,
            typename Container>
        auto
        s3(CEIter ces_first,
           CEIter ces_last,
           int ces_size,
           collation_strength strength,
           case_first case_1st,
           case_level case_lvl,
           l2_weight_order l2_order,
           CPIter cps_first,
           Sentinel cps_last,
           int cps_size,
           Container & bytes) -> detail::cp_iter_ret_t<void, CPIter>
        {
            level_sort_key_values_t l1;
            level_sort_key_values_t l2;
            level_sort_key_values_t l3;
            level_sort_key_values_t l4;
            // For when case level bumps L4.
            level_sort_key_values_t l4_overflow;

            auto const strength_for_copies =
                case_lvl == case_level::on
                    ? collation_strength(static_cast<int>(strength) + 1)
                    : strength;
            for (; ces_first != ces_last; ++ces_first) {
                auto ce = *ces_first;
                ce = detail::modify_for_case(ce, strength, case_1st, case_lvl);
                if (ce.l1_)
                    l1.push_back(ce.l1_);
                if (collation_strength::secondary <= strength_for_copies) {
                    if (ce.l2_)
                        l2.push_back(ce.l2_);
                    if (collation_strength::tertiary <= strength_for_copies) {
                        if (ce.l3_)
                            l3.push_back(ce.l3_);
                        if (collation_strength::quaternary <=
                            strength_for_copies) {
                            if (ce.l4_) {
                                l4.push_back(ce.l4_);
                                if (ces_first->l4_)
                                    l4_overflow.push_back(ces_first->l4_);
                            }
                        }
                    }
                }
            }

            if (l1.empty() && l2.empty() && l3.empty() && l4.empty() &&
                l4_overflow.empty()) {
                return;
            }

            if (!l2.empty()) {
                if (l2_order == l2_weight_order::backward)
                    std::reverse(l2.begin(), l2.end());
                auto packed_l2 = pack_words(l2);
                detail::rle(packed_l2,
                    min_secondary_byte,
                    common_secondary_byte,
                    max_secondary_byte);
                detail::pad_words(packed_l2);
                detail::level_bytes_to_values(packed_l2, l2);
                if (!l3.empty()) {
                    auto packed_l3 = detail::pack_words(l3);
                    detail::rle(packed_l3,
                        min_tertiary_byte,
                        common_tertiary_byte,
                        max_tertiary_byte);
                    detail::pad_words(packed_l3);
                    detail::level_bytes_to_values(packed_l3, l3);
                }
            }

            int const separators = static_cast<int>(strength_for_copies);

            level_sort_key_values_t nfd;
            if (collation_strength::quaternary < strength) {
                boost::text::v1::normalize_to_nfd(
                    cps_first, cps_last, std::back_inserter(nfd));
            }

            int size = l1.size();
            if (collation_strength::primary < strength_for_copies) {
                size += l2.size();
                if (collation_strength::secondary < strength_for_copies) {
                    size += l3.size();
                    if (collation_strength::tertiary < strength_for_copies) {
                        size += l4.size();
                        if (!l4_overflow.empty()) {
                            ++size;
                            size += l4_overflow.size();
                        }
                        if (collation_strength::quaternary <
                            strength_for_copies)
                            size += nfd.size();
                    }
                }
            }
            size += separators;

            bytes.resize(bytes.size() + size);

            auto it = bytes.end() - size;
            it = std::copy(l1.begin(), l1.end(), it);
            if (collation_strength::primary < strength_for_copies) {
                *it++ = 0x0000;
                it = std::copy(l2.begin(), l2.end(), it);
                if (collation_strength::secondary < strength_for_copies) {
                    *it++ = 0x0000;
                    it = std::copy(l3.begin(), l3.end(), it);
                    if (collation_strength::tertiary < strength_for_copies) {
                        *it++ = 0x0000;
                        it = std::copy(l4.begin(), l4.end(), it);
                        if (!l4_overflow.empty()) {
                            *it++ = 0x0000;
                            it = std::copy(
                                l4_overflow.begin(), l4_overflow.end(), it);
                        }
                        if (collation_strength::quaternary <
                            strength_for_copies) {
                            *it++ = 0x0000;
                            it = std::copy(nfd.begin(), nfd.end(), it);
                        }
                    }
                }
            }
            BOOST_ASSERT(it == bytes.end());
        }

        template<typename CPIter, typename Sentinel>
        auto collation_sort_key(
            CPIter first,
            Sentinel last,
            collation_strength strength,
            case_first case_1st,
            case_level case_lvl,
            variable_weighting weighting,
            l2_weight_order l2_order,
            collation_table const & table)
            -> detail::cp_iter_ret_t<text_sort_key, CPIter>;

        template<
            typename CPIter1,
            typename Sentinel1,
            typename CPIter2,
            typename Sentinel2>
        int collate_impl(
            CPIter1 lhs_first,
            Sentinel1 lhs_last,
            CPIter2 rhs_first,
            Sentinel2 rhs_last,
            collation_strength strength,
            case_first case_1st,
            case_level case_lvl,
            variable_weighting weighting,
            l2_weight_order l2_order,
            collation_table const & table);

        template<typename Result, typename Iter>
        auto make_iterator(Result first, Iter it, null_sentinel s)
            -> decltype(Result(first.base(), it, s))
        {
            return Result(first.base(), it, s);
        }

        template<typename Result, typename Iter>
        auto make_iterator(Result first, Iter it, Result last)
            -> decltype(Result(first.base(), it, last.base()))
        {
            return Result(first.base(), it, last.base());
        }

        template<typename Iter>
        Iter make_iterator(Iter first, Iter it, Iter last)
        {
            return it;
        }

        template<typename Iter, typename Sentinel>
        auto collate_impl(
            utf8_tag,
            Iter lhs_first,
            Sentinel lhs_last,
            utf8_tag,
            Iter rhs_first,
            Sentinel rhs_last,
            collation_strength strength,
            case_first case_1st,
            case_level case_lvl,
            variable_weighting weighting,
            l2_weight_order l2_order,
            collation_table const & table);

        template<
            typename Tag1,
            typename Iter1,
            typename Sentinel1,
            typename Tag2,
            typename Iter2,
            typename Sentinel2>
        auto collate_impl(
            Tag1,
            Iter1 lhs_first,
            Sentinel1 lhs_last,
            Tag2,
            Iter2 rhs_first,
            Sentinel2 rhs_last,
            collation_strength strength,
            case_first case_1st,
            case_level case_lvl,
            variable_weighting weighting,
            l2_weight_order l2_order,
            collation_table const & table)
        {
            auto const lhs = boost::text::v1::as_utf32(lhs_first, lhs_last);
            auto const rhs = boost::text::v1::as_utf32(rhs_first, rhs_last);
            text_sort_key const lhs_sk = collation_sort_key(
                lhs.begin(),
                lhs.end(),
                strength,
                case_1st,
                case_lvl,
                weighting,
                l2_order,
                table);
            text_sort_key const rhs_sk = collation_sort_key(
                rhs.begin(),
                rhs.end(),
                strength,
                case_1st,
                case_lvl,
                weighting,
                l2_order,
                table);
            return boost::text::v1::compare(lhs_sk, rhs_sk);
        }

        template<
            typename CPIter1,
            typename Sentinel1,
            typename CPIter2,
            typename Sentinel2>
        auto collate(
            CPIter1 lhs_first,
            Sentinel1 lhs_last,
            CPIter2 rhs_first,
            Sentinel2 rhs_last,
            collation_strength strength,
            case_first case_1st,
            case_level case_lvl,
            variable_weighting weighting,
            l2_weight_order l2_order,
            collation_table const & table)
            -> detail::cp_iter_ret_t<int, CPIter1>
        {
            auto lhs_u =
                detail::unpack_iterator_and_sentinel(lhs_first, lhs_last);
            auto rhs_u =
                detail::unpack_iterator_and_sentinel(rhs_first, rhs_last);
            return collate_impl(
                lhs_u.tag_,
                lhs_u.f_,
                lhs_u.l_,
                rhs_u.tag_,
                rhs_u.f_,
                rhs_u.l_,
                strength,
                case_1st,
                case_lvl,
                weighting,
                l2_order,
                table);
#if 0 // TODO: This causes a failure in tailoring_g0
            // Looking for a common prefix does not work very well if L2 is
            // backward.
            if (collation_strength::secondary <= strength &&
                l2_order == l2_weight_order::backward) {
                return collate_impl(
                    lhs_first_,
                    lhs_last_,
                    rhs_first_,
                    rhs_last_,
                    strength,
                    case_1st,
                    case_lvl,
                    weighting,
                    l2_order,
                    table);
            }

            // Identical CPs will result in identical CEs, so we can ignore
            // any common prefix.
            auto mismatches = detail::utf_mismatch(
                lhs_first_, lhs_last_, rhs_first_, rhs_last_);

            if (mismatches.first != lhs_last_ && mismatches.second == rhs_last_)
                return 0;

            // Same as the logic in get_collation_elements().
            for (; mismatches.first != lhs_first_ &&
                   mismatches.second != rhs_last_;) {
                --mismatches.first;
                --mismatches.second;
                if (detail::ccc(*mismatches.first) == 0)
                    break;
            }

            return collate_impl(
                mismatches.first,
                lhs_last_,
                mismatches.second,
                rhs_last_,
                strength,
                case_1st,
                case_lvl,
                weighting,
                l2_order,
                table);
#endif
        }
    }

#ifdef BOOST_TEXT_DOXYGEN

    /** Returns a collation sort key for the code points in `[first, last)`,
        using the given collation table.  Any optional settings such as
        `case_1st` will be honored, so long as they do not conflict with the
        settings on the given table.

        Consider using one of the overloads that takes collation_flags
        instead.

        This function only participates in overload resolution if `CPIter`
        models the CPIter concept.

        \pre `[first, last)` is normalized FCC. */
    template<typename CPIter, typename Sentinel>
    text_sort_key collation_sort_key(
        CPIter first,
        Sentinel last,
        collation_table const & table,
        collation_strength strength = collation_strength::tertiary,
        case_first case_1st = case_first::off,
        case_level case_lvl = case_level::off,
        variable_weighting weighting = variable_weighting::non_ignorable,
        l2_weight_order l2_order = l2_weight_order::forward);

    /** Returns a collation sort key for the code points in `[first, last)`,
        using the given collation table.  Any optional settings flags will be
        honored, so long as they do not conflict with the settings on the
        given table.

        This function only participates in overload resolution if `CPIter`
        models the CPIter concept.

        \pre `[first, last)` is normalized FCC. */
    template<typename CPIter, typename Sentinel>
    text_sort_key collation_sort_key(
        CPIter first,
        Sentinel last,
        collation_table const & table,
        collation_flags flags = collation_flags::none);

    /** Returns a collation sort key for the code points in `r`, using the
        given collation table.  Any optional settings flags will be honored,
        so long as they do not conflict with the settings on the given table.

        This function only participates in overload resolution if `CPRange`
        models the CPRange concept.

        \pre r is normalized FCC. */
    template<typename CPRange>
    text_sort_key collation_sort_key(
        CPRange const & r,
        collation_table const & table,
        collation_flags flags = collation_flags::none);

    /** Returns a collation sort key for the graphemes in `r`, using the given
        collation table.  Any optional settings flags will be honored, so long
        as they do not conflict with the settings on the given table.

        This function only participates in overload resolution if
        `GraphemeRange` models the GraphemeRange concept.

        \pre r is normalized FCC. */
    template<typename GraphemeRange>
    text_sort_key collation_sort_key(
        GraphemeRange const & r,
        collation_table const & table,
        collation_flags flags = collation_flags::none);

    /** Creates sort keys for `[lhs_first, lhs_last)` and `[rhs_first,
        rhs_last)`, then returns the result of calling compare() on the
        keys. Any optional settings such as `case_1st` will be honored, so
        long as they do not conflict with the settings on the given table.

        Consider using one of the overloads that takes collation_flags
        instead.

        This function only participates in overload resolution if `CPIter1`
        models the CPIter concept.

        \pre `[lhs_first, lhs_last)` is normalized FCC.
        \pre `[rhs_first, rhs_last)` is normalized FCC. */
    template<
        typename CPIter1,
        typename Sentinel1,
        typename CPIter2,
        typename Sentinel2>
    int collate(
        CPIter1 lhs_first,
        Sentinel1 lhs_last,
        CPIter2 rhs_first,
        Sentinel2 rhs_last,
        collation_table const & table,
        collation_strength strength = collation_strength::tertiary,
        case_first case_1st = case_first::off,
        case_level case_lvl = case_level::off,
        variable_weighting weighting = variable_weighting::non_ignorable,
        l2_weight_order l2_order = l2_weight_order::forward);

    /** Creates sort keys for `[lhs_first, lhs_last)` and `[rhs_first,
        rhs_last)`, then returns the result of calling compare() on the keys.
        Any optional settings flags will be honored, so long as they do not
        conflict with the settings on the given table.

        This function only participates in overload resolution if `CPIter1`
        models the CPIter concept.

        \pre `[lhs_first, lhs_last)` is normalized FCC.
        \pre `[rhs_first, rhs_last)` is normalized FCC. */
    template<
        typename CPIter1,
        typename Sentinel1,
        typename CPIter2,
        typename Sentinel2>
    int collate(
        CPIter1 lhs_first,
        Sentinel1 lhs_last,
        CPIter2 rhs_first,
        Sentinel2 rhs_last,
        collation_table const & table,
        collation_flags flags = collation_flags::none);

    /** Creates sort keys for `r1` and `r2`, then returns the result of
        calling compare() on the keys.  Any optional settings flags will be
        honored, so long as they do not conflict with the settings on the
        given table.

        This function only participates in overload resolution if `CPRange1`
        models the CPRange concept.

        \pre `r1` is normalized FCC.
        \pre `r2` is normalized FCC. */
    template<typename CPRange1, typename CPRange2>
    int collate(
        CPRange1 const & r1,
        CPRange2 const & r2,
        collation_table const & table,
        collation_flags flags = collation_flags::none);

    /** Creates sort keys for `r1` and `r2`, then returns the result of
        calling compare() on the keys.  Any optional settings flags will be
        honored, so long as they do not conflict with the settings on the
        given table.

        This function only participates in overload resolution if
        `GraphemeRange1` models the GraphemeRange concept.

        \pre `r1` is normalized FCC.
        \pre `r2` is normalized FCC. */
    template<typename GraphemeRange1, typename GraphemeRange2>
    int collate(
        GraphemeRange1 const & r1,
        GraphemeRange2 const & r2,
        collation_table const & table,
        collation_flags flags = collation_flags::none);

#else

    template<typename CPIter, typename Sentinel>
    auto collation_sort_key(
        CPIter first,
        Sentinel last,
        collation_table const & table,
        collation_strength strength = collation_strength::tertiary,
        case_first case_1st = case_first::off,
        case_level case_lvl = case_level::off,
        variable_weighting weighting = variable_weighting::non_ignorable,
        l2_weight_order l2_order = l2_weight_order::forward)
        -> detail::cp_iter_ret_t<text_sort_key, CPIter>
    {
        return detail::collation_sort_key(
            first,
            last,
            strength,
            case_1st,
            case_lvl,
            weighting,
            l2_order,
            table);
    }

    template<typename CPIter, typename Sentinel>
    auto collation_sort_key(
        CPIter first,
        Sentinel last,
        collation_table const & table,
        collation_flags flags = collation_flags::none)
        -> detail::cp_iter_ret_t<text_sort_key, CPIter>
    {
        return detail::collation_sort_key(
            first,
            last,
            detail::to_strength(flags),
            detail::to_case_first(flags),
            detail::to_case_level(flags),
            detail::to_weighting(flags),
            detail::to_l2_order(flags),
            table);
    }

    template<typename CPRange>
    auto collation_sort_key(
        CPRange const & r,
        collation_table const & table,
        collation_flags flags = collation_flags::none)
        -> detail::cp_rng_alg_ret_t<text_sort_key, CPRange>
    {
        return detail::collation_sort_key(
            std::begin(r),
            std::end(r),
            detail::to_strength(flags),
            detail::to_case_first(flags),
            detail::to_case_level(flags),
            detail::to_weighting(flags),
            detail::to_l2_order(flags),
            table);
    }

    template<typename GraphemeRange>
    auto collation_sort_key(
        GraphemeRange const & r,
        collation_table const & table,
        collation_flags flags = collation_flags::none)
        -> detail::graph_rng_alg_ret_t<text_sort_key, GraphemeRange>
    {
        return detail::collation_sort_key(
            std::begin(r).base(),
            std::end(r).base(),
            detail::to_strength(flags),
            detail::to_case_first(flags),
            detail::to_case_level(flags),
            detail::to_weighting(flags),
            detail::to_l2_order(flags),
            table);
    }

    template<
        typename CPIter1,
        typename Sentinel1,
        typename CPIter2,
        typename Sentinel2>
    auto collate(
        CPIter1 lhs_first,
        Sentinel1 lhs_last,
        CPIter2 rhs_first,
        Sentinel2 rhs_last,
        collation_table const & table,
        collation_strength strength = collation_strength::tertiary,
        case_first case_1st = case_first::off,
        case_level case_lvl = case_level::off,
        variable_weighting weighting = variable_weighting::non_ignorable,
        l2_weight_order l2_order = l2_weight_order::forward)
        -> detail::cp_iter_ret_t<int, CPIter1>
    {
        return detail::collate(
            lhs_first,
            lhs_last,
            rhs_first,
            rhs_last,
            strength,
            case_1st,
            case_lvl,
            weighting,
            l2_order,
            table);
    }

    template<
        typename CPIter1,
        typename Sentinel1,
        typename CPIter2,
        typename Sentinel2>
    auto collate(
        CPIter1 lhs_first,
        Sentinel1 lhs_last,
        CPIter2 rhs_first,
        Sentinel2 rhs_last,
        collation_table const & table,
        collation_flags flags = collation_flags::none)
        -> detail::cp_iter_ret_t<int, CPIter1>
    {
        return detail::collate(
            lhs_first,
            lhs_last,
            rhs_first,
            rhs_last,
            detail::to_strength(flags),
            detail::to_case_first(flags),
            detail::to_case_level(flags),
            detail::to_weighting(flags),
            detail::to_l2_order(flags),
            table);
    }

    template<typename CPRange1, typename CPRange2>
    auto collate(
        CPRange1 const & r1,
        CPRange2 const & r2,
        collation_table const & table,
        collation_flags flags = collation_flags::none)
        -> detail::cp_rng_alg_ret_t<int, CPRange1>
    {
        return collate(
            std::begin(r1),
            std::end(r1),
            std::begin(r2),
            std::end(r2),
            table,
            detail::to_strength(flags),
            detail::to_case_first(flags),
            detail::to_case_level(flags),
            detail::to_weighting(flags),
            detail::to_l2_order(flags));
    }

    template<typename GraphemeRange1, typename GraphemeRange2>
    auto collate(
        GraphemeRange1 const & r1,
        GraphemeRange2 const & r2,
        collation_table const & table,
        collation_flags flags = collation_flags::none)
        -> detail::graph_rng_alg_ret_t<int, GraphemeRange1>
    {
        return collate(
            std::begin(r1).base(),
            std::end(r1).base(),
            std::begin(r2).base(),
            std::end(r2).base(),
            table,
            detail::to_strength(flags),
            detail::to_case_first(flags),
            detail::to_case_level(flags),
            detail::to_weighting(flags),
            detail::to_l2_order(flags));
    }

#endif

}}}

#include <boost/text/collation_table.hpp>

namespace boost { namespace text { inline namespace v1 { namespace detail {

    template<typename CPIter, typename Sentinel, std::size_t N, std::size_t M>
    CPIter get_collation_elements(
        CPIter first,
        Sentinel last,
        collation_strength strength,
        case_first case_1st,
        case_level case_lvl,
        variable_weighting weighting,
        l2_weight_order l2_order,
        collation_table const & table,
        std::array<uint32_t, N> & buffer,
        typename std::array<uint32_t, N>::iterator & buf_it,
        int & cps,
        container::small_vector<collation_element, M> & ces)
    {
        auto it = first;
        {
            auto u = detail::unpack_iterator_and_sentinel(it, last);
            auto copy_result = detail::transcode_to_32<true>(
                u.tag_, u.f_, u.l_, buffer.end() - buf_it, buf_it);
            it = detail::make_iterator(first, copy_result.iter, last);
            buf_it = copy_result.out;
        }

        // The chunk we pass to S2 should end at the earliest contiguous
        // starter (ccc == 0) we find searching backward from the end.  This
        // is because 1) we don't want to cut off trailing combining
        // characters that may participate in longest-match determination in
        // S2.1, and 2) in S2.3 we need to know if earlier CPs are
        // variable-weighted or not.
        auto s2_it = buf_it;
        if (s2_it == buffer.end()) {
            while (s2_it != buffer.begin()) {
                if (detail::ccc(*--s2_it) == 0)
                    break;
            }
            // TODO: This will produce incorrect results if std::prev(s2_it)
            // points to a CP with variable-weighted CEs.
        }

        auto const end_of_raw_input = std::prev(it, s2_it - buf_it);
        auto const ces_size = ces.size();
        ces.resize(ces_size + M);
        auto ces_end = table.copy_collation_elements(
            buffer.begin(),
            s2_it,
            ces.begin() + ces_size,
            strength,
            case_1st,
            case_lvl,
            weighting);
        ces.resize(ces_end - ces.begin());
        buf_it = std::copy(s2_it, buf_it, buffer.begin());
        first = end_of_raw_input;

        return first;
    }

    template<typename CPIter, typename Sentinel>
    auto collation_sort_key(
        CPIter first,
        Sentinel last,
        collation_strength strength,
        case_first case_1st,
        case_level case_lvl,
        variable_weighting weighting,
        l2_weight_order l2_order,
        collation_table const & table)
        -> detail::cp_iter_ret_t<text_sort_key, CPIter>
    {
        auto const initial_first = first;

        if (table.l2_order())
            l2_order = *table.l2_order();
        if (table.weighting())
            weighting = *table.weighting();
        if (table.case_1st())
            case_1st = *table.case_1st();
        if (table.case_lvl())
            case_lvl = *table.case_lvl();

        std::array<uint32_t, 128> buffer;
        container::small_vector<collation_element, 128 * 10> ces;
        auto buf_it = buffer.begin();
        int cps = 0;
        while (first != last) {
            first = get_collation_elements(
                first,
                last,
                strength,
                case_1st,
                case_lvl,
                weighting,
                l2_order,
                table,
                buffer,
                buf_it,
                cps,
                ces);
        }

        std::vector<uint32_t> bytes;
        detail::s3(ces.begin(),
           ces.end(),
           ces.size(),
           strength,
           case_1st,
           case_lvl,
           l2_order,
           initial_first,
           last,
           cps,
           bytes);

        return text_sort_key(std::move(bytes));
    }

    template<typename CEIter>
    int compare_l2(
        CEIter lhs_first,
        CEIter lhs_last,
        CEIter rhs_first,
        CEIter rhs_last,
        collation_strength strength,
        case_first case_1st,
        case_level case_lvl)
    {
        for (; lhs_first != lhs_last && rhs_first != rhs_last;
             ++lhs_first, ++rhs_first) {
            collation_element & lhs = *lhs_first;
            lhs = detail::modify_for_case(lhs, strength, case_1st, case_lvl);
            collation_element & rhs = *rhs_first;
            rhs = detail::modify_for_case(rhs, strength, case_1st, case_lvl);

            if (lhs.l2_ < rhs.l2_)
                return -1;
            if (rhs.l2_ < lhs.l2_)
                return 1;
        }

        std::transform(
            lhs_first, lhs_last, lhs_first, [=](collation_element ce) {
                return detail::modify_for_case(
                    ce, strength, case_1st, case_lvl);
            });
        std::transform(
            rhs_first, rhs_last, rhs_first, [=](collation_element ce) {
                return detail::modify_for_case(
                    ce, strength, case_1st, case_lvl);
            });

        auto const lhs_at_end = lhs_first == lhs_last;
        auto const rhs_at_end = rhs_first == rhs_last;

        if (lhs_at_end && rhs_at_end)
            return 0;
        if (lhs_at_end) {
            return std::any_of(
                       rhs_first,
                       rhs_last,
                       [](collation_element ce) { return ce.l2_ != 0; })
                       ? -1
                       : 0;
        }
        if (rhs_at_end) {
            return std::any_of(
                       lhs_first,
                       lhs_last,
                       [](collation_element ce) { return ce.l2_ != 0; })
                       ? 1
                       : 0;
        }

        if (lhs_first->l2_ < rhs_first->l2_)
            return -1;
        if (rhs_first->l2_ < lhs_first->l2_)
            return 1;
        return 0;
    }

    template<typename CEIter, typename Proj>
    int compare_l3_or_l4(
        CEIter lhs_first,
        CEIter lhs_last,
        CEIter rhs_first,
        CEIter rhs_last,
        collation_strength strength,
        case_first case_1st,
        case_level case_lvl,
        Proj proj)
    {
        auto const mismatches = algorithm::mismatch(
            lhs_first,
            lhs_last,
            rhs_first,
            rhs_last,
            [=](collation_element lhs, collation_element rhs) {
                return proj(lhs) == proj(rhs);
            });
        auto const lhs_at_end = mismatches.first == lhs_last;
        auto const rhs_at_end = mismatches.second == rhs_last;

        if (lhs_at_end) {
            return std::any_of(
                       mismatches.second,
                       rhs_last,
                       [=](collation_element ce) { return proj(ce) != 0; })
                       ? -1
                       : 0;
        }
        if (rhs_at_end) {
            return std::any_of(
                       mismatches.first,
                       lhs_last,
                       [=](collation_element ce) { return proj(ce) != 0; })
                       ? 1
                       : 0;
        }

        if (proj(*mismatches.first) < proj(*mismatches.second))
            return -1;
        if (proj(*mismatches.second) < proj(*mismatches.first))
            return 1;
        return 0;
    }
        template<typename Iter, typename Sentinel>
        auto collate_impl(
            utf8_tag,
            Iter lhs_first,
            Sentinel lhs_last,
            utf8_tag,
            Iter rhs_first,
            Sentinel rhs_last,
            collation_strength strength,
            case_first case_1st,
            case_level case_lvl,
            variable_weighting weighting,
            l2_weight_order l2_order,
            collation_table const & table)
        {
            auto lhs_it = lhs_first;
            auto rhs_it = rhs_first;

#if 1
            // Looking for a common prefix does not work very well if L2 is
            // backward.
            if (collation_strength::secondary <= strength &&
                l2_order == l2_weight_order::backward) {
                auto const lhs = boost::text::v1::as_utf32(lhs_it, lhs_last);
                auto const rhs = boost::text::v1::as_utf32(rhs_it, rhs_last);
                text_sort_key const lhs_sk = collation_sort_key(
                    lhs.begin(),
                    lhs.end(),
                    strength,
                    case_1st,
                    case_lvl,
                    weighting,
                    l2_order,
                    table);
                text_sort_key const rhs_sk = collation_sort_key(
                    rhs.begin(),
                    rhs.end(),
                    strength,
                    case_1st,
                    case_lvl,
                    weighting,
                    l2_order,
                    table);
                return boost::text::v1::compare(lhs_sk, rhs_sk);
            }

//            std::cout << "CP0\n";

            // This is std::ranges::mismatch(), but I can't use that yet.
            for (; lhs_it != lhs_last && rhs_it != rhs_last;
                 ++lhs_it, ++rhs_it) {
                if (*lhs_it != *rhs_it) {
                    // Back up to the start of the current CP.
                    while (lhs_it != lhs_first) {
                        --lhs_it;
                        --rhs_it;
                        if (!boost::text::v1::continuation(*lhs_it))
                            break;
                    }
                    break;
                }
            }
            if (lhs_it == lhs_last && rhs_it == rhs_last)
                return 0;

//            std::cout << "CP1\n";
#endif

#if 1
            auto const & latin_cache =
                detail::get_latin_cache(table, case_1st, case_lvl, weighting);
            for (; lhs_it != lhs_last && rhs_it != rhs_last;) {
                auto lhs_next = lhs_it;
                auto rhs_next = rhs_it;
                unsigned char const l_c = *lhs_next;
                unsigned char const r_c = *rhs_next;
                uint32_t l, r;
                if (l_c < 0x80) {
                    // TODO: Process 1 <= n <= 16 values here using SIMD.
                    l = l_c;
                    ++lhs_next;
                } else {
                    l = detail::advance(lhs_next, lhs_last);
                }
                if (r_c < 0x80) {
                    // TODO: Process 1 <= n <= 16 values here using SIMD.
                    r = r_c;
                    ++rhs_next;
                } else {
                    r = detail::advance(rhs_next, rhs_last);
                }
                if (collation_latin_cache::size <= l ||
                    collation_latin_cache::size <= r) {
                    break;
                }
                auto l_ces = latin_cache[l];
                auto r_ces = latin_cache[r];
                auto const mismatches = algorithm::mismatch(
                    l_ces.begin(), l_ces.end(), r_ces.begin(), r_ces.end());
                auto const l_at_end = mismatches.first == l_ces.end();
                auto const r_at_end = mismatches.second == r_ces.end();
                if (l_at_end && std::any_of(
                                    mismatches.second,
                                    r_ces.end(),
                                    [](detail::collation_element const & ce) {
                                        return ce.l1_;
                                    })) {
                    return -1;
                }
                if (r_at_end && std::any_of(
                                    mismatches.first,
                                    l_ces.end(),
                                    [](detail::collation_element const & ce) {
                                        return ce.l1_;
                                    })) {
                    return 1;
                }
                lhs_it = lhs_next;
                rhs_it = rhs_next;
            }
            if (lhs_it == lhs_last && rhs_it == rhs_last)
                return 0;

//            std::cout << "CP2\n";
#endif

            uint32_t const merge_separator_primary = 0x02000000;

            auto const lhs = boost::text::v1::as_utf32(lhs_it, lhs_last);
            auto const rhs = boost::text::v1::as_utf32(rhs_it, rhs_last);
            text_sort_key const lhs_sk = collation_sort_key(
                lhs.begin(),
                lhs.end(),
                strength,
                case_1st,
                case_lvl,
                weighting,
                l2_order,
                table);
            text_sort_key const rhs_sk = collation_sort_key(
                rhs.begin(),
                rhs.end(),
                strength,
                case_1st,
                case_lvl,
                weighting,
                l2_order,
                table);

//            std::cout << "CP3\n";

            return boost::text::v1::compare(lhs_sk, rhs_sk);
        }

    template<
        typename CPIter1,
        typename Sentinel1,
        typename CPIter2,
        typename Sentinel2>
    int collate_impl(
        CPIter1 lhs_first,
        Sentinel1 lhs_last,
        CPIter2 rhs_first,
        Sentinel2 rhs_last,
        collation_strength strength,
        case_first case_1st,
        case_level case_lvl,
        variable_weighting weighting,
        l2_weight_order l2_order,
        collation_table const & table)
    {
        if (table.l2_order())
            l2_order = *table.l2_order();
        if (table.weighting())
            weighting = *table.weighting();
        if (table.case_1st())
            case_1st = *table.case_1st();
        if (table.case_lvl())
            case_lvl = *table.case_lvl();

        std::array<uint32_t, 128> lhs_buffer;
        std::array<uint32_t, 128> rhs_buffer;
        container::small_vector<collation_element, 128 * 10> lhs_ces;
        container::small_vector<collation_element, 128 * 10> rhs_ces;
        auto lhs_buf_it = lhs_buffer.begin();
        auto rhs_buf_it = rhs_buffer.begin();
        int lhs_cps = 0;
        int rhs_cps = 0;

        // Find and compare the first nonignorable CEs on the left and right.
        // Keep doing this one buffer-chunk at a time as long as the CEs are
        // all the same.
        std::ptrdiff_t last_ce_index = 0;
        while (lhs_first != lhs_last && rhs_first != rhs_last) {
            lhs_first = get_collation_elements(
                lhs_first,
                lhs_last,
                strength,
                case_1st,
                case_lvl,
                weighting,
                l2_order,
                table,
                lhs_buffer,
                lhs_buf_it,
                lhs_cps,
                lhs_ces);
            rhs_first = get_collation_elements(
                rhs_first,
                rhs_last,
                strength,
                case_1st,
                case_lvl,
                weighting,
                l2_order,
                table,
                rhs_buffer,
                rhs_buf_it,
                rhs_cps,
                rhs_ces);

            auto const mismatches = algorithm::mismatch(
                lhs_ces.begin() + last_ce_index,
                lhs_ces.end(),
                rhs_ces.begin() + last_ce_index,
                rhs_ces.end(),
                [](collation_element lhs, collation_element rhs) {
                    return lhs.l1_ == rhs.l1_;
                });
            auto const lhs_at_end = mismatches.first == lhs_ces.end();
            auto const rhs_at_end = mismatches.second == rhs_ces.end();
            if (lhs_at_end && rhs_at_end) {
                last_ce_index = (std::min)(lhs_ces.size(), rhs_ces.size());
                continue;
            }
            if (!lhs_at_end && !rhs_at_end)
                return mismatches.first->l1_ < mismatches.second->l1_ ? -1 : 1;
        }

        // Now we're at the end of the common prefix of both sides, but there
        // may be some CPs remaining on the left or right for which we have
        // not gotten the CEs.

        // If there are extra CEs on the left and at least one of them is
        // non-ignorable, return 1.
        while (lhs_first != lhs_last && rhs_first != rhs_last) {
            lhs_first = get_collation_elements(
                lhs_first,
                lhs_last,
                strength,
                case_1st,
                case_lvl,
                weighting,
                l2_order,
                table,
                lhs_buffer,
                lhs_buf_it,
                lhs_cps,
                lhs_ces);
            if (std::any_of(
                    lhs_ces.begin() + last_ce_index,
                    lhs_ces.end(),
                    [](collation_element ce) { return ce.l1_ != 0; })) {
                return 1;
            }
            last_ce_index = lhs_ces.size();
        }

        // If there are extra CEs on the right and at least one of them is
        // non-ignorable, return -1.
        while (rhs_first != rhs_last && rhs_first != rhs_last) {
            rhs_first = get_collation_elements(
                rhs_first,
                rhs_last,
                strength,
                case_1st,
                case_lvl,
                weighting,
                l2_order,
                table,
                rhs_buffer,
                rhs_buf_it,
                rhs_cps,
                rhs_ces);
            if (std::any_of(
                    rhs_ces.begin() + last_ce_index,
                    rhs_ces.end(),
                    [](collation_element ce) { return ce.l1_ != 0; })) {
                return -1;
            }
            last_ce_index = rhs_ces.size();
        }

        // Now we have all the CEs, and they're equal within the common
        // prefix, and possibly one side or the other is longer, but the extra
        // length is all primary-ignorable.

        if (strength == collation_strength::primary)
            return 0;

        int const l2_comparison = l2_order == l2_weight_order::forward
                                      ? compare_l2(
                                            lhs_ces.begin(),
                                            lhs_ces.end(),
                                            rhs_ces.begin(),
                                            rhs_ces.end(),
                                            strength,
                                            case_1st,
                                            case_lvl)
                                      : compare_l2(
                                            lhs_ces.rbegin(),
                                            lhs_ces.rend(),
                                            rhs_ces.rbegin(),
                                            rhs_ces.rend(),
                                            strength,
                                            case_1st,
                                            case_lvl);
        if (l2_comparison)
            return l2_comparison;

        if (strength == collation_strength::secondary)
            return 0;

        int const l3_comparison = compare_l3_or_l4(
            lhs_ces.begin(),
            lhs_ces.end(),
            rhs_ces.begin(),
            rhs_ces.end(),
            strength,
            case_1st,
            case_lvl,
            [](collation_element ce) { return ce.l3_; });
        if (l3_comparison)
            return l3_comparison;

        if (strength == collation_strength::tertiary)
            return 0;

        int const l4_comparison = compare_l3_or_l4(
            lhs_ces.begin(),
            lhs_ces.end(),
            rhs_ces.begin(),
            rhs_ces.end(),
            strength,
            case_1st,
            case_lvl,
            [](collation_element ce) { return ce.l4_; });
        if (l4_comparison)
            return l4_comparison;

        if (strength == collation_strength::quaternary)
            return 0;

        string lhs_str;
        boost::text::v1::normalize_to_nfd_append_utf8(
            lhs_first, lhs_last, lhs_str);
        string rhs_str;
        boost::text::v1::normalize_to_nfd_append_utf8(
            rhs_first, rhs_last, rhs_str);

        auto const lhs_str_utf32 = as_utf32(lhs_str);
        auto const rhs_str_utf32 = as_utf32(rhs_str);
        auto const mismatches = algorithm::mismatch(
            lhs_str_utf32.begin(),
            lhs_str_utf32.end(),
            rhs_str_utf32.begin(),
            rhs_str_utf32.end());
        auto const lhs_at_end = mismatches.first == lhs_str_utf32.end();
        auto const rhs_at_end = mismatches.second == rhs_str_utf32.end();

        if (lhs_at_end && rhs_at_end)
            return 0;
        if (lhs_at_end)
            return 1;
        if (rhs_at_end)
            return -1;

        if (*mismatches.first < *mismatches.second)
            return -1;
        if (*mismatches.second < *mismatches.first)
            return 1;
        return 0;
    }

}}}}

#ifndef BOOST_TEXT_DOXYGEN

namespace std {
    template<>
    struct hash<boost::text::v1::text_sort_key>
    {
        using argument_type = boost::text::v1::text_sort_key;
        using result_type = std::size_t;
        result_type operator()(argument_type const & key) const noexcept
        {
            return std::accumulate(
                key.begin(),
                key.end(),
                result_type(key.size()),
                boost::text::v1::detail::hash_combine_);
        }
    };
}

#endif

#endif
