#pragma once

// Minimal reader for the records a plugin defines itself (internal to dsd_core).
// Fallout 4 plugin layout: a TES4 header record, then top-level groups; a record or group header is 24 bytes.

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <istream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/unordered/unordered_flat_set.hpp>
#include <zlib.h>

namespace DSD::PluginFile
{
	constexpr std::size_t   kHeaderSize = 24;  // record and group headers have the same size
	constexpr std::uint32_t kCompressed = 0x00040000;
	constexpr std::uint32_t kLocalized = 0x00000080;  // TES4 flag: strings live in string tables

	struct Header
	{
		std::array<char, 4> signature{};
		std::uint32_t       size{ 0 };    // record: data size; group: total size including this header
		std::uint32_t       flags{ 0 };   // record: flags; group: label
		std::uint32_t       formID{ 0 };  // record: form ID; group: type

		[[nodiscard]] bool Is(std::string_view a_sig) const noexcept { return std::string_view{ signature.data(), 4 } == a_sig; }
	};

	template <class T>
	[[nodiscard]] T Read(const std::uint8_t* a_data) noexcept
	{
		T value;
		std::memcpy(&value, a_data, sizeof(T));
		return value;
	}

	[[nodiscard]] inline Header ParseHeader(const std::uint8_t* a_data) noexcept
	{
		Header header;
		std::memcpy(header.signature.data(), a_data, 4);
		header.size = Read<std::uint32_t>(a_data + 4);
		header.flags = Read<std::uint32_t>(a_data + 8);
		header.formID = Read<std::uint32_t>(a_data + 12);
		return header;
	}

	[[nodiscard]] inline bool ReadHeader(std::istream& a_in, Header& a_header)
	{
		std::array<std::uint8_t, kHeaderSize> buffer{};
		if (!a_in.read(reinterpret_cast<char*>(buffer.data()), buffer.size())) {
			return false;
		}
		a_header = ParseHeader(buffer.data());
		return true;
	}

	[[nodiscard]] inline std::vector<std::uint8_t> ReadBytes(std::istream& a_in, std::size_t a_size)
	{
		std::vector<std::uint8_t> data(a_size);
		if (a_size && !a_in.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(a_size))) {
			throw std::runtime_error("unexpected end of plugin file");
		}
		return data;
	}

	// Record body, inflated when the record is compressed
	[[nodiscard]] inline std::vector<std::uint8_t> RecordData(const Header& a_header, const std::uint8_t* a_data)
	{
		if (!(a_header.flags & kCompressed)) {
			return { a_data, a_data + a_header.size };
		}
		if (a_header.size < 4) {
			throw std::runtime_error("compressed record too small");
		}

		uLongf                    size = Read<std::uint32_t>(a_data);
		std::vector<std::uint8_t> data(size);
		if (uncompress(data.data(), &size, a_data + 4, a_header.size - 4) != Z_OK) {
			throw std::runtime_error("cannot inflate compressed record");
		}
		data.resize(size);
		return data;
	}

	// Calls a_visitor(signature, data, size) for each subrecord; handles XXXX (size of the next subrecord)
	template <class F>
	void ForEachSubrecord(const std::vector<std::uint8_t>& a_data, F a_visitor)
	{
		std::size_t                  pos = 0;
		std::optional<std::uint32_t> bigSize;
		while (pos + 6 <= a_data.size()) {
			const std::string_view signature{ reinterpret_cast<const char*>(a_data.data() + pos), 4 };
			std::size_t            size = Read<std::uint16_t>(a_data.data() + pos + 4);
			pos += 6;
			if (signature == "XXXX" && size == 4 && pos + 4 <= a_data.size()) {
				bigSize = Read<std::uint32_t>(a_data.data() + pos);
				pos += size;
				continue;
			}
			if (bigSize) {
				size = *std::exchange(bigSize, std::nullopt);
			}
			if (pos + size > a_data.size()) {
				throw std::runtime_error("subrecord runs past the record");
			}
			a_visitor(signature, a_data.data() + pos, size);
			pos += size;
		}
	}

	[[nodiscard]] inline std::uint32_t CountMasters(const std::vector<std::uint8_t>& a_tes4)
	{
		std::uint32_t masters = 0;
		ForEachSubrecord(a_tes4, [&](std::string_view a_sig, const std::uint8_t*, std::size_t) {
			if (a_sig == "MAST") {
				++masters;
			}
		});
		return masters;
	}

	// Calls a_visitor(low 24 bits of the form ID, record body) for each record of type a_type that the plugin
	// itself defines (not overrides of its masters). a_wanted: low 24 bits to visit; empty visits every record.
	// Child groups inside the type's group are skipped. Throws std::runtime_error on a malformed file.
	// a_localized (optional): set before the first visit to whether the plugin is localized (TES4 flag 0x80).
	template <class F>
	void ForEachOwnRecord(std::istream& a_plugin, std::string_view a_type, std::span<const std::uint32_t> a_wanted, F a_visitor, bool* a_localized = nullptr)
	{
		const boost::unordered_flat_set<std::uint32_t> wanted(a_wanted.begin(), a_wanted.end());

		Header header;
		if (!ReadHeader(a_plugin, header) || !header.Is("TES4")) {
			throw std::runtime_error("not a plugin file (no TES4 header)");
		}
		if (a_localized) {
			*a_localized = (header.flags & kLocalized) != 0;
		}
		const auto tes4 = ReadBytes(a_plugin, header.size);
		const auto masters = CountMasters(RecordData(header, tes4.data()));

		while (ReadHeader(a_plugin, header)) {
			if (!header.Is("GRUP") || header.size < kHeaderSize) {
				throw std::runtime_error("expected a top-level group");
			}

			const auto groupData = header.size - kHeaderSize;
			const bool typeGroup = header.formID == 0 && a_type.size() == 4 && std::memcmp(&header.flags, a_type.data(), 4) == 0;
			if (!typeGroup) {
				a_plugin.seekg(groupData, std::ios::cur);
				continue;
			}

			const auto group = ReadBytes(a_plugin, groupData);
			for (std::size_t pos = 0; pos + kHeaderSize <= group.size();) {
				const auto record = ParseHeader(group.data() + pos);
				if (record.Is("GRUP")) {
					pos += std::max<std::size_t>(record.size, kHeaderSize);
					continue;
				}

				const auto end = pos + kHeaderSize + record.size;
				if (end > group.size()) {
					throw std::runtime_error("record runs past its group");
				}
				const auto low = record.formID & 0x00FFFFFF;
				if (record.Is(a_type) && (record.formID >> 24) == masters && (wanted.empty() || wanted.contains(low))) {
					a_visitor(low, RecordData(record, group.data() + pos + kHeaderSize));
				}
				pos = end;
			}
			break;  // one group per type and plugin
		}
	}
}
