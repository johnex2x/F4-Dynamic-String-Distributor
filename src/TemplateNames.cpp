#include "TemplateNames.h"

#include "Manager.h"
#include "Runtime.h"
#include "TextWrite.h"
#include "core/NameSlots.h"

namespace
{
	[[nodiscard]] constexpr bool IsNamed(const RE::BGSLocalizedString& a_text) noexcept
	{
		return !static_cast<std::string_view>(a_text).empty();
	}

	[[nodiscard]] constexpr std::optional<DSD::NameLayout> LayoutOf(RE::ENUM_FORM_ID a_type) noexcept
	{
		switch (a_type) {
		case RE::ENUM_FORM_ID::kARMO:
		case RE::ENUM_FORM_ID::kWEAP:
			return DSD::NameLayout::kOwnFirst;
		case RE::ENUM_FORM_ID::kNPC_:
			return DSD::NameLayout::kOwnLast;
		default:
			return std::nullopt;
		}
	}

	// What the form looks like at runtime. Both values come from the game's own arrays:
	// an object template (OBTS) without a FULL subrecord has an empty name in BGSMod::Template::Items.
	struct FormNames
	{
		DSD::NameLayout                          layout;
		RE::TESFullName*                         own{ nullptr };
		std::vector<RE::BGSMod::Template::Item*> namedTemplates;

		[[nodiscard]] bool OwnNamed() const noexcept { return own && IsNamed(own->fullName); }
	};

	[[nodiscard]] std::optional<FormNames> Inspect(RE::TESForm* a_form)
	{
		const auto layout = LayoutOf(a_form->GetFormType());
		if (!layout) {
			return std::nullopt;
		}

		// ARMO, WEAP and NPC_ all derive from TESBoundObject, which holds the object template (TESBoundObjects.h, offset 0x30)
		FormNames names{ *layout };
		names.own = a_form->As<RE::TESFullName>();
		for (const auto item : static_cast<RE::TESBoundObject*>(a_form)->objectTemplate.items) {
			if (item && IsNamed(item->fullName)) {
				names.namedTemplates.push_back(item);
			}
		}
		return names;
	}
}

namespace TemplateNames
{
	Result WriteName(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text, bool* a_unchanged)
	{
		const auto names = Inspect(a_form);
		if (!names) {
			return Result::kNotNamed;
		}

		const auto slot = DSD::ResolveFullIndex(names->layout, names->OwnNamed(), names->namedTemplates.size(), a_index);
		switch (slot.kind) {
		case DSD::NameSlot::Kind::kOwn:
			if (names->own) {
				const bool changed = TextWrite::Assign(names->own->fullName, a_text);
				if (a_unchanged) {
					*a_unchanged = !changed;
				}
				return Result::kOwnName;
			}
			return Result::kNoSlot;
		case DSD::NameSlot::Kind::kTemplate: {
			const bool changed = TextWrite::Assign(names->namedTemplates[slot.templateIndex]->fullName, a_text);
			if (a_unchanged) {
				*a_unchanged = !changed;
			}
			return Result::kTemplateName;
		}
		default:
			return Result::kNoSlot;
		}
	}

	std::uint32_t OwnNameIndex(const RE::TESForm* a_form)
	{
		const auto names = Inspect(const_cast<RE::TESForm*>(a_form));
		return names ? DSD::OwnNameIndex(names->layout, names->OwnNamed(), names->namedTemplates.size()) : 1;
	}

	void ApplyInnr()
	{
		const auto& store = Manager::GetSingleton().Store();
		if (!store.HasRuleTexts()) {
			return;
		}

		std::size_t applied = 0;    // includes the unchanged ones
		std::size_t unchanged = 0;  // part of applied: the rule already held the text
		std::size_t unmatched = 0;  // rules with text that no entry has as its original
		std::size_t slotHits = 0;   // part of applied: by an entry with "rule"
		std::size_t slotOther = 0;  // an entry with "rule" at the position, but another text there (rule order differs)

		// Every rule of every INNR, whichever plugin's version won: matched by its text now (see TranslationStore::RuleText).
		// Layout from CommonLibF4 TESForms.h BGSInstanceNamingRules: ruleSets[10] at 0x28, RuleData.text at 0x00.
		for (const auto rules : DSD::GetDataHandler()->GetFormArray<RE::BGSInstanceNamingRules>()) {
			if (!rules) {
				continue;
			}
			// Rule position as the plugin file numbers it: all rule sets in order, 1-based (every rule has its WNAM)
			std::uint32_t position = 0;
			for (auto& set : rules->ruleSets) {
				for (auto& rule : set) {
					++position;
					const auto now = static_cast<std::string_view>(rule.text);
					if (now.empty()) {
						continue;
					}
					auto       match = DSD::TranslationStore::RuleMatch::kNoSlot;
					const auto text = store.RuleText(rules->GetFormID(), position, now, &match);
					if (match == DSD::TranslationStore::RuleMatch::kSlotOther) {
						++slotOther;
						logger::debug("INNR WNAM {:08X} rule {}: slot entry for another text, now \"{}\"", rules->GetFormID(), position, now);
					}
					slotHits += match == DSD::TranslationStore::RuleMatch::kSlot;
					if (!text) {
						++unmatched;
						logger::debug("INNR WNAM {:08X}: no entry for \"{}\"", rules->GetFormID(), now);
						continue;
					}
					unchanged += !TextWrite::Assign(rule.text, *text);
					++applied;
				}
			}
		}

		logger::info("  INNR WNAM : {:>6} applied ({} unchanged), {:>4} rules with no entry; by rule position {} applied, {} with another text there",
			applied, unchanged, unmatched, slotHits, slotOther);
	}
}
