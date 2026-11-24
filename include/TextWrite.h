#pragma once

// Writes into string fields only when they hold something else. The apply passes run again after every
// save load, and by then nearly every field already holds the translation; comparing is far cheaper than
// re-interning the string in the game's string pool.
namespace TextWrite
{
	// A BGSLocalizedString that still holds a "<ID=...>" prefix keeps it when assigned (CommonLibF4 BSFixedString.h,
	// BGSLocalizedString::operator=; the buffer it builds also counts the terminating NUL), so compare behind the prefix
	[[nodiscard]] inline bool Holds(const RE::BGSLocalizedString& a_field, std::string_view a_text)
	{
		constexpr std::size_t kPrefixLength = 13;  // BGSLocalizedString::PREFIX_LENGTH (private)
		const std::string_view current = a_field;
		if (current == a_text) {
			return true;
		}
		if (current.starts_with("<ID=") && current.size() >= kPrefixLength) {
			auto tail = current.substr(kPrefixLength);
			if (tail.ends_with('\0')) {
				tail.remove_suffix(1);
			}
			return tail == a_text;
		}
		return false;
	}

	[[nodiscard]] inline bool Holds(const RE::BSFixedStringCS& a_field, std::string_view a_text)
	{
		return static_cast<std::string_view>(a_field) == a_text;
	}

	// Returns true when the field was written, false when it already held a_text
	template <class Field>
	bool Assign(Field& a_field, const std::string& a_text)
	{
		if (Holds(a_field, a_text)) {
			return false;
		}
		a_field = a_text;
		return true;
	}
}
