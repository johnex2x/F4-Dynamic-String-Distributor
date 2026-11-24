#include "NameRefresh.h"

#include "AeHookBytes.h"
#include "Hangul.h"
#include "HookUtil.h"
#include "Runtime.h"
#include "Settings.h"

namespace
{
	constexpr std::size_t kDetailCap = 300;      // detail lines per Apply
	constexpr std::size_t kBucketReserve = 50;   // part of the cap that only bucket lines may use

	using Clock = std::chrono::steady_clock;

	// [Debug] bNameRefreshDryRun: walk and count, but never call SetDisplayNameFromInstanceData (first AE run)
	bool g_dryRun = false;

	[[nodiscard]] std::int64_t MicrosSince(Clock::time_point a_start)
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - a_start).count();
	}

	struct Counts
	{
		std::size_t seen{ 0 };  // extra lists with a text extra
		std::size_t otherType{ 0 };
		std::size_t otherTypeHangul{ 0 };
		std::size_t customName{ 0 };
		std::size_t displayText{ 0 };
		std::size_t ownerQuest{ 0 };
		std::size_t guardedHangul{ 0 };  // custom, displayText or ownerQuest names with Hangul
		std::size_t textOnly{ 0 };
		std::size_t textOnlyHangul{ 0 };
		std::size_t noInstanceData{ 0 };
		std::size_t noInstanceDataHangul{ 0 };
		std::size_t rebuilt{ 0 };
		std::size_t changed{ 0 };
		std::size_t stillHangul{ 0 };
		std::size_t removed{ 0 };
		std::size_t instanceChanged{ 0 };
		std::size_t modsChanged{ 0 };
	};

	struct Detail
	{
		bool        enabled{ false };
		std::size_t used{ 0 };
		std::size_t cap{ kDetailCap };  // raised in a dry run, so a test item's line is never dropped

		// Rebuild results may not eat the lines reserved for the buckets
		[[nodiscard]] bool TakeChange() { return enabled && used + kBucketReserve < cap && ++used; }
		[[nodiscard]] bool TakeBucket() { return enabled && used < cap && ++used; }
	};

	// Other inventories (containers, actors, vendors): every stack is counted, only a stack whose name has Hangul is rebuilt.
	// Keep this gate. A stored inventory name can hold text other mods wrote on purpose, and the rebuild discards it.
	// Without the gate, the rebuild removed suffixes that other mods append to item names at run time, doubled power armor
	// paint suffixes, and turned some translated names back into the source language.
	struct ContainerCounts
	{
		std::size_t lists{ 0 };
		std::size_t stacks{ 0 };
		std::size_t withText{ 0 };
		std::size_t hangul{ 0 };
		long long   us{ 0 };  // time spent in lists that exist (the walk of cells is timed apart)
	};

	[[nodiscard]] std::string_view TextOf(const RE::ExtraTextDisplayData* a_text)
	{
		return a_text ? static_cast<std::string_view>(a_text->displayName) : std::string_view{};
	}

	// objectID, index, rank and disabled of every mod on the instance, packed one value each
	[[nodiscard]] std::vector<std::uint64_t> ModList(const RE::BGSObjectInstanceExtra* a_inst)
	{
		std::vector<std::uint64_t> out;
		if (!a_inst || !a_inst->values) {
			return out;
		}
		for (const auto& mod : a_inst->GetIndexData()) {
			out.push_back(static_cast<std::uint64_t>(mod.objectID) | (static_cast<std::uint64_t>(mod.index) << 32) |
						  (static_cast<std::uint64_t>(mod.rank) << 40) | (static_cast<std::uint64_t>(mod.disabled) << 48));
		}
		return out;
	}

	[[nodiscard]] const char* TypeName(const RE::TESBoundObject* a_base)
	{
		return a_base ? a_base->GetFormTypeString() : "none";
	}

	[[nodiscard]] std::string_view FullNameOf(RE::TESBoundObject* a_base)
	{
		const auto full = a_base ? a_base->As<RE::TESFullName>() : nullptr;
		return full ? static_cast<std::string_view>(full->fullName) : std::string_view{};
	}

	void LogBucket(Detail& a_detail, std::string_view a_where, std::string_view a_bucket, std::uint32_t a_ref, RE::TESBoundObject* a_base,
		std::string_view a_text)
	{
		if (a_detail.TakeBucket()) {
			logger::info("[NameRefresh] {} {} ref {:08X} base {:08X} type {} text \"{}\" base FULL \"{}\"", a_where, a_bucket, a_ref,
				a_base ? a_base->GetFormID() : 0u, TypeName(a_base), a_text, FullNameOf(a_base));
		}
	}

	// Applies the buckets to one extra list. a_ref is 0 for a player inventory stack.
	void Process(RE::ExtraDataList* a_list, RE::TESBoundObject* a_base, std::uint32_t a_ref, std::string_view a_where, Counts& a_counts, Detail& a_detail)
	{
		if (!a_list || !a_base) {
			return;
		}
		const auto text = a_list->GetByType<RE::ExtraTextDisplayData>();
		if (!text) {
			return;
		}
		++a_counts.seen;
		const auto name = TextOf(text);

		const auto type = a_base->GetFormType();
		if (type != RE::ENUM_FORM_ID::kARMO && type != RE::ENUM_FORM_ID::kWEAP) {
			++a_counts.otherType;
			if (HasHangul(name)) {
				++a_counts.otherTypeHangul;
				LogBucket(a_detail, a_where, "otherType", a_ref, a_base, name);
			}
			return;
		}

		// flags bit 0 (BSExtraData +0x10) marks a custom name
		const bool custom = (text->flags & 1) != 0;
		const bool displayText = text->displayNameText != nullptr;
		const bool quest = text->ownerQuest != nullptr;
		a_counts.customName += custom;
		a_counts.displayText += displayText;
		a_counts.ownerQuest += quest;
		if (custom || displayText || quest) {
			if (HasHangul(name)) {
				++a_counts.guardedHangul;
			}
			if (HasHangul(name) || g_dryRun) {
				LogBucket(a_detail, a_where, custom ? "custom" : displayText ? "displayText" : "ownerQuest", a_ref, a_base, name);
			}
			return;
		}

		const auto inst = a_list->GetByType<RE::BGSObjectInstanceExtra>();
		if (!inst) {
			++a_counts.textOnly;
			if (HasHangul(name)) {
				++a_counts.textOnlyHangul;
			}
			if (HasHangul(name) || g_dryRun) {
				LogBucket(a_detail, a_where, "textOnly", a_ref, a_base, name);
			}
			return;
		}

		const auto data = a_list->GetByType<RE::ExtraInstanceData>();
		if (!data || !data->data) {
			++a_counts.noInstanceData;
			if (HasHangul(name)) {
				++a_counts.noInstanceDataHangul;
			}
			if (HasHangul(name) || g_dryRun) {
				LogBucket(a_detail, a_where, "noInstanceData", a_ref, a_base, name);
			}
			return;
		}

		if (g_dryRun) {
			++a_counts.rebuilt;  // would rebuild
			if (a_detail.TakeChange()) {
				logger::info("[NameRefresh] {} candidate ref {:08X} base {:08X} type {} text \"{}\" base FULL \"{}\"", a_where, a_ref,
					a_base->GetFormID(), TypeName(a_base), name, FullNameOf(a_base));
			}
			return;
		}

		// The call may delete the text extra, so keep copies of everything read from it
		const std::string                       before{ name };
		const auto                              modsBefore = ModList(inst);
		const auto                              dataBefore = data;
		const auto                              instanceBefore = data->data.get();
		RE::BSTSmartPointer<RE::TBO_InstanceData> held = data->data;

		a_list->SetDisplayNameFromInstanceData(inst, a_base, held);
		++a_counts.rebuilt;

		const auto textAfter = a_list->GetByType<RE::ExtraTextDisplayData>();
		const auto dataAfter = a_list->GetByType<RE::ExtraInstanceData>();
		if (dataAfter != dataBefore || !dataAfter || dataAfter->data.get() != instanceBefore) {
			++a_counts.instanceChanged;
		}
		if (ModList(a_list->GetByType<RE::BGSObjectInstanceExtra>()) != modsBefore) {
			++a_counts.modsChanged;
		}

		if (!textAfter) {
			++a_counts.removed;
			if (a_detail.TakeChange()) {
				logger::info("[NameRefresh] {} removed ref {:08X} base {:08X} type {} before \"{}\"", a_where, a_ref, a_base->GetFormID(),
					TypeName(a_base), before);
			}
			return;
		}
		const std::string after{ TextOf(textAfter) };
		const bool        hangul = HasHangul(after);
		if (after != before) {
			++a_counts.changed;
			if (a_detail.TakeChange()) {
				logger::info("[NameRefresh] {} changed ref {:08X} base {:08X} type {} before \"{}\" after \"{}\"", a_where, a_ref, a_base->GetFormID(),
					TypeName(a_base), before, after);
			}
		}
		if (hangul) {
			++a_counts.stillHangul;
		}
	}

	// Walks the existing inventory list of a reference (no list is built here) and rebuilds the stacks whose name has Hangul
	void WalkContainer(RE::TESObjectREFR* a_ref, ContainerCounts& a_out, Counts& a_counts, Detail& a_detail)
	{
		const auto list = a_ref ? a_ref->inventoryList : nullptr;
		if (!list) {
			return;
		}
		++a_out.lists;
		const auto               start = Clock::now();
		const RE::BSAutoReadLock lock{ list->rwLock };
		for (auto& item : list->data) {
			for (auto stack = item.stackData.get(); stack; stack = stack->nextStack.get()) {
				++a_out.stacks;
				const auto extra = stack->extra.get();
				const auto text = extra ? extra->GetByType<RE::ExtraTextDisplayData>() : nullptr;
				if (!text) {
					continue;
				}
				++a_out.withText;
				if (!HasHangul(TextOf(text))) {
					continue;
				}
				++a_out.hangul;
				if (a_detail.enabled) {
					const auto owner = a_ref->GetObjectReference();
					logger::info("[NameRefresh] container owner ref {:08X} base {:08X} type {} item {:08X} type {} text \"{}\"", a_ref->GetFormID(),
						owner ? owner->GetFormID() : 0u, TypeName(owner), item.object ? item.object->GetFormID() : 0u, TypeName(item.object), TextOf(text));
				}
				Process(extra, item.object, a_ref->GetFormID(), "container", a_counts, a_detail);
			}
		}
		a_out.us += MicrosSince(start);
	}

	// NiTArray keeps holes: size() counts the non-null slots, and the u16 at +0x12 is one past the highest slot in use
	// (OG fork layout; AE proven in docs/phaseE-namerefresh-proof.md fact 9). Walk to that index and skip nulls.
	template <class Array>
	[[nodiscard]] std::uint16_t HighWater(const Array& a_array)
	{
		const auto used = *reinterpret_cast<const std::uint16_t*>(reinterpret_cast<const std::byte*>(std::addressof(a_array)) + 0x12);
		return static_cast<std::uint16_t>(std::min<std::size_t>(used, a_array.capacity()));
	}

	struct WalkTotals
	{
		std::size_t cells{ 0 };
		std::size_t refs{ 0 };
		std::size_t duplicates{ 0 };
		std::size_t worldspaces{ 0 };
	};

	// Copies the cell's references under its lock, then rebuilds names from the copies without the lock
	void WalkCell(RE::TESObjectCELL* a_cell, std::string_view a_where, std::vector<RE::NiPointer<RE::TESObjectREFR>>& a_buffer, WalkTotals& a_totals,
		Counts& a_counts, ContainerCounts& a_containers, Counts& a_containerCounts, Detail& a_detail, Detail& a_containerDetail)
	{
		if (!a_cell) {
			return;
		}
		a_buffer.clear();
		{
			const RE::BSAutoLock<RE::BSSpinLock> lock{ a_cell->spinLock };
			if (a_cell->references.empty()) {
				return;
			}
			a_buffer.assign(a_cell->references.begin(), a_cell->references.end());
		}
		++a_totals.cells;
		for (const auto& ref : a_buffer) {
			if (!ref) {
				continue;
			}
			++a_totals.refs;
			Process(ref->extraList.get(), ref->GetObjectReference(), ref->GetFormID(), a_where, a_counts, a_detail);
			WalkContainer(ref.get(), a_containers, a_containerCounts, a_containerDetail);
		}
		a_buffer.clear();
	}

	void WalkPlayerInventory(Counts& a_counts, Detail& a_detail, std::size_t& a_stacks)
	{
		const auto player = DSD::GetPlayer();
		const auto list = player ? player->inventoryList : nullptr;
		if (!list) {
			return;
		}
		const RE::BSAutoReadLock lock{ list->rwLock };
		for (auto& item : list->data) {
			for (auto stack = item.stackData.get(); stack; stack = stack->nextStack.get()) {
				++a_stacks;
				Process(stack->extra.get(), item.object, 0, "inv", a_counts, a_detail);
			}
		}
	}

	// A reference seen in testing (Tull Collection.esp 002872)
	void TraceHolster(std::string_view a_tag, std::string_view a_phase)
	{
		const auto handler = RE::TESDataHandler::GetSingleton();
		const auto ref = handler ? handler->LookupForm<RE::TESObjectREFR>(0x002872, "Tull Collection.esp") : nullptr;
		if (!ref) {
			logger::info("[NameRefresh] {} holster {}: reference not in memory", a_tag, a_phase);
			return;
		}
		const auto list = ref->extraList.get();
		const auto text = list ? list->GetByType<RE::ExtraTextDisplayData>() : nullptr;
		const auto base = ref->GetObjectReference();
		const auto full = base ? base->As<RE::TESFullName>() : nullptr;
		logger::info("[NameRefresh] {} holster {}: ref {:08X} text extra {} \"{}\" instance {} instanceData {} base FULL \"{}\"", a_tag, a_phase,
			ref->GetFormID(), text != nullptr, TextOf(text), list && list->GetByType<RE::BGSObjectInstanceExtra>() != nullptr,
			list && list->GetByType<RE::ExtraInstanceData>() != nullptr, full ? static_cast<std::string_view>(full->fullName) : std::string_view{});
	}

	void LogPrologue()
	{
		static bool done = false;
		if (std::exchange(done, true)) {
			return;
		}
		const auto  address = REL::ID(457340).address();
		std::string hex;
		for (std::size_t i = 0; i < 16; ++i) {
			hex += std::format("{:02X} ", *reinterpret_cast<const std::uint8_t*>(address + i));
		}
		logger::info("[NameRefresh] SetDisplayNameFromInstanceData (ID 457340) at {:X}, first 16 bytes: {}", address, hex);
	}

	void Run(std::string_view a_tag)
	{
		const auto total = Clock::now();
		// AE: the two engine functions the walk calls must have the bytes the proof saw (docs/phaseE-namerefresh-proof.md)
		if (DSD::IsAE()) {
			static const bool codeOk = [] {
				const REL::Relocation<std::uintptr_t> rebuild{ REL::ID(2190179) };
				const REL::Relocation<std::uintptr_t> spinLock{ REL::ID(2192245) };
				return HookUtil::CodeMatches(rebuild.address(), AeHookBytes::kSetDisplayNameFromInstanceData, "ExtraDataList::SetDisplayNameFromInstanceData") &&
				       HookUtil::CodeMatches(spinLock.address(), AeHookBytes::kSpinLockLock, "BSSpinLock::lock");
			}();
			if (!codeOk) {
				logger::warn("[NameRefresh] {}: engine code differs from the proven AE bytes, nothing done", a_tag);
				return;
			}
		}
		const auto handler = DSD::GetDataHandler();
		if (!handler) {
			logger::info("[NameRefresh] {}: no data handler, nothing done", a_tag);
			return;
		}

		g_dryRun = Settings::GetSingleton().nameRefreshDryRun;
		Detail detail;
		detail.enabled = Settings::GetSingleton().logNameRefresh;
		Detail containerDetail;  // has its own cap: the walk reaches the containers after the placed items used theirs
		containerDetail.enabled = detail.enabled;
		if (g_dryRun) {
			detail.cap = containerDetail.cap = std::numeric_limits<std::size_t>::max() / 2;
		}
		// OG only: both use fork wrappers whose IDs are OG IDs (the prologue probe reads ID 457340, the holster trace
		// calls TESDataHandler::LookupForm)
		const bool ogTrace = detail.enabled && !DSD::IsAE();
		if (ogTrace) {
			LogPrologue();
			TraceHolster(a_tag, "before");
		}

		Counts                                       refCounts;
		ContainerCounts                              containers;
		Counts                                       containerCounts;
		WalkTotals                                   interior;
		WalkTotals                                   exterior;
		std::vector<RE::NiPointer<RE::TESObjectREFR>> buffer;

		auto start = Clock::now();
		const auto interiorEnd = HighWater(handler->interiorCells);
		for (std::uint16_t i = 0; i < interiorEnd; ++i) {
			WalkCell(handler->interiorCells[i], "interior", buffer, interior, refCounts, containers, containerCounts, detail, containerDetail);
		}
		const auto interiorUs = MicrosSince(start);

		start = Clock::now();
		boost::unordered_flat_set<const RE::TESObjectCELL*> seen;
		const auto                                          visit = [&](RE::TESObjectCELL* a_cell) {
            if (!a_cell) {
                return;
            }
            if (!seen.insert(a_cell).second) {
                ++exterior.duplicates;
                return;
            }
            WalkCell(a_cell, "exterior", buffer, exterior, refCounts, containers, containerCounts, detail, containerDetail);
		};
		for (const auto world : handler->GetFormArray<RE::TESWorldSpace>()) {
			if (!world) {
				continue;
			}
			++exterior.worldspaces;
			visit(world->persistentCell);
			for (const auto& entry : world->cellMap) {
				visit(entry.second);
			}
		}
		const auto exteriorUs = MicrosSince(start);

		start = Clock::now();
		Counts      invCounts;
		std::size_t invStacks = 0;
		WalkPlayerInventory(invCounts, detail, invStacks);
		const auto inventoryUs = MicrosSince(start);

		const auto line = [](const Counts& c) {
			return std::format("seen {} otherType {} (Hangul {}) custom {} displayText {} ownerQuest {} (Hangul {}) textOnly {} (Hangul {}) noInstanceData {} (Hangul {}) "
							   "rebuilt {} changed {} stillHangul {} removed {} instanceChanged {} modsChanged {}",
				c.seen, c.otherType, c.otherTypeHangul, c.customName, c.displayText, c.ownerQuest, c.guardedHangul, c.textOnly, c.textOnlyHangul, c.noInstanceData,
				c.noInstanceDataHangul, c.rebuilt, c.changed, c.stillHangul, c.removed, c.instanceChanged, c.modsChanged);
		};
		logger::info("[NameRefresh] {} summary{}: interior cells {} refs {} | worldspaces {} exterior cells {} refs {} duplicateCells {} | "
					 "us interior {} exterior {} inventory {} total {} | refs: {} | player inventory stacks {}: {} | "
					 "containers lists {} stacks {} withText {} hangul {} us {} (inside interior/exterior): {}",
			a_tag, g_dryRun ? " (dry run: rebuilt = would rebuild, nothing written)" : "", interior.cells, interior.refs, exterior.worldspaces, exterior.cells, exterior.refs, exterior.duplicates, interiorUs, exteriorUs,
			inventoryUs, MicrosSince(total), line(refCounts), invStacks, line(invCounts), containers.lists, containers.stacks, containers.withText,
			containers.hangul, containers.us, line(containerCounts));

		if (ogTrace) {
			TraceHolster(a_tag, "after");
		}
	}
}

namespace NameRefresh
{
	void Apply(std::string_view a_tag)
	{
		try {
			Run(a_tag);
		} catch (const std::exception& e) {
			logger::error("[NameRefresh] {}: stopped by exception: {}", a_tag, e.what());
		} catch (...) {
			logger::error("[NameRefresh] {}: stopped by unknown exception", a_tag);
		}
	}
}