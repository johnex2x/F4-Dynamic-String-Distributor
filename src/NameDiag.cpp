#include "NameDiag.h"

#include "Hangul.h"

#include <mutex>

namespace
{
	constexpr std::size_t kWalkDetailCap = 200;   // detail lines per walk
	constexpr std::size_t kCellDetailCap = 500;   // detail lines for all per-cell walks of the session
	constexpr std::size_t kArmoDetailCap = 30;

	using Clock = std::chrono::steady_clock;

	[[nodiscard]] std::int64_t MicrosSince(Clock::time_point a_start)
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - a_start).count();
	}

	[[nodiscard]] std::string_view BaseName(const RE::TESBoundObject* a_base)
	{
		if (!a_base) {
			return {};
		}
		const auto full = const_cast<RE::TESBoundObject*>(a_base)->As<RE::TESFullName>();
		return full ? static_cast<std::string_view>(full->fullName) : std::string_view{};
	}

	struct WalkCounts
	{
		std::size_t cells{ 0 };
		std::size_t refs{ 0 };
		std::size_t withExtra{ 0 };
		std::size_t hangul{ 0 };
		std::size_t details{ 0 };  // detail lines written by this walk
	};

	[[nodiscard]] const RE::ExtraTextDisplayData* TextExtra(const RE::ExtraDataList* a_extra)
	{
		return a_extra ? a_extra->GetByType<RE::ExtraTextDisplayData>() : nullptr;
	}

	// Walks one cell's references. a_budget is the number of detail lines still allowed.
	void WalkCell(RE::TESObjectCELL* a_cell, std::string_view a_tag, WalkCounts& a_counts, std::size_t& a_budget)
	{
		if (!a_cell) {
			return;
		}
		++a_counts.cells;
		for (const auto& ref : a_cell->references) {
			if (!ref) {
				continue;
			}
			++a_counts.refs;
			// Only the extra list is read: no GetDisplayFullName, so no 3D load and no instance build
			const auto extra = TextExtra(ref->extraList.get());
			if (!extra) {
				continue;
			}
			++a_counts.withExtra;
			const auto text = static_cast<std::string_view>(extra->displayName);
			if (!HasHangul(text)) {
				continue;
			}
			++a_counts.hangul;
			if (a_budget == 0) {
				continue;
			}
			--a_budget;
			++a_counts.details;
			const auto base = ref->GetObjectReference();
			logger::info("[NameDiag] {} ref {:08X} base {:08X} type {} extra \"{}\" base FULL \"{}\"", a_tag, ref->GetFormID(),
				base ? base->GetFormID() : 0u, base ? base->GetFormTypeString() : "none", text, BaseName(base));
		}
	}

	// Player inventory stacks that carry an extra text
	void WalkInventory(std::string_view a_tag, std::size_t& a_budget)
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		const auto list = player ? player->inventoryList : nullptr;
		if (!list) {
			return;
		}
		const auto start = Clock::now();
		std::size_t stacks = 0;
		std::size_t withExtra = 0;
		std::size_t hangul = 0;
		{
			const RE::BSAutoReadLock lock{ list->rwLock };
			for (auto& item : list->data) {
				for (auto stack = item.stackData.get(); stack; stack = stack->nextStack.get()) {
					++stacks;
					const auto extra = TextExtra(stack->extra.get());
					if (!extra) {
						continue;
					}
					++withExtra;
					const auto text = static_cast<std::string_view>(extra->displayName);
					if (!HasHangul(text)) {
						continue;
					}
					++hangul;
					if (a_budget == 0) {
						continue;
					}
					--a_budget;
					logger::info("[NameDiag] {} inv base {:08X} type {} extra \"{}\" base FULL \"{}\"", a_tag,
						item.object ? item.object->GetFormID() : 0u, item.object ? item.object->GetFormTypeString() : "none", text, BaseName(item.object));
				}
			}
		}
		logger::info("[NameDiag] {} inventory: {} stacks, {} with extra text, {} with Hangul, {} us", a_tag, stacks, withExtra, hangul, MicrosSince(start));
	}

	// A reference seen in testing (Tull Collection.esp 002872): which extras it holds decides where the Korean name lives.
	// a_cell set: only when the reference is in that cell.
	void TraceHolster(std::string_view a_tag, const RE::TESObjectCELL* a_cell = nullptr)
	{
		const auto handler = RE::TESDataHandler::GetSingleton();
		const auto ref = handler ? handler->LookupForm<RE::TESObjectREFR>(0x002872, "Tull Collection.esp") : nullptr;
		if (a_cell && (!ref || ref->parentCell != a_cell)) {
			return;
		}
		if (!ref) {
			logger::info("[NameDiag] {} holster: reference not in memory", a_tag);
			return;
		}
		std::string types;
		const auto  list = ref->extraList.get();
		if (list) {
			for (std::uint32_t i = 0; i < 0x100; ++i) {
				if (list->GetByType(static_cast<RE::EXTRA_DATA_TYPE>(i))) {
					types += std::format("{:02X} ", i);
				}
			}
		}
		const auto extra = TextExtra(list);
		const auto base = ref->GetObjectReference();
		logger::info("[NameDiag] {} holster: ref {:08X} cell {:08X} 3D {} extras [{}] text extra \"{}\" base FULL \"{}\"", a_tag, ref->GetFormID(),
			ref->parentCell ? ref->parentCell->GetFormID() : 0u, ref->Get3D() != nullptr, types,
			extra ? static_cast<std::string_view>(extra->displayName) : std::string_view{}, BaseName(base));
	}

	// Cells that the game attached since the sink was registered, by FormID (the pointer is checked before use).
	// CommonLibF4 has no TES class, so the grid cells cannot be listed; the attach event stands in for them.
	std::mutex                                                g_cellsLock;
	std::unordered_map<std::uint32_t, RE::TESObjectCELL*>     g_attached;
	std::size_t                                               g_cellBudget{ kCellDetailCap };

	class CellSink final : public RE::BSTEventSink<RE::CellAttachDetachEvent>
	{
	public:
		RE::BSEventNotifyControl ProcessEvent(const RE::CellAttachDetachEvent& a_event, RE::BSTEventSource<RE::CellAttachDetachEvent>*) override
		{
			using Type = RE::CellAttachDetachEvent::EVENT_TYPE;
			const auto cell = a_event.cell;
			if (!cell) {
				return RE::BSEventNotifyControl::kContinue;
			}
			const auto type = a_event.type.get();
			if (type == Type::kPreDetach || type == Type::kPostDetach) {
				const std::scoped_lock lock{ g_cellsLock };
				g_attached.erase(cell->GetFormID());
			} else if (type == Type::kPostAttach) {
				{
					const std::scoped_lock lock{ g_cellsLock };
					g_attached[cell->GetFormID()] = cell;
				}
				const auto start = Clock::now();
				WalkCounts counts;
				std::size_t budget;
				{
					const std::scoped_lock lock{ g_cellsLock };
					budget = g_cellBudget;
				}
				WalkCell(cell, "cell", counts, budget);
				{
					const std::scoped_lock lock{ g_cellsLock };
					g_cellBudget -= std::min(g_cellBudget, counts.details);
				}
				logger::info("[NameDiag] cell {:08X}: {} refs, {} with extra text, {} with Hangul, {} us",
					cell->GetFormID(), counts.refs, counts.withExtra, counts.hangul, MicrosSince(start));
				TraceHolster("cell", cell);
			}
			return RE::BSEventNotifyControl::kContinue;
		}
	};

	CellSink g_sink;
}

namespace NameDiag
{
	void Install()
	{
		static bool installed = false;
		if (std::exchange(installed, true)) {
			return;
		}
		// Singleton() has an OG ID (862142) in the 1.10.163 address library
		RE::CellAttachDetachEventSource::CellAttachDetachEventSourceSingleton::GetSingleton().source.RegisterSink(&g_sink);
		logger::info("[NameDiag] registered the cell attach sink");
	}

	void WalkLoaded(std::string_view a_tag)
	{
		const auto start = Clock::now();
		WalkCounts  counts;
		std::size_t budget = kWalkDetailCap;

		const auto player = RE::PlayerCharacter::GetSingleton();
		const auto current = player ? player->parentCell : nullptr;
		if (current) {
			WalkCell(current, a_tag, counts, budget);
		}
		// An interior cell is the whole loaded world; outside, add the cells the attach event reported
		if (!current || !current->IsInterior()) {
			std::vector<RE::TESObjectCELL*> others;
			{
				const std::scoped_lock lock{ g_cellsLock };
				for (const auto& [id, cell] : g_attached) {
					if (cell != current && RE::TESForm::GetFormByID(id) == cell) {
						others.push_back(cell);
					}
				}
			}
			for (const auto cell : others) {
				WalkCell(cell, a_tag, counts, budget);
			}
		}
		logger::info("[NameDiag] {} walk: {} cells, {} refs, {} with extra text, {} with Hangul, {} us", a_tag, counts.cells, counts.refs,
			counts.withExtra, counts.hangul, MicrosSince(start));

		std::size_t invBudget = kWalkDetailCap;
		WalkInventory(a_tag, invBudget);
		TraceHolster(a_tag);
	}

	void WalkInteriors(std::string_view a_tag)
	{
		const auto handler = RE::TESDataHandler::GetSingleton();
		if (!handler) {
			return;
		}
		const auto  start = Clock::now();
		WalkCounts  counts;
		std::size_t budget = kWalkDetailCap;
		std::size_t differ = 0;
		std::size_t differLines = 0;
		std::size_t total = 0;
		for (std::uint32_t i = 0; i < handler->interiorCells.size(); ++i) {
			const auto cell = handler->interiorCells[i];
			if (!cell) {
				continue;
			}
			++total;
			if (cell->references.empty()) {
				continue;
			}
			logger::info("[NameDiag] {} interior in memory: {:08X} {} refs", a_tag, cell->GetFormID(), cell->references.size());
			WalkCell(cell, a_tag, counts, budget);
			for (const auto& ref : cell->references) {
				const auto extra = ref ? TextExtra(ref->extraList.get()) : nullptr;
				if (!extra) {
					continue;
				}
				const auto base = ref->GetObjectReference();
				const auto text = static_cast<std::string_view>(extra->displayName);
				if (text != BaseName(base)) {
					++differ;
					if (differLines++ < kWalkDetailCap) {
						logger::info("[NameDiag] {} differs ref {:08X} base {:08X} type {} extra \"{}\" base FULL \"{}\"", a_tag, ref->GetFormID(),
							base ? base->GetFormID() : 0u, base ? base->GetFormTypeString() : "none", text, BaseName(base));
					}
				} else if (differLines++ < kWalkDetailCap) {
					logger::info("[NameDiag] {} same ref {:08X} base {:08X} type {} text \"{}\"", a_tag, ref->GetFormID(),
						base ? base->GetFormID() : 0u, base ? base->GetFormTypeString() : "none", text);
				}
			}
		}
		logger::info("[NameDiag] {} interiors: {} known, {} in memory, {} refs, {} with extra text, {} with Hangul, {} differ from base, {} us", a_tag, total,
			counts.cells, counts.refs, counts.withExtra, counts.hangul, differ, MicrosSince(start));
		TraceHolster(a_tag);
	}

	void ArmoCensus(std::string_view a_tag)
	{
		const auto start = Clock::now();
		const auto handler = RE::TESDataHandler::GetSingleton();
		if (!handler) {
			return;
		}

		std::size_t total = 0;
		std::size_t korean = 0;
		std::size_t counts[2][2]{};  // [has named object template][has INRD]
		std::vector<std::string> first;

		for (const auto armo : handler->GetFormArray<RE::TESObjectARMO>()) {
			if (!armo) {
				continue;
			}
			++total;
			const auto name = static_cast<std::string_view>(armo->fullName);
			if (!HasHangul(name)) {
				continue;
			}
			++korean;
			bool templated = false;
			for (const auto item : armo->objectTemplate.items) {
				if (item && !static_cast<std::string_view>(item->fullName).empty()) {
					templated = true;
					break;
				}
			}
			const bool inrd = armo->instanceNamingRules != nullptr;
			++counts[templated][inrd];
			if (first.size() < kArmoDetailCap) {
				first.push_back(std::format("{:08X} EDID \"{}\" name \"{}\"", armo->GetFormID(), armo->GetFormEditorID(), name));
			}
		}

		logger::info("[NameDiag] {} ARMO census: {} of {} with Hangul in own name; template+INRD {}, template only {}, INRD only {}, neither {}; {} us",
			a_tag, korean, total, counts[1][1], counts[1][0], counts[0][1], counts[0][0], MicrosSince(start));
		for (const auto& line : first) {
			logger::info("[NameDiag] {} ARMO {}", a_tag, line);
		}
	}
}
