#pragma once

// Which translation a player dialogue option shows, decided from form IDs only.
// The game's visible text is never compared: the game strings can be in another language than the JSON `original` (for example Korean game text, English originals).
// No game objects are touched here, so the rules can be tested without Fallout 4 running.

#include <cstdint>
#include <span>
#include <string_view>

#include "core/TranslationStore.h"

namespace DSD
{
	// One INFO of the chain that starts at the option's INFO and follows TESTopicInfo::dataInfo (shared-response INFOs)
	struct InfoLink
	{
		std::uint32_t formID{ 0 };
		std::uint32_t responseID{ 0 };        // responseID of the first response this INFO owns; only meaningful if hasResponses
		bool          hasResponses{ false };  // this INFO owns a response list (a shared-response INFO has none and points on)
	};

	enum class OptionSource
	{
		kNone,
		kInfoRnam,  // INFO RNAM: the option text
		kInfoNam1,  // INFO NAM1: the player's spoken response, which XDI shows when an INFO has no RNAM
		kDialFull,  // DIAL FULL: the topic name, which the vanilla wheel shows when an INFO has no RNAM
	};

	[[nodiscard]] std::string_view OptionSourceName(OptionSource a_source) noexcept;

	struct OptionHit
	{
		const Entry*  entry{ nullptr };
		OptionSource  source{ OptionSource::kNone };
		std::uint32_t formID{ 0 };  // the INFO (or DIAL) form the entry was found for

		explicit operator bool() const noexcept { return entry != nullptr; }
	};

	// INFO RNAM of the first INFO in the chain that has one. xTranslator exports RNAM with index 1; upstream DSD files use no index.
	[[nodiscard]] OptionHit FindInfoPrompt(const TranslationStore& a_store, std::span<const InfoLink> a_chain);

	// INFO NAM1 of the response the chain owns, keyed like the DialogueResponse hook: the first INFO of the chain that owns
	// responses, with the responseID of its first response.
	[[nodiscard]] OptionHit FindInfoResponse(const TranslationStore& a_store, std::span<const InfoLink> a_chain);

	// Vanilla dialogue wheel: INFO RNAM, then DIAL FULL of the topic (a_topicFormID 0: no topic known)
	[[nodiscard]] OptionHit ResolveWheelOption(const TranslationStore& a_store, std::span<const InfoLink> a_chain, std::uint32_t a_topicFormID);

	// XDI option list: its prompt and response fields are replaced independently.
	// The prompt is INFO RNAM only; with no RNAM XDI shows the response field, so INFO NAM1 is looked up as well.
	struct XdiOption
	{
		OptionHit prompt;
		OptionHit response;

		explicit operator bool() const noexcept { return prompt || response; }
	};

	[[nodiscard]] XdiOption ResolveXdiOption(const TranslationStore& a_store, std::span<const InfoLink> a_chain);
}
