#include "core/DialogueOptions.h"

namespace DSD
{
	namespace
	{
		constexpr auto kPrompt = TypeOf("INFO RNAM");
		constexpr auto kResponse = TypeOf("INFO NAM1");
		constexpr auto kTopicName = TypeOf("DIAL FULL");
	}

	std::string_view OptionSourceName(OptionSource a_source) noexcept
	{
		switch (a_source) {
		case OptionSource::kInfoRnam:
			return "INFO RNAM";
		case OptionSource::kInfoNam1:
			return "INFO NAM1";
		case OptionSource::kDialFull:
			return "DIAL FULL";
		default:
			return "none";
		}
	}

	OptionHit FindInfoPrompt(const TranslationStore& a_store, std::span<const InfoLink> a_chain)
	{
		for (const auto& link : a_chain) {
			if (link.formID == 0) {
				continue;
			}
			auto entry = a_store.Find(kPrompt, link.formID, 1);
			if (!entry) {
				entry = a_store.Find(kPrompt, link.formID, 0);
			}
			if (entry) {
				return { entry, OptionSource::kInfoRnam, link.formID };
			}
		}
		return {};
	}

	OptionHit FindInfoResponse(const TranslationStore& a_store, std::span<const InfoLink> a_chain)
	{
		for (const auto& link : a_chain) {
			if (link.formID == 0 || !link.hasResponses) {
				continue;
			}
			// The first INFO that owns responses is the owner; a later one in the chain is never tried
			if (const auto entry = a_store.Find(kResponse, link.formID, link.responseID)) {
				return { entry, OptionSource::kInfoNam1, link.formID };
			}
			return {};
		}
		return {};
	}

	OptionHit ResolveWheelOption(const TranslationStore& a_store, std::span<const InfoLink> a_chain, std::uint32_t a_topicFormID)
	{
		if (const auto hit = FindInfoPrompt(a_store, a_chain)) {
			return hit;
		}
		if (a_topicFormID != 0) {
			// Same index rule as the name writer: xTranslator exports FULL with index 1
			auto entry = a_store.Find(kTopicName, a_topicFormID, 1);
			if (!entry) {
				entry = a_store.Find(kTopicName, a_topicFormID, 0);
			}
			if (entry) {
				return { entry, OptionSource::kDialFull, a_topicFormID };
			}
		}
		return {};
	}

	XdiOption ResolveXdiOption(const TranslationStore& a_store, std::span<const InfoLink> a_chain)
	{
		return { FindInfoPrompt(a_store, a_chain), FindInfoResponse(a_store, a_chain) };
	}
}
