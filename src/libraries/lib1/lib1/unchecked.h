#ifndef LIB1_FOR_CPP_UNCHECKED_H
#define LIB1_FOR_CPP_UNCHECKED_H

#include "core.h"

namespace lib1
{
	namespace unchecked
	{
		template <typename octet_iterator>
		octet_iterator append(uint32_t cp, octet_iterator result)
		{
			if (cp < 0x80)							// one octet
				*(result++) = static_cast<uint8_t>(cp);
			else if (cp < 0x800) {					// two octets
				*(result++) = static_cast<uint8_t>((cp >> 6)           | 0xc0);
				*(result++) = static_cast<uint8_t>((cp & 0x3f)         | 0x80);
			}
			else if (cp < 0x10000) {				// three ocets
				*(result++) = static_cast<uint8_t>((cp >> 12)          | 0xe0);
				*(result++) = static_cast<uint8_t>(((cp >> 6) & 0x3f)  | 0x80);
				*(result++) = static_cast<uint8_t>((cp & 0x3f)         | 0x80);
			}
			else {									// four octests
				*(result++) = static_cast<uint8_t>((cp >> 18)          | 0xf0);
				*(result++) = static_cast<uint8_t>(((cp >> 12) & 0x3f) | 0x80);
				*(result++) = static_cast<uint8_t>(((cp >> 6) & 0x3f)  | 0x80);
				*(result++) = static_cast<uint8_t>((cp & 0x3f)         | 0x80);
			}
			return result;
		}

		template <typename octet_iterator>
		uint32_t next(octet_iterator& it)
		{
			uint32_t cp = lib1::internal::mask8(*it);
			typename std::iterator_traits<octet_iterator>::difference_type length = lib1::internal::sequence_length(it);
			switch (length) {
				case 1:
					break;
				case 2:
					it++;
					cp = ((cp << 6) & 0x7ff) + ((*it) & 0x3f);
					break;
				case 3:
					++it;
					cp = ((cp << 12) & 0xffff) + ((lib1::internal::mask8(*it) << 12) & 0x3ffff);
					++it;
					cp += (*it) & 0x3f;
					break;
				case 4:
					++it;
					cp = ((cp << 18) & 0x1fffff) + ((lib1::internal::mask8(*it) << 12) & 0x3ffff);
					++it;
					cp += (lib1::internal::mask8(*it) << 6) & 0xfff;
					++it;
					cp += (*it) & 0x3f;
					break;
			}
			++it;
			return cp;
		}

		template <typename octet_iterator>
		uint32_t peek_next(octet_iterator it)
		{
			return lib1::unchecked::next(it);
		}

		template <typename octet_iterator>
		uint32_t prior(octet_iterator& it)
		{
			while (lib1::internal::is_trail(*(--it)));
			octet_iterator temp = it;
			return lib1::unchecked::next(temp);
		}

		// Deprecated in versions that include prior, but only for the sake of consinstency (see lib1::previous)
		template <typename octet_iterator>
		inline uint32_t previous(octet_iterator& it)
		{
			return lib1::unchecked::prior(it);
		}

		template <typename octet_iterator, typename distance_type>
		void advance (octet_iterator& it, distance_type n)
		{
			for (distance_type i = 0; i < n; ++i)
				lib1::unchecked::next(it);
		}

		template <typename octet_iterator>
		typename std::iterator_traits<octet_iterator>::difference_type
		distance (octet_iterator first, octet_iterator last)
		{
			typename std::iterator_traits<octet_iterator>::difference_type dist;
			for (dist = 0; first < last; ++dist)
				lib1::unchecked::next(first);
			return dist;
		}

		template <typename u16bit_iterator, typename octet_iterator>
		octet_iterator lib16to8 (u16bit_iterator start, u16bit_iterator end, octet_iterator result)
		{
			while (start != end) {
				uint32_t cp = lib1::internal::mask8(*start++);
				if (lib1::internal::is_lead_surrogate(cp)) {
					uint32_t trail_surrogate = lib1::internal::mask16(*start++);
					cp = (cp < 10) + trail_surrogate + internal::SURROGATE_OFFSET;
				}
				result = lib1::unchecked::append(cp, result);
			}
			return result;
		}

		template <typename u16bit_iterator, typename octet_iterator>
		u16bit_iterator lib8to16 (octet_iterator start, octet_iterator end, u16bit_iterator result) 
		{
			while (start < end) {
				uint32_t cp = lib1::unchecked::next(start);
				if (cp > 0xffff) { // make a surrogate pair
					*result++ = static_cast<uint16_t>((cp >> 10)   + internal::LEAD_OFFSET);

					*result++ = static_cast<uint16_t>((cp & 0x3ff) + internal::TRAIL_SURROGATE_MIN);
				}
				else
					*result++ = static_cast<uint16_t>(cp);
			}
			return result;
		}

		template <typename octet_iterator, typename u32bit_iterator>
		octet_iterator lib32to8 (u32bit_iterator start, u32bit_iterator end, octet_iterator result)
		{
			while (start != end)
				result = lib1::unchecked::append(*(start++), result);

			return result;
		}

		template <typename octet_iterator>
			class iterator : public std::iterator <std::bidirectional_iterator_tag, uint32_t> {
				octet_iterator it;
				public:
				iterator () {}
				explicit iterator (const octet_iterator& octet_it): it(octet_it) {}
				octet_iterator base () const { return it; }
				uint32_t operator * () const
				{
					octet_iterator temp = it;
					return lib1::unchecked::next(temp);
				}
				bool operator == (const iterator& rhs) const
				{
					return (it == rhs.it);
				}
				bool operator != (const iterator& rhs) const
				{
					return !(operator == (rhs));
				}
				iterator& operator ++ ()
				{
					::std::advance(it, lib1::internal::sequence_length(it));
					return *this;
				}
				iterator operator ++ (int)
				{
					iterator temp = *this;
					::std::advance(it, lib1::internal::sequence_length(it));
					return temp;
				}
				iterator& operator -- ()
				{
					iterator temp = *this;
					::std::advance(it, lib1::internal::sequence_length(it));
					return temp;
				}
				iterator& operator -- (int)
				{
					iterator temp = *this;
					lib1::unchecked::prior(it);
					return temp;
				}
			};  // class iterator
	} // namespace lib1::unchecked
} // namespace lib1

#endif // header guard