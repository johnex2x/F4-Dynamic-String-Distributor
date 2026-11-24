#include "ConstTranslation.h"

#include "AeHookBytes.h"
#include "HookUtil.h"
#include "Manager.h"
#include "PortedConstFields.h"
#include "Runtime.h"
#include "Settings.h"
#include "TemplateNames.h"
#include "TextWrite.h"

namespace
{
	struct ApplyStats
	{
		std::size_t applied{ 0 };    // includes the unchanged ones
		std::size_t unchanged{ 0 };  // part of applied: the field already held the text
		std::size_t formMissing{ 0 };
		std::size_t wrongType{ 0 };  // form has no field for this type
		std::size_t deferred{ 0 };   // entry belongs to a later ticket (e.g. template names)
		std::size_t skipped{ 0 };    // player, missing editor ID, ...
		std::size_t templateNames{ 0 };  // part of applied: object template names (FULL index > 1)
		std::size_t byChoice{ 0 };       // part of applied: PERK EPF2 entries with "choice"
		std::size_t choiceMissing{ 0 };  // part of skipped: PERK EPF2 entries with "choice" whose choice has another index
	};

	// Indexed text fields. Index meaning checked against Fallout4.esm and the xTranslator export:
	// MESG ITXT is 0-based, TERM lists are 1-based, single fields use index 1 (or 0 in upstream files).
	enum class Slot
	{
		kApplied,
		kWrongType,  // the form has no such field
		kNoSlot,     // the index points past the form's list
		kUnchanged   // the field already held the text (counts as applied)
	};

	[[nodiscard]] Slot Written(bool a_changed) noexcept { return a_changed ? Slot::kApplied : Slot::kUnchanged; }

	[[nodiscard]] bool IsPlayerForm(const RE::TESForm* a_form)
	{
		return (a_form->IsPlayer() || a_form->IsPlayerRef()) && !Settings::GetSingleton().enablePlayerRef;
	}

	using SparseNameMap = RE::BSTHashMap<const RE::TESForm*, RE::BGSLocalizedString>;

	// AE: the sparse FULL name map (docs/phaseD-sparse-fullname.md). The engine reads and writes it with its own code
	// (lookup ID 2193216, erase/insert ID 2193219); that code proves the layout, which equals the fork's BSTHashMap:
	// object at (ID 2661402 - 8), capacity +0xC, free +0x10, good +0x14, sentinel +0x18, entries +0x28, 0x18-byte
	// entries {key, BGSLocalizedString, next}, slot = CRC32 of the key's 8 bytes & (capacity - 1) (ID 2267895).
	// This runs once, at the first apply; any failed check keeps the map off and logs why.
	[[nodiscard]] SparseNameMap* FindSparseNameMapAE()
	{
		constexpr std::uint64_t kMapID = 2661402;      // data: the map object + 8
		constexpr std::uint64_t kLookupID = 2193216;   // code that reads the map; its bytes pin the layout
		constexpr std::uint64_t kHashID = 2267895;     // CRC helper the lookup calls
		constexpr std::size_t   kMapSize = 0x30;       // up to and including the entries pointer at +0x28
		constexpr std::size_t   kEntrySize = 0x18;
		// Offsets inside the lookup of the three RIP-relative loads: instruction start, instruction length,
		// map member it must read (equal in .191, .221 and .240); the disp32 is the last 4 bytes of the instruction
		struct Load { std::size_t at; std::size_t length; std::ptrdiff_t member; };
		constexpr Load kLoads[]{ { 0x5B, 7, 0x28 }, { 0x67, 6, 0xC }, { 0x89, 7, 0x18 } };

		const REL::Relocation<std::uintptr_t> mapId{ REL::ID(kMapID) };
		const REL::Relocation<std::uintptr_t> lookup{ REL::ID(kLookupID) };
		const REL::Relocation<std::uintptr_t> hash{ REL::ID(kHashID) };
		const auto                            address = mapId.address() - 0x8;

		if (!HookUtil::CodeMatches(lookup.address(), AeHookBytes::kSparseNameLookup, "TESFullName sparse map lookup") ||
			!HookUtil::CodeMatches(hash.address(), AeHookBytes::kSparseNameHash, "TESFullName sparse map hash")) {
			return nullptr;
		}

		// The lookup must read exactly the members this layout names
		const auto code = reinterpret_cast<const std::uint8_t*>(lookup.address());
		for (const auto& load : kLoads) {
			std::int32_t displacement;
			std::memcpy(&displacement, code + load.at + load.length - sizeof(displacement), sizeof(displacement));
			const auto target = lookup.address() + load.at + load.length + static_cast<std::ptrdiff_t>(displacement);
			if (target != address + static_cast<std::uintptr_t>(load.member)) {
				logger::error("Sparse FULL map: the engine lookup reads {:X} at +{:X}, expected map {:X} + {:X}; not used",
					target, load.at, address, load.member);
				return nullptr;
			}
		}

		const auto data = REL::Module::get().segment(REL::Segment::data);
		if (address < data.address() || address > data.address() + data.size() || data.size() - (address - data.address()) < kMapSize) {
			logger::error("Sparse FULL map: object {:X} is not inside .data; not used", address);
			return nullptr;
		}

		const auto base = reinterpret_cast<const std::byte*>(address);
		const auto u32 = [&](std::ptrdiff_t a_offset) { std::uint32_t v; std::memcpy(&v, base + a_offset, sizeof(v)); return v; };
		const auto u64 = [&](std::ptrdiff_t a_offset) { std::uint64_t v; std::memcpy(&v, base + a_offset, sizeof(v)); return v; };

		const auto capacity = u32(0xC);
		const auto free = u32(0x10);
		const auto good = u32(0x14);
		const auto sentinel = u64(0x18);
		const auto entries = u64(0x28);
		const bool sentinelOk = sentinel >= data.address() && sentinel < data.address() + data.size();
		const bool shapeOk = (capacity == 0 || std::has_single_bit(capacity)) && free <= capacity && (capacity == 0 || good < capacity) && sentinelOk;
		const bool entriesOk = capacity == 0 ? entries == 0 : (entries != 0 && entries % alignof(void*) == 0);
		if (!shapeOk || !entriesOk) {
			logger::error("Sparse FULL map {:X} has an unexpected layout (capacity {}, free {}, good {}, sentinel {:X}, entries {:X}); not used",
				address, capacity, free, good, sentinel, entries);
			return nullptr;
		}

		// Live check of the hash and the entry layout against the filled map: the used slots must equal capacity - free,
		// and an entry either sits in the slot its key hashes to or in a chain that starts in that slot
		std::size_t used = 0;
		std::size_t placed = 0;
		const auto  slots = reinterpret_cast<const std::byte*>(entries);
		const auto  slotAt = [&](std::size_t a_index) { return slots + a_index * kEntrySize; };
		const auto  nextOf = [&](const std::byte* a_slot) { std::uint64_t v; std::memcpy(&v, a_slot + 0x10, sizeof(v)); return v; };
		const auto  keyOf = [&](const std::byte* a_slot) { std::uint64_t v; std::memcpy(&v, a_slot, sizeof(v)); return v; };
		const auto  homeOf = [&](std::uint64_t a_key) {
			return static_cast<std::size_t>(RE::BSCRC32<const RE::TESForm*>()(reinterpret_cast<const RE::TESForm*>(a_key)) & (capacity - 1));
		};
		for (std::size_t i = 0; i < capacity; ++i) {
			const auto slot = slotAt(i);
			if (nextOf(slot) == 0) {
				continue;
			}
			++used;
			const auto home = homeOf(keyOf(slot));
			if (home == i || (nextOf(slotAt(home)) != 0 && homeOf(keyOf(slotAt(home))) == home)) {
				++placed;
			}
		}
		if (used + free != capacity || placed * 10 < used * 9) {
			logger::error("Sparse FULL map {:X}: {} used slots, {} free, capacity {}, {} entries hash to a chain start; not used",
				address, used, free, capacity, placed);
			return nullptr;
		}

		logger::info("Sparse FULL map {:X}: capacity {}, {} entries", address, capacity, used);
		return reinterpret_cast<SparseNameMap*>(address);
	}

	// The map of names for forms without a TESFullName component. OG: the fork's map. AE: the checked map or nullptr.
	// Decided once, so an apply pass pays nothing beyond what OG pays.
	[[nodiscard]] SparseNameMap* GetSparseNameMap()
	{
		if (DSD::IsAE()) {
			static SparseNameMap* const map = FindSparseNameMapAE();
			return map;
		}
		return std::addressof(RE::TESFullName::GetSparseFullNameMap());
	}

	// Writes a name without AddChange, so translations never end up in save games (same as upstream)
	[[nodiscard]] Slot SetName(RE::TESForm* a_form, const std::string& a_text)
	{
		if (const auto fullName = a_form->As<RE::TESFullName>()) {
			return Written(TextWrite::Assign(fullName->fullName, a_text));
		}

		// Forms without a TESFullName component (STAT, MSTT, SCOL, KYWD, FLST, LIGH, ...) keep names in a sparse map
		if (const auto sparse = GetSparseNameMap()) {
			const auto it = sparse->find(a_form);
			if (it != sparse->end()) {
				return Written(TextWrite::Assign(it->second, a_text));
			}
		}

		return Slot::kWrongType;
	}

	void ApplyNames(const DSD::TranslationStore& a_store, DSD::TypeCode a_type, ApplyStats& a_stats)
	{
		a_store.ForEach(a_type, [&](std::uint32_t a_formID, std::uint32_t a_index, const DSD::Entry& a_entry) {
			// Index > 1 is an object template name; only ARMO, WEAP and NPC_ have them (TemplateNames.cpp)
			if (!ConstTranslation::IsBaseNameIndex(a_index) && !TemplateNames::IsTemplateAwareType(a_type)) {
				++a_stats.deferred;
				return;
			}

			const auto form = DSD::LookupFormByID(a_formID);
			if (!form) {
				++a_stats.formMissing;
				logger::debug("{} {:08X}: form not found", DSD::TypeCodeToString(a_type), a_formID);
				return;
			}
			if (IsPlayerForm(form)) {
				++a_stats.skipped;
				return;
			}

			if (TemplateNames::IsTemplateAwareType(a_type)) {
				bool unchanged = false;
				switch (TemplateNames::WriteName(form, a_index, a_entry.text, &unchanged)) {
				case TemplateNames::Result::kTemplateName:
					++a_stats.templateNames;
					[[fallthrough]];
				case TemplateNames::Result::kOwnName:
					++a_stats.applied;
					a_stats.unchanged += unchanged;
					return;
				default:
					++a_stats.skipped;
					logger::debug("{} {:08X}: index {} not found on the form", DSD::TypeCodeToString(a_type), a_formID, a_index);
					return;
				}
			}

			switch (SetName(form, a_entry.text)) {
			case Slot::kApplied:
				++a_stats.applied;
				break;
			case Slot::kUnchanged:
				++a_stats.applied;
				++a_stats.unchanged;
				break;
			default:
				++a_stats.wrongType;
				logger::debug("{} {:08X}: form type {} has no name field", DSD::TypeCodeToString(a_type), a_formID, form->GetFormTypeString());
				break;
			}
		});
	}

	// Index 1 is the xTranslator default (for an NPC with template names, the own name comes after them);
	// upstream DSD files use no index
	[[nodiscard]] const DSD::Entry* FindName(const DSD::TranslationStore& a_store, DSD::TypeCode a_type, const RE::TESForm* a_form)
	{
		const auto a_formID = a_form->GetFormID();
		const auto entry = a_store.Find(a_type, a_formID, TemplateNames::OwnNameIndex(a_form));
		return entry ? entry : a_store.Find(a_type, a_formID, 0);
	}

	// AE: the TESNPC TESFullName vtable and the TESFullName offset inside TESNPC (docs/phaseC-c1-rtti.md section 3).
	// Set by InstallHooks before the AE hook is installed.
	std::uintptr_t           g_npcFullNameVtable{ 0 };
	constexpr std::ptrdiff_t kNpcFullNameOffset = 288;

	// TESFullName::CopyComponent(BaseFormComponent*), vtable slot 6, OG Address Library ID 1510422, AE ID 2193213.
	// NPCs that use a template (or leveled NPCs) copy the name from another NPC. Upstream DSD
	// (NPCFullNameCopyComponent) writes the source NPC's translation first, so the copy is translated.
	// Two instantiations, chosen at install time, so the hook itself carries no runtime check:
	// OG casts with fallout_cast; AE (the fork's RTTI IDs are absent there) compares the vtable pointer, which
	// identifies a TESNPC exactly (no class derives from TESNPC).
	template <bool AE>
	struct FullNameCopyComponentT
	{
		static void thunk(RE::TESFullName* a_this, RE::BaseFormComponent* a_from)
		{
			if (a_from && Manager::GetSingleton().EnsureLoaded()) {
				RE::TESForm* form = nullptr;
				if constexpr (AE) {
					if (*reinterpret_cast<const std::uintptr_t*>(a_from) == g_npcFullNameVtable) {
						form = reinterpret_cast<RE::TESForm*>(reinterpret_cast<std::byte*>(a_from) - kNpcFullNameOffset);
					}
				} else {
					// TESNPC's own RTTI entry is unreliable in CommonLibF4, so cast to TESForm and check the type
					form = RE::fallout_cast<RE::TESForm*>(a_from);
				}
				if (form && form->GetFormType() == RE::ENUM_FORM_ID::kNPC_ && !IsPlayerForm(form)) {
					if (const auto entry = FindName(Manager::GetSingleton().Store(), DSD::TypeOf("NPC_ FULL"), form)) {
						static_cast<void>(SetName(form, entry->text));  // before the copy, so the result does not matter
					}
				}
			}
			func(a_this, a_from);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	using FieldWriter = Slot (*)(RE::TESForm*, std::uint32_t a_index, const std::string& a_text);

	// Layout from libxse/commonlibf4 RE/B/BGSQuestObjective.h (only forward-declared in the alandtse fork)
	struct QuestObjective
	{
		RE::BGSLocalizedString displayText;  // 00
		RE::TESQuest*          ownerQuest;   // 08
		void*                  targets;      // 10
		std::uint32_t          numTargets;   // 18
		std::uint16_t          index;        // 1C
		bool                   initialized;  // 1E
		char                   state;        // 1F
		std::uint32_t          flags;        // 20
	};
	static_assert(sizeof(QuestObjective) == 0x28);

	// QUST NNAM: index is the objective index (QOBJ)
	Slot WriteQuestObjective(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text)
	{
		const auto quest = a_form->As<RE::TESQuest>();
		if (!quest) {
			return Slot::kWrongType;
		}
		for (const auto objective : quest->objectives) {
			const auto data = reinterpret_cast<QuestObjective*>(objective);
			if (data && data->index == a_index) {
				return Written(TextWrite::Assign(data->displayText, a_text));
			}
		}
		return Slot::kNoSlot;
	}

	// MESG ITXT: index is the 0-based button position
	Slot WriteMessageButton(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text)
	{
		const auto message = a_form->As<RE::BGSMessage>();
		if (!message) {
			return Slot::kWrongType;
		}
		std::uint32_t position = 0;
		for (const auto button : message->buttonList) {
			if (!button) {
				continue;
			}
			if (position++ == a_index) {
				return Written(TextWrite::Assign(button->text, a_text));
			}
		}
		return Slot::kNoSlot;
	}

	Slot WriteMessageShortName(RE::TESForm* a_form, std::uint32_t, const std::string& a_text)
	{
		const auto message = a_form->As<RE::BGSMessage>();
		if (!message) {
			return Slot::kWrongType;
		}
		return Written(TextWrite::Assign(message->shortName, a_text));
	}

	// 1-based list position; 0 (no index in the JSON) means the first one
	[[nodiscard]] constexpr std::uint32_t Position(std::uint32_t a_index) noexcept { return a_index ? a_index - 1 : 0; }

	Slot WriteTerminalBody(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text)
	{
		const auto terminal = a_form->As<RE::BGSTerminal>();
		if (!terminal) {
			return Slot::kWrongType;
		}
		const auto position = Position(a_index);
		if (position >= terminal->bodyTextItems.size()) {
			return Slot::kNoSlot;
		}
		return Written(TextWrite::Assign(terminal->bodyTextItems[position].itemText, a_text));
	}

	Slot WriteTerminalItem(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text)
	{
		const auto terminal = a_form->As<RE::BGSTerminal>();
		if (!terminal) {
			return Slot::kWrongType;
		}
		const auto position = Position(a_index);
		if (position >= terminal->menuItems.size()) {
			return Slot::kNoSlot;
		}
		return Written(TextWrite::Assign(terminal->menuItems[position].itemText, a_text));
	}

	// TERM RNAM / UNAM count only the menu items that have that subrecord
	template <class Field>
	Slot WriteTerminalNthField(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text, Field a_field)
	{
		const auto terminal = a_form->As<RE::BGSTerminal>();
		if (!terminal) {
			return Slot::kWrongType;
		}
		auto remaining = Position(a_index);
		for (auto& item : terminal->menuItems) {
			if (const auto field = a_field(item)) {
				if (remaining-- == 0) {
					return Written(TextWrite::Assign(*field, a_text));
				}
			}
		}
		return Slot::kNoSlot;
	}

	Slot WriteTerminalResponse(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text)
	{
		return WriteTerminalNthField(a_form, a_index, a_text, [](RE::BGSTerminal::MenuItem& a_item) -> RE::BGSLocalizedString* {
			return static_cast<std::string_view>(a_item.responseText).empty() ? nullptr : &a_item.responseText;
		});
	}

	// The selection result is a union. Fallout4.esm sets ANAM 8 exactly on the items with display text (UNAM).
	// A form pointer in the union would start with a vtable in the exe's .rdata; a string never does.
	[[nodiscard]] bool PointsToForm(const void* a_pointer)
	{
		static const auto rdata = REL::Module::get().segment(REL::Segment::rdata);
		const auto        first = *static_cast<const std::uintptr_t*>(a_pointer);
		return first >= rdata.address() && first < rdata.address() + rdata.size();
	}

	Slot WriteTerminalResult(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text)
	{
		constexpr std::int8_t kDisplayText = 8;
		return WriteTerminalNthField(a_form, a_index, a_text, [](RE::BGSTerminal::MenuItem& a_item) -> RE::BGSLocalizedString* {
			const auto text = a_item.selectionResult.displayText;
			return a_item.flags == kDisplayText && text && !PointsToForm(text) ? text : nullptr;
		});
	}

	Slot WriteTerminalWelcome(RE::TESForm* a_form, std::uint32_t, const std::string& a_text)
	{
		const auto terminal = a_form->As<RE::BGSTerminal>();
		if (!terminal) {
			return Slot::kWrongType;
		}
		return Written(TextWrite::Assign(terminal->welcomeText, a_text));
	}

	Slot WriteTerminalHeader(RE::TESForm* a_form, std::uint32_t, const std::string& a_text)
	{
		const auto terminal = a_form->As<RE::BGSTerminal>();
		if (!terminal) {
			return Slot::kWrongType;
		}
		return Written(TextWrite::Assign(terminal->headerTextOverride, a_text));
	}

	struct IndexedField
	{
		DSD::TypeCode type;
		FieldWriter   write;
	};

	// Ticket 13 writers live in PortedConstFields.cpp; their Slot has the same enumerators
	template <PortedConstFields::Slot (*Write)(RE::TESForm*, std::uint32_t, const std::string&)>
	Slot Ported(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text)
	{
		return static_cast<Slot>(Write(a_form, a_index, a_text));
	}

	constexpr std::array kIndexedFields{
		IndexedField{ DSD::TypeOf("QUST NNAM"), WriteQuestObjective },
		IndexedField{ DSD::TypeOf("MESG ITXT"), WriteMessageButton },
		IndexedField{ DSD::TypeOf("MESG NNAM"), WriteMessageShortName },
		IndexedField{ DSD::TypeOf("TERM BTXT"), WriteTerminalBody },
		IndexedField{ DSD::TypeOf("TERM ITXT"), WriteTerminalItem },
		IndexedField{ DSD::TypeOf("TERM RNAM"), WriteTerminalResponse },
		IndexedField{ DSD::TypeOf("TERM UNAM"), WriteTerminalResult },
		IndexedField{ DSD::TypeOf("TERM WNAM"), WriteTerminalWelcome },
		IndexedField{ DSD::TypeOf("TERM NAM0"), WriteTerminalHeader },
		IndexedField{ DSD::TypeOf("NPC_ SHRT"), Ported<PortedConstFields::WriteNpcShortName> },
		IndexedField{ DSD::TypeOf("LSCR DESC"), Ported<PortedConstFields::WriteLoadScreenText> },
		IndexedField{ DSD::TypeOf("MGEF DNAM"), Ported<PortedConstFields::WriteMagicDescription> },
		IndexedField{ DSD::TypeOf("REFR FULL"), Ported<PortedConstFields::WriteMapMarkerName> },
		IndexedField{ DSD::TypeOf("REGN RDMP"), Ported<PortedConstFields::WriteRegionMapName> },
	};

	constexpr DSD::TypeCode kPerkChoiceType = DSD::TypeOf("PERK EPF2");

	void Count(Slot a_slot, ApplyStats& a_stats, DSD::TypeCode a_type, const RE::TESForm* a_form, std::uint32_t a_index)
	{
		switch (a_slot) {
		case Slot::kUnchanged:
			++a_stats.unchanged;
			[[fallthrough]];
		case Slot::kApplied:
			++a_stats.applied;
			break;
		case Slot::kWrongType:
			++a_stats.wrongType;
			logger::debug("{} {:08X}: form type {} has no such field", DSD::TypeCodeToString(a_type), a_form->GetFormID(), a_form->GetFormTypeString());
			break;
		case Slot::kNoSlot:
			++a_stats.skipped;
			logger::debug("{} {:08X}: index {} not found on the form", DSD::TypeCodeToString(a_type), a_form->GetFormID(), a_index);
			break;
		}
	}

	void ApplyIndexedField(const DSD::TranslationStore& a_store, const IndexedField& a_field, ApplyStats& a_stats)
	{
		a_store.ForEach(a_field.type, [&](std::uint32_t a_formID, std::uint32_t a_index, const DSD::Entry& a_entry) {
			const auto form = DSD::LookupFormByID(a_formID);
			if (!form) {
				++a_stats.formMissing;
				return;
			}
			Count(a_field.write(form, a_index, a_entry.text), a_stats, a_field.type, form, a_index);
		});
	}

	void ApplyPerkChoices(const DSD::TranslationStore& a_store, ApplyStats& a_stats)
	{
		a_store.ForEach(kPerkChoiceType, [&](std::uint32_t a_formID, std::uint32_t a_index, const DSD::Entry& a_entry) {
			const auto form = DSD::LookupFormByID(a_formID);
			if (!form) {
				++a_stats.formMissing;
				return;
			}
			const auto choice = a_entry.choice.value_or(0);
			const auto slot = static_cast<Slot>(PortedConstFields::WritePerkActivateChoice(form, a_index, choice, a_entry.text));
			Count(slot, a_stats, kPerkChoiceType, form, a_index);
			if (choice) {
				if (slot == Slot::kNoSlot) {
					++a_stats.choiceMissing;
					logger::debug("PERK EPF2 {:08X}: choice {} does not have index {}", a_formID, choice, a_index);
				} else if (slot != Slot::kWrongType) {
					++a_stats.byChoice;
				}
			}
		});
	}

	// Copies the text into the game heap, so the game may free or replace it like its own strings
	[[nodiscard]] char* GameHeapCopy(const std::string& a_text)
	{
		const auto buffer = static_cast<char*>(DSD::GameMalloc(a_text.size() + 1));
		std::memcpy(buffer, a_text.c_str(), a_text.size() + 1);
		return buffer;
	}

	void ApplyGameSettings(const DSD::TranslationStore& a_store, ApplyStats& a_stats)
	{
		const auto collection = DSD::GetGameSettingCollection();
		if (!collection) {
			logger::error("GameSettingCollection not available");
			return;
		}

		a_store.ForEach(DSD::TypeOf("GMST DATA"), [&](std::uint32_t a_formID, std::uint32_t, const DSD::Entry& a_entry) {
			if (!a_entry.editorID || a_entry.editorID->empty()) {
				++a_stats.skipped;
				logger::error("GMST DATA {:08X}: editor_id is missing", a_formID);
				return;
			}

			const auto setting = collection->GetSetting(*a_entry.editorID);
			if (!setting) {
				++a_stats.formMissing;
				logger::debug("GMST {} not found", *a_entry.editorID);
				return;
			}
			if (setting->GetType() != RE::Setting::SETTING_TYPE::kString) {
				++a_stats.wrongType;
				return;
			}

			++a_stats.applied;
			if (setting->GetString() == a_entry.text) {  // re-apply after a save load: no new game-heap copy
				++a_stats.unchanged;
				return;
			}
			setting->SetString(GameHeapCopy(a_entry.text));
		});
	}
}

namespace ConstTranslation
{
	namespace
	{
		// AE: ID 2193213 with its code bytes, the TESFullName vtable (class name, slot 6 = the target), and the TESNPC
		// vtable that the AE thunk compares against (class name, complete-object offset 288, slot 6 is the same function).
		// Any mismatch skips the hook with one warning; the other features keep working.
		void InstallHooksAE()
		{
			constexpr std::uint64_t         copyComponentID = 2193213;
			REL::Relocation<std::uintptr_t> vtable{ RE::VTABLE::TESFullName[0] };
			REL::Relocation<std::uintptr_t> target{ REL::ID(copyComponentID) };
			REL::Relocation<std::uintptr_t> npcVtable{ RE::VTABLE::TESNPC[5] };
			if (!HookUtil::VtableSlotIsSafe(vtable.address(), 6, ".?AVTESFullName@@", 0, AeHookBytes::kFullNameCopyComponent, "TESFullName::CopyComponent") ||
				!HookUtil::VtableSlotIsSafe(npcVtable.address(), 6, ".?AVTESNPC@@", static_cast<std::uint32_t>(kNpcFullNameOffset), AeHookBytes::kFullNameCopyComponent, "TESFullName::CopyComponent (TESNPC vtable)")) {
				return;
			}
			const auto slot = reinterpret_cast<const std::uintptr_t*>(vtable.address())[6];
			if (slot != target.address()) {
				logger::error("Hook TESFullName::CopyComponent not installed: ID {} is {:X}, vtable slot 6 is {:X}", copyComponentID, target.address(), slot);
				return;
			}
			g_npcFullNameVtable = npcVtable.address();
			HookUtil::InstallPrologueHookChecked<FullNameCopyComponentT<true>>(copyComponentID, "TESFullName::CopyComponent", AeHookBytes::kFullNameCopyComponent);
		}
	}

	void InstallHooks()
	{
		if (DSD::IsAE()) {
			InstallHooksAE();
			return;
		}

		// The ID is not byte-checked on OG (exe is encrypted on disk), so compare it with the vtable first
		constexpr std::uint64_t         copyComponentID = 1510422;
		REL::Relocation<std::uintptr_t> vtable{ RE::VTABLE::TESFullName[0] };
		REL::Relocation<std::uintptr_t> target{ REL::ID(copyComponentID) };
		const auto                      slot = reinterpret_cast<const std::uintptr_t*>(vtable.address())[6];
		if (slot != target.address()) {
			logger::error("Hook TESFullName::CopyComponent not installed: ID {} is {:X}, vtable slot 6 is {:X}", copyComponentID, target.address(), slot);
			return;
		}
		HookUtil::InstallPrologueHook<FullNameCopyComponentT<false>>(copyComponentID, "TESFullName::CopyComponent");
	}

	void ApplyAll()
	{
		const auto  start = std::chrono::steady_clock::now();
		const auto& store = Manager::GetSingleton().Store();

		if (DSD::IsAE() && !GetSparseNameMap()) {
			logger::info("  FULL of forms without a TESFullName component (STAT, MSTT, SCOL, KYWD, FLST, LIGH) is not translated on this runtime");
		}

		std::map<std::string, ApplyStats> stats;
		for (const auto type : store.Types()) {
			if (IsBaseNameType(type)) {
				ApplyNames(store, type, stats[DSD::TypeCodeToString(type)]);
			}
		}
		ApplyGameSettings(store, stats["GMST DATA"]);
		for (const auto& field : kIndexedFields) {
			ApplyIndexedField(store, field, stats[DSD::TypeCodeToString(field.type)]);
		}
		ApplyPerkChoices(store, stats["PERK EPF2"]);

		const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
		logger::info("Const translations applied in {} ms:", elapsed.count());
		for (const auto& [name, s] : stats) {
			logger::info("  {} : {:>6} applied ({} unchanged), {:>4} form missing, {:>4} wrong type, {:>4} deferred, {:>4} skipped",
				name, s.applied, s.unchanged, s.formMissing, s.wrongType, s.deferred, s.skipped);
			if (s.templateNames) {
				logger::info("    of which {} are object template names", s.templateNames);
			}
			if (s.byChoice || s.choiceMissing) {
				logger::info("    by choice position {} applied, {} with another index there", s.byChoice, s.choiceMissing);
			}
		}
		TemplateNames::ApplyInnr();
	}
}
