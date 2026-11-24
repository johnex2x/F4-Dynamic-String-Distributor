#pragma once

// Hangul syllables U+AC00 to U+D7A3, in UTF-8 (3 bytes: EA B0 80 .. ED 9E A3)
[[nodiscard]] inline bool HasHangul(std::string_view a_text) noexcept
{
	for (std::size_t i = 0; i + 2 < a_text.size(); ++i) {
		const auto b0 = static_cast<unsigned char>(a_text[i]);
		if (b0 < 0xE0 || b0 > 0xEF) {
			continue;
		}
		const auto b1 = static_cast<unsigned char>(a_text[i + 1]);
		const auto b2 = static_cast<unsigned char>(a_text[i + 2]);
		if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) {
			continue;
		}
		const auto cp = (static_cast<std::uint32_t>(b0 & 0x0F) << 12) | (static_cast<std::uint32_t>(b1 & 0x3F) << 6) | (b2 & 0x3F);
		if (cp >= 0xAC00 && cp <= 0xD7A3) {
			return true;
		}
	}
	return false;
}