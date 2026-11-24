#include "HookUtil.h"

// Small x64 instruction-length decoder for function prologues.
// Adapted from FallHook RuntimePrologueHook.cpp (GPL-3.0, https://github.com/Agent69-stack/FallHook).
namespace
{
	constexpr std::size_t kMaxPatchBytes = 16;
	constexpr std::size_t kJumpSize = 5;
	constexpr std::size_t kAbsoluteJumpSize = 14;  // FF 25 00000000 <u64>

	struct Decoded
	{
		std::size_t length{ 0 };
		bool        relative{ false };  // RIP-relative or branch: cannot be copied as-is
	};

	[[nodiscard]] bool HasModRM(std::uint8_t a_op) noexcept
	{
		switch (a_op) {
		case 0x00: case 0x01: case 0x02: case 0x03: case 0x08: case 0x09: case 0x0A: case 0x0B:
		case 0x10: case 0x11: case 0x12: case 0x13: case 0x18: case 0x19: case 0x1A: case 0x1B:
		case 0x20: case 0x21: case 0x22: case 0x23: case 0x28: case 0x29: case 0x2A: case 0x2B:
		case 0x30: case 0x31: case 0x32: case 0x33: case 0x38: case 0x39: case 0x3A: case 0x3B:
		case 0x63: case 0x69: case 0x6B: case 0x80: case 0x81: case 0x83: case 0x84: case 0x85:
		case 0x86: case 0x87: case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D:
		case 0x8E: case 0x8F: case 0xC0: case 0xC1: case 0xC6: case 0xC7: case 0xD0: case 0xD1:
		case 0xD2: case 0xD3: case 0xF6: case 0xF7: case 0xFE: case 0xFF:
			return true;
		default:
			return false;
		}
	}

	[[nodiscard]] bool HasModRM0F(std::uint8_t a_op) noexcept
	{
		return (a_op >= 0x10 && a_op <= 0x7F) || (a_op >= 0x90 && a_op <= 0x9F) ||
		       (a_op >= 0xA3 && a_op <= 0xAF) || (a_op >= 0xB0 && a_op <= 0xBF) || a_op == 0x1F;
	}

	[[nodiscard]] std::optional<Decoded> DecodeInstruction(const std::uint8_t* a_code, std::size_t a_max)
	{
		std::size_t i = 0;
		bool        rexW = false;
		for (;; ++i) {
			if (i >= a_max) {
				return std::nullopt;
			}
			const auto prefix = a_code[i];
			if (prefix >= 0x40 && prefix <= 0x4F) {
				rexW = (prefix & 0x08) != 0;
				continue;
			}
			if (prefix == 0x66 || prefix == 0x67 || prefix == 0xF2 || prefix == 0xF3 ||
				prefix == 0x2E || prefix == 0x36 || prefix == 0x3E || prefix == 0x26 ||
				prefix == 0x64 || prefix == 0x65) {
				continue;
			}
			break;
		}

		const auto op = a_code[i++];
		Decoded    result{ .length = i };
		const auto withImmediate = [&](std::size_t a_size) -> std::optional<Decoded> {
			result.length = i + a_size;
			return result.length <= a_max ? std::optional{ result } : std::nullopt;
		};

		if ((op >= 0x50 && op <= 0x5F) || op == 0x90 || op == 0xC3 || op == 0xCC) {
			return result;
		}
		if (op == 0x6A || (op >= 0xB0 && op <= 0xB7)) {
			return withImmediate(1);
		}
		if (op == 0x68) {
			return withImmediate(4);
		}
		if (op >= 0xB8 && op <= 0xBF) {
			return withImmediate(rexW ? 8 : 4);
		}
		if (op == 0xE8 || op == 0xE9 || op == 0xEB || (op >= 0x70 && op <= 0x7F)) {
			result.relative = true;
			return result;
		}

		if (op == 0x0F) {
			if (i >= a_max) {
				return std::nullopt;
			}
			const auto op2 = a_code[i++];
			if (op2 >= 0x80 && op2 <= 0x8F) {
				result.relative = true;
				return withImmediate(4);
			}
			if (!HasModRM0F(op2)) {
				return std::nullopt;
			}
		} else if (!HasModRM(op)) {
			return std::nullopt;
		}

		// ModRM (+ SIB + displacement)
		if (i >= a_max) {
			return std::nullopt;
		}
		const auto modRM = a_code[i++];
		const auto mod = static_cast<std::uint8_t>(modRM >> 6);
		const auto rm = static_cast<std::uint8_t>(modRM & 0x7);
		std::uint8_t sibBase = 0;
		const bool   hasSib = mod != 0x3 && rm == 0x4;
		if (hasSib) {
			if (i >= a_max) {
				return std::nullopt;
			}
			sibBase = static_cast<std::uint8_t>(a_code[i++] & 0x7);
		}
		if (mod == 0x0) {
			if (rm == 0x5) {
				result.relative = true;  // [rip + disp32]
				i += 4;
			} else if (hasSib && sibBase == 0x5) {
				i += 4;  // [index*scale + disp32]
			}
		} else if (mod == 0x1) {
			i += 1;
		} else if (mod == 0x2) {
			i += 4;
		}

		switch (op) {
		case 0x80: case 0x83: case 0xC0: case 0xC1: case 0xC6: case 0x6B:
			i += 1;
			break;
		case 0x69: case 0x81: case 0xC7:
			i += 4;
			break;
		case 0xF6:
			i += ((modRM >> 3) & 0x7) == 0 ? 1 : 0;
			break;
		case 0xF7:
			i += ((modRM >> 3) & 0x7) == 0 ? 4 : 0;
			break;
		default:
			break;
		}

		result.length = i;
		return i <= a_max ? std::optional{ result } : std::nullopt;
	}

	[[nodiscard]] std::optional<std::size_t> PatchLength(const std::uint8_t* a_code)
	{
		std::size_t length = 0;
		while (length < kJumpSize) {
			const auto decoded = DecodeInstruction(a_code + length, kMaxPatchBytes - length);
			if (!decoded || decoded->length == 0 || decoded->relative) {
				return std::nullopt;
			}
			length += decoded->length;
		}
		return length;
	}

	[[nodiscard]] std::string FormatBytes(const std::uint8_t* a_bytes, std::size_t a_count)
	{
		std::string out;
		for (std::size_t i = 0; i < a_count; ++i) {
			out += std::format("{}{:02X}", i ? " " : "", a_bytes[i]);
		}
		return out;
	}
}

namespace HookUtil
{
	namespace
	{
		[[nodiscard]] bool InSegment(REL::Segment::Name a_name, std::uintptr_t a_address, std::size_t a_size)
		{
			const auto segment = REL::Module::get().segment(a_name);
			return a_address >= segment.address() && a_size <= segment.size() && a_address - segment.address() <= segment.size() - a_size;
		}

		[[nodiscard]] bool InImageData(std::uintptr_t a_address, std::size_t a_size)
		{
			return InSegment(REL::Segment::rdata, a_address, a_size) || InSegment(REL::Segment::data, a_address, a_size);
		}
	}

	bool CodeMatches(std::uintptr_t a_address, const ExpectedCode& a_expected, std::string_view a_name)
	{
		const auto size = a_expected.bytes.size();
		if (!a_address || size == 0 || a_expected.mask.size() != size || !InSegment(REL::Segment::text, a_address, size)) {
			logger::warn("Hook {} skipped on game version {}: target {:X} is not inside .text", a_name, REL::Module::get().version().string(), a_address);
			return false;
		}
		const auto code = reinterpret_cast<const std::uint8_t*>(a_address);
		for (std::size_t i = 0; i < size; ++i) {
			if (((code[i] ^ a_expected.bytes[i]) & a_expected.mask[i]) != 0) {
				logger::warn("Hook {} skipped on game version {}: code at {:X} differs from the expected AE bytes at offset +{:X} (found {:02X}, expected {:02X})",
					a_name, REL::Module::get().version().string(), a_address, i, code[i], a_expected.bytes[i]);
				return false;
			}
		}
		return true;
	}

	bool VtableSlotIsSafe(std::uintptr_t a_vtable, std::size_t a_slot, std::string_view a_rttiName,
		std::uint32_t a_colOffset, const ExpectedCode& a_expected, std::string_view a_name)
	{
		const auto version = REL::Module::get().version().string();
		const auto refuse = [&](std::string_view a_reason) {
			logger::warn("Hook {} skipped on game version {}: {}", a_name, version, a_reason);
			return false;
		};

		// RTTI complete object locator: pointer just before the vtable (signature 1, offset +4, type descriptor RVA +0xC)
		if (!a_vtable || !InImageData(a_vtable - sizeof(std::uintptr_t), sizeof(std::uintptr_t) * (a_slot + 2))) {
			return refuse("vtable is not inside the game image");
		}
		const auto locator = *reinterpret_cast<const std::uintptr_t*>(a_vtable - sizeof(std::uintptr_t));
		if (!InImageData(locator, 0x14)) {
			return refuse("vtable has no RTTI locator");
		}
		const auto fields = reinterpret_cast<const std::uint32_t*>(locator);
		if (fields[0] != 1 || fields[1] != a_colOffset) {
			return refuse(std::format("RTTI locator signature/offset is {}/{:X}, expected 1/{:X}", fields[0], fields[1], a_colOffset));
		}
		const auto descriptor = REL::Module::get().base() + fields[3];
		const auto nameAddress = descriptor + 0x10;  // type_info: vptr, spare, then the decorated name
		if (!InImageData(nameAddress, a_rttiName.size() + 1)) {
			return refuse("RTTI type descriptor is outside the game image");
		}
		const std::string_view found{ reinterpret_cast<const char*>(nameAddress), a_rttiName.size() };
		if (found != a_rttiName || *reinterpret_cast<const char*>(nameAddress + a_rttiName.size()) != '\0') {
			return refuse(std::format("RTTI class name differs from {}", a_rttiName));
		}

		const auto current = reinterpret_cast<const std::uintptr_t*>(a_vtable)[a_slot];
		if (!InSegment(REL::Segment::text, current, 1)) {
			return refuse(std::format("slot {} holds {:X}, which is not inside .text", a_slot, current));
		}
		return CodeMatches(current, a_expected, a_name);
	}

	std::uintptr_t HookPrologue(std::uintptr_t a_target, std::uintptr_t a_detour, std::string_view a_name)
	{
		const auto code = reinterpret_cast<const std::uint8_t*>(a_target);
		const auto length = PatchLength(code);
		if (!length) {
			logger::error("Hook {} not installed: unsupported prologue at {:X}: {}", a_name, a_target, FormatBytes(code, kMaxPatchBytes));
			return 0;
		}

		// Trampoline: original prologue bytes, then an absolute jump back behind them
		auto&      trampoline = F4SE::GetTrampoline();
		const auto original = static_cast<std::uint8_t*>(trampoline.allocate(*length + kAbsoluteJumpSize));
		std::memcpy(original, code, *length);

		const std::uint8_t jumpBack[6]{ 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 };
		const auto         returnAddress = a_target + *length;
		std::memcpy(original + *length, jumpBack, sizeof(jumpBack));
		std::memcpy(original + *length + sizeof(jumpBack), &returnAddress, sizeof(returnAddress));

		trampoline.write_branch<kJumpSize>(a_target, a_detour);
		if (*length > kJumpSize) {
			std::vector<std::uint8_t> nops(*length - kJumpSize, 0x90);
			REL::safe_write(a_target + kJumpSize, nops.data(), nops.size());
		}

		logger::info("Hook {} installed at {:X} (prologue {} bytes: {})", a_name, a_target, *length, FormatBytes(original, *length));
		return reinterpret_cast<std::uintptr_t>(original);
	}
}
