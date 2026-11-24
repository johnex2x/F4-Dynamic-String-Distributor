#pragma once

#include "ConstTranslation.h"

// Object template names (FULL index > 1 on ARMO, WEAP, NPC_) and item naming rules (INNR WNAM).
// Index rules: include/core/NameSlots.h
namespace TemplateNames
{
	// Types whose FULL index counts object template names too
	[[nodiscard]] constexpr bool IsTemplateAwareType(DSD::TypeCode a_type) noexcept
	{
		const auto record = ConstTranslation::Record(a_type);
		return ConstTranslation::Subrecord(a_type) == ConstTranslation::Signature("FULL") &&
		       (record == ConstTranslation::Signature("ARMO") || record == ConstTranslation::Signature("WEAP") || record == ConstTranslation::Signature("NPC_"));
	}

	enum class Result
	{
		kOwnName,
		kTemplateName,
		kNoSlot,   // the index points to a name the form does not have
		kNotNamed  // form is not a template-aware record type
	};

	// Writes the name that FULL index a_index stands for. Does not call AddChange.
	// Skips the write when the name already holds the text; a_unchanged (optional) reports that.
	[[nodiscard]] Result WriteName(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text, bool* a_unchanged = nullptr);

	// FULL index of the form's own name in the translation files (1 unless the NPC has template names)
	[[nodiscard]] std::uint32_t OwnNameIndex(const RE::TESForm* a_form);

	// Applies INNR WNAM and logs the result
	void ApplyInnr();
}
