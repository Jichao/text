#ifndef BOOST_TEXT_GRAPHEME_ITERATOR_HPP
#define BOOST_TEXT_GRAPHEME_ITERATOR_HPP

#include <boost/text/config.hpp>
#include <boost/text/grapheme.hpp>
#include <boost/text/grapheme_break.hpp>

#include <boost/stl_interfaces/iterator_interface.hpp>

#include <boost/assert.hpp>

#include <iterator>
#include <type_traits>
#include <stdexcept>


namespace boost { namespace text {

    /** A bidirectional filtering iterator that iterates over the extended
        grapheme clusters in a sequence of code points. */
    template<typename CPIter, typename Sentinel = CPIter>
    struct grapheme_iterator : stl_interfaces::iterator_interface<
                                   grapheme_iterator<CPIter, Sentinel>,
                                   std::bidirectional_iterator_tag,
                                   grapheme_ref<CPIter>,
                                   grapheme_ref<CPIter>,
                                   grapheme_ref<CPIter> const *>
    {
        using iterator_type = CPIter;

        static_assert(
            detail::is_cp_iter<CPIter>::value,
            "CPIter must be a code point iterator");
        static_assert(
            std::is_same<
                typename std::iterator_traits<CPIter>::iterator_category,
                std::bidirectional_iterator_tag>::value ||
                std::is_same<
                    typename std::iterator_traits<CPIter>::iterator_category,
                    std::random_access_iterator_tag>::value,
            "grapheme_iterator requires its CPIter parameter to be at least "
            "bidirectional.");

        grapheme_iterator() noexcept : grapheme_{} {}

        grapheme_iterator(CPIter first, CPIter it, Sentinel last) noexcept :
            grapheme_{it, next_grapheme_break(it, last)},
            first_(first),
            last_(last)
        {}

        template<typename CPIter2, typename Sentinel2>
        grapheme_iterator(grapheme_iterator<CPIter2, Sentinel2> const & other) :
            grapheme_(other.grapheme_.begin(), other.grapheme_.end()),
            first_(other.first_),
            last_(other.last_)
        {}

        grapheme_ref<CPIter> operator*() const noexcept { return grapheme_; }
        grapheme_ref<CPIter> const * operator->() const noexcept
        {
            return &grapheme_;
        }

        grapheme_iterator & operator++() noexcept
        {
            auto const first = grapheme_.end();
            grapheme_ =
                grapheme_ref<CPIter>(first, next_grapheme_break(first, last_));
            return *this;
        }

        grapheme_iterator & operator--() noexcept
        {
            auto const last = grapheme_.begin();
            grapheme_ = grapheme_ref<CPIter>(
                prev_grapheme_break(first_, std::prev(last), last_), last);
            return *this;
        }

        CPIter base() const noexcept { return grapheme_.begin(); }

        using base_type = stl_interfaces::iterator_interface<
            grapheme_iterator<CPIter, Sentinel>,
            std::bidirectional_iterator_tag,
            grapheme_ref<CPIter>,
            grapheme_ref<CPIter>,
            grapheme_ref<CPIter> const *>;
        using base_type::operator++;
        using base_type::operator--;

    private:
        grapheme_ref<CPIter> grapheme_;
        CPIter first_;
        Sentinel last_;

        template<typename CPIter2, typename Sentinel2>
        friend struct grapheme_iterator;
    };

    template<
        typename CPIter1,
        typename Sentinel1,
        typename CPIter2,
        typename Sentinel2>
    auto operator==(
        grapheme_iterator<CPIter1, Sentinel1> lhs,
        grapheme_iterator<CPIter2, Sentinel2> rhs) noexcept
        -> decltype(lhs.base() == rhs.base())
    {
        return lhs.base() == rhs.base();
    }

    /** This function is constexpr in C++14 and later. */
    template<typename CPIter, typename Sentinel>
    BOOST_TEXT_CXX14_CONSTEXPR auto
    operator==(grapheme_iterator<CPIter, Sentinel> it, Sentinel s) noexcept
        -> decltype(it.base() == s)
    {
        return it.base() == s;
    }

    /** This function is constexpr in C++14 and later. */
    template<typename CPIter, typename Sentinel>
    BOOST_TEXT_CXX14_CONSTEXPR auto
    operator==(Sentinel s, grapheme_iterator<CPIter, Sentinel> it) noexcept
        -> decltype(it.base() == s)
    {
        return it.base() == s;
    }

    /** This function is constexpr in C++14 and later. */
    template<typename CPIter, typename Sentinel>
    BOOST_TEXT_CXX14_CONSTEXPR auto
    operator!=(grapheme_iterator<CPIter, Sentinel> it, Sentinel s) noexcept
        -> decltype(it.base() != s)
    {
        return it.base() != s;
    }

    /** This function is constexpr in C++14 and later. */
    template<typename CPIter, typename Sentinel>
    BOOST_TEXT_CXX14_CONSTEXPR auto
    operator!=(Sentinel s, grapheme_iterator<CPIter, Sentinel> it) noexcept
        -> decltype(it.base() != s)
    {
        return it.base() != s;
    }

}}

#endif
