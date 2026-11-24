#include "core/NameSlots.h"

namespace DSD
{
	NameSlot ResolveFullIndex(NameLayout a_layout, bool a_ownNamed, std::size_t a_namedTemplates, std::uint32_t a_index) noexcept
	{
		using Kind = NameSlot::Kind;

		if (a_index == 0) {
			return { Kind::kOwn, 0 };
		}
		if (a_namedTemplates == 0) {
			return a_index == 1 ? NameSlot{ Kind::kOwn, 0 } : NameSlot{};
		}

		if (a_layout == NameLayout::kOwnFirst) {
			const std::size_t first = a_ownNamed ? 2 : 1;  // ordinal of the first template name
			if (a_ownNamed && a_index == 1) {
				return { Kind::kOwn, 0 };
			}
			const std::size_t position = a_index - first;
			return a_index >= first && position < a_namedTemplates ? NameSlot{ Kind::kTemplate, position } : NameSlot{};
		}

		if (a_index <= a_namedTemplates) {
			return { Kind::kTemplate, a_index - 1 };
		}
		return a_index == a_namedTemplates + 1 && a_ownNamed ? NameSlot{ Kind::kOwn, 0 } : NameSlot{};
	}

	std::uint32_t OwnNameIndex(NameLayout a_layout, bool a_ownNamed, std::size_t a_namedTemplates) noexcept
	{
		if (a_namedTemplates == 0 || !a_ownNamed) {
			return 1;
		}
		return a_layout == NameLayout::kOwnFirst ? 1 : static_cast<std::uint32_t>(a_namedTemplates + 1);
	}
}
