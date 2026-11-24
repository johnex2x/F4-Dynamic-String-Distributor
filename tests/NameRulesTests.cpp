#include "ConstTranslation.h"
#include "core/NameSlots.h"

#include <catch2/catch_test_macros.hpp>

using namespace DSD;
using namespace ConstTranslation;

TEST_CASE("name writer only takes base names", "[names]")
{
	CHECK(IsBaseNameType(TypeOf("ARMO FULL")));
	CHECK(IsBaseNameType(TypeOf("NPC_ FULL")));
	CHECK(IsBaseNameType(TypeOf("STAT FULL")));
	CHECK_FALSE(IsBaseNameType(TypeOf("REFR FULL")));  // map markers, ticket 13
	CHECK(IsBaseNameType(TypeOf("DIAL FULL")));  // dialogue topics are TESFullName
	CHECK_FALSE(IsBaseNameType(TypeOf("ARMO DESC")));

	// ARMO FULL 00237688 has index 1 (own name) and 3/5/7 (object template names "Light"/"Medium"/"Heavy")
	CHECK(IsBaseNameIndex(0));
	CHECK(IsBaseNameIndex(1));
	CHECK_FALSE(IsBaseNameIndex(3));
}

// Index rules below were checked against Fallout4.esm + Fallout4.json (ticket 11 notes)
TEST_CASE("FULL index: ARMO/WEAP keep the own name first", "[names]")
{
	using Kind = NameSlot::Kind;

	// ARMO 00237688: own name + 7 named templates; Fallout4.json has index 1, 3, 5, 7 (the others are untranslated)
	CHECK(ResolveFullIndex(NameLayout::kOwnFirst, true, 7, 1).kind == Kind::kOwn);
	const auto light = ResolveFullIndex(NameLayout::kOwnFirst, true, 7, 3);
	CHECK(light.kind == Kind::kTemplate);
	CHECK(light.templateIndex == 1);
	CHECK(ResolveFullIndex(NameLayout::kOwnFirst, true, 7, 8).templateIndex == 6);
	CHECK(ResolveFullIndex(NameLayout::kOwnFirst, true, 7, 9).kind == Kind::kNone);

	// no own name: the first FULL is already a template name
	CHECK(ResolveFullIndex(NameLayout::kOwnFirst, false, 3, 1).kind == Kind::kTemplate);
	CHECK(ResolveFullIndex(NameLayout::kOwnFirst, false, 3, 3).templateIndex == 2);
	CHECK(ResolveFullIndex(NameLayout::kOwnFirst, false, 3, 4).kind == Kind::kNone);

	CHECK(OwnNameIndex(NameLayout::kOwnFirst, true, 7) == 1);
}

TEST_CASE("FULL index: NPC_ keeps the own name last", "[names]")
{
	using Kind = NameSlot::Kind;

	// NPC 00248270: 13 template names "Lost Synth 1..13"; the own name "Synth Strider" is index 14
	CHECK(ResolveFullIndex(NameLayout::kOwnLast, true, 13, 1).kind == Kind::kTemplate);
	CHECK(ResolveFullIndex(NameLayout::kOwnLast, true, 13, 13).templateIndex == 12);
	CHECK(ResolveFullIndex(NameLayout::kOwnLast, true, 13, 14).kind == Kind::kOwn);
	CHECK(ResolveFullIndex(NameLayout::kOwnLast, true, 13, 15).kind == Kind::kNone);
	CHECK(OwnNameIndex(NameLayout::kOwnLast, true, 13) == 14);

	// NPC 00249AF7: one template name, the own name is index 2
	CHECK(ResolveFullIndex(NameLayout::kOwnLast, true, 1, 2).kind == Kind::kOwn);

	// no named template: index 1 is the own name
	CHECK(ResolveFullIndex(NameLayout::kOwnLast, true, 0, 1).kind == Kind::kOwn);
	CHECK(ResolveFullIndex(NameLayout::kOwnLast, true, 0, 0).kind == Kind::kOwn);
	CHECK(ResolveFullIndex(NameLayout::kOwnLast, true, 0, 2).kind == Kind::kNone);
	CHECK(OwnNameIndex(NameLayout::kOwnLast, true, 0) == 1);
}
