#include "MenuRefCountFix.h"

// Why: a debug build put a hardware write watch on ExamineMenu's refcount (+8) on AE 1.11.240. Upscaling.dll stored
// the count with a plain load / inc / store on the main thread every frame while the game changed it with lock xadd
// on a worker thread. A lost AddRef freed the menu early (CTD after leaving a workbench); a lost Release kept it alive
// (hang on the blurred exit screen). Addictol.dll has the same inlined AddRef.
//
// Patch: the three instructions (8 bytes) become lock inc dword [reg+8] (4 bytes) and a 4-byte NOP. The register they
// loaded is overwritten before any later use, no branch lands inside the 8 bytes, and inc / lock inc set the same flags.
namespace
{
	struct Site
	{
		const wchar_t*                 module;
		const char*                    name;
		std::uint32_t                  rva;  // first byte of the 8 replaced bytes
		std::array<std::uint8_t, 24>   context;  // rva - 8 .. rva + 16, as shipped
		std::array<std::uint8_t, 8>    patch;
	};

	// Upscaling 1.1.1 (Nexus 99130, Upscaling.dll 9,392,128 bytes): inlined RE::UI::GetMenu at 0x7360
	//   740A mov ecx,[rax+8]; 740D inc ecx; 740F mov [rax+8],ecx
	// Addictol (Addictol.dll build checked 2026-11-22): inlined RE::UI::GetMenu at 0x17AB80
	//   17ADDB mov eax,[rcx+8]; 17ADDE inc eax; 17ADE0 mov [rcx+8],eax
	constexpr std::array kSites{
		Site{ L"Upscaling.dll", "Upscaling.dll+740A", 0x740A,
			{ 0x44, 0x24, 0x70, 0x48, 0x85, 0xC0, 0x74, 0x0D, 0x8B, 0x48, 0x08, 0xFF, 0xC1, 0x89, 0x48, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x70, 0x48, 0x8D, 0x4C },
			{ 0xF0, 0xFF, 0x40, 0x08, 0x0F, 0x1F, 0x40, 0x00 } },  // lock inc dword [rax+8]; nop dword [rax+0]
		Site{ L"Addictol.dll", "Addictol.dll+17ADDB", 0x17ADDB,
			{ 0x8B, 0x4B, 0x08, 0x48, 0x85, 0xC9, 0x74, 0x08, 0x8B, 0x41, 0x08, 0xFF, 0xC0, 0x89, 0x41, 0x08, 0x48, 0x8D, 0x44, 0x24, 0x20, 0x49, 0x89, 0x0F },
			{ 0xF0, 0xFF, 0x41, 0x08, 0x0F, 0x1F, 0x40, 0x00 } },  // lock inc dword [rcx+8]; nop dword [rax+0]
	};

	[[nodiscard]] std::string_view Patch(const Site& a_site)
	{
		const auto module = ::GetModuleHandleW(a_site.module);
		if (!module) {
			return "not loaded";
		}
		const auto base = reinterpret_cast<std::uint8_t*>(module);
		const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
		const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
		if (a_site.rva < 8 || a_site.rva + 16 > nt->OptionalHeader.SizeOfImage) {
			return "image too small (other version): not patched";
		}
		const auto target = base + a_site.rva;
		if (std::memcmp(target, a_site.patch.data(), a_site.patch.size()) == 0) {
			return "already patched";
		}
		if (std::memcmp(target - 8, a_site.context.data(), a_site.context.size()) != 0) {
			return "bytes differ (other version): not patched";
		}
		DWORD old = 0;
		if (!::VirtualProtect(target, a_site.patch.size(), PAGE_EXECUTE_READWRITE, &old)) {
			return "VirtualProtect failed: not patched";
		}
		std::memcpy(target, a_site.patch.data(), a_site.patch.size());
		::VirtualProtect(target, a_site.patch.size(), old, &old);
		::FlushInstructionCache(::GetCurrentProcess(), target, a_site.patch.size());
		return "patched to lock inc";
	}
}

namespace MenuRefCountFix
{
	void Apply()
	{
		for (const auto& site : kSites) {
			logger::info("{} (non-atomic menu AddRef): {}", site.name, Patch(site));
		}
	}
}
