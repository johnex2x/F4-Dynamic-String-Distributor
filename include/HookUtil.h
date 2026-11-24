#pragma once

// Hook helpers ported from upstream DSD (PCH.h, namespace stl)
namespace HookUtil
{
	// Replaces a 5-byte call at a_src with a call to T::thunk; T::func keeps the original target
	template <class T, std::size_t SIZE = 5>
	void WriteThunkCall(std::uintptr_t a_src)
	{
		auto& trampoline = F4SE::GetTrampoline();
		T::func = trampoline.write_call<SIZE>(a_src, T::thunk);
	}

	// Jumps from the start of the function at a_target to a_detour.
	// The OG exe on disk is encrypted (SteamStub), so prologue lengths are decoded at runtime.
	// Returns the address that runs the original function, or 0 if the prologue is unsupported
	// (the bytes are logged then). Decoder adapted from FallHook (GPL-3.0).
	[[nodiscard]] std::uintptr_t HookPrologue(std::uintptr_t a_target, std::uintptr_t a_detour, std::string_view a_name);

	// Expected code bytes of a hook target on AE (1.11.x). A mask byte of 0x00 ignores that byte
	// (RIP-relative displacements and rel32 call targets differ between builds). Generated into AeHookBytes.h.
	struct ExpectedCode
	{
		std::span<const std::uint8_t> bytes;
		std::span<const std::uint8_t> mask;
	};

	// AE only. True when the code at a_address matches a_expected. Otherwise logs one warning that names the hook
	// and the game version, and returns false so the caller skips just that hook.
	[[nodiscard]] bool CodeMatches(std::uintptr_t a_address, const ExpectedCode& a_expected, std::string_view a_name);

	// AE only. True when writing a_slot of the vtable at a_vtable is safe: the vtable's RTTI class name is a_rttiName,
	// its complete-object offset is a_colOffset, the current slot value lies in .text and its code matches a_expected.
	// Otherwise logs one warning naming the hook and returns false.
	[[nodiscard]] bool VtableSlotIsSafe(std::uintptr_t a_vtable, std::size_t a_slot, std::string_view a_rttiName,
		std::uint32_t a_colOffset, const ExpectedCode& a_expected, std::string_view a_name);

	// Hooks the function behind an Address Library ID. T needs `static thunk(...)` and `static inline REL::Relocation<decltype(thunk)> func`.
	template <class T>
	bool InstallPrologueHook(std::uint64_t a_id, std::string_view a_name)
	{
		REL::Relocation<std::uintptr_t> target{ REL::ID(a_id) };
		const auto original = HookPrologue(target.address(), reinterpret_cast<std::uintptr_t>(&T::thunk), a_name);
		if (!original) {
			return false;
		}
		T::func = original;
		return true;
	}

	// AE only: like InstallPrologueHook, but the target bytes are compared with a_expected first
	// (a mismatch skips this hook only, with one warning).
	template <class T>
	bool InstallPrologueHookChecked(std::uint64_t a_id, std::string_view a_name, const ExpectedCode& a_expected)
	{
		REL::Relocation<std::uintptr_t> target{ REL::ID(a_id) };
		if (!CodeMatches(target.address(), a_expected, a_name)) {
			return false;
		}
		const auto original = HookPrologue(target.address(), reinterpret_cast<std::uintptr_t>(&T::thunk), a_name);
		if (!original) {
			return false;
		}
		T::func = original;
		return true;
	}
}
