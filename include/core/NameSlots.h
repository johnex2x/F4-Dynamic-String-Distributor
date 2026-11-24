#pragma once

// Game-independent index rules for object template names (FULL index > 1).
// The rules were checked against Fallout4.esm and the xTranslator export Fallout4.json (see ticket 11).
// INNR WNAM is matched by text instead (TranslationStore::RuleText).

#include <cstddef>
#include <cstdint>
#include <optional>

namespace DSD
{
	// Where the form's own FULL sits among the FULL subrecords of its record.
	// ARMO, WEAP: own name first, then one FULL per object template that has a name.
	// NPC_: the object template names come first, the own name is the last FULL.
	enum class NameLayout
	{
		kOwnFirst,
		kOwnLast
	};

	struct NameSlot
	{
		enum class Kind
		{
			kNone,
			kOwn,
			kTemplate
		};

		Kind        kind{ Kind::kNone };
		std::size_t templateIndex{ 0 };  // position among the templates that have a name (0-based)
	};

	// a_ownNamed: the form's own name is not empty.
	// a_namedTemplates: number of object templates whose name is not empty.
	// Templates without a name have no FULL subrecord, so they do not take an index.
	// Index 0 (no index in an upstream DSD file) is always the own name.
	// Without named templates, index 1 is the own name (the case for every other record type).
	[[nodiscard]] NameSlot ResolveFullIndex(NameLayout a_layout, bool a_ownNamed, std::size_t a_namedTemplates, std::uint32_t a_index) noexcept;

	// Index of the own name in the translation files
	[[nodiscard]] std::uint32_t OwnNameIndex(NameLayout a_layout, bool a_ownNamed, std::size_t a_namedTemplates) noexcept;
}
