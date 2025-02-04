#pragma once

namespace Game
{
	typedef unsigned int(*BG_GetNumWeapons_t)();
	extern BG_GetNumWeapons_t BG_GetNumWeapons;

	typedef const char*(*BG_GetWeaponName_t)(unsigned int index);
	extern BG_GetWeaponName_t BG_GetWeaponName;

	typedef void*(*BG_LoadWeaponDef_LoadObj_t)(const char* name);
	extern BG_LoadWeaponDef_LoadObj_t BG_LoadWeaponDef_LoadObj;

	typedef WeaponCompleteDef*(*BG_LoadWeaponCompleteDefInternal_t)(const char* folder, const char* name);
	extern BG_LoadWeaponCompleteDefInternal_t BG_LoadWeaponCompleteDefInternal;

	typedef WeaponDef*(*BG_GetWeaponDef_t)(unsigned int weaponIndex);
	extern BG_GetWeaponDef_t BG_GetWeaponDef;

	typedef const char*(*BG_GetEntityTypeName_t)(int eType);
	extern BG_GetEntityTypeName_t BG_GetEntityTypeName;

	typedef bool(*BG_IsWeaponValid_t)(const playerState_s* ps, unsigned int weaponIndex);
	extern BG_IsWeaponValid_t BG_IsWeaponValid;

	typedef int(*BG_GetEquippedWeaponIndex_t)(const playerState_s* ps, unsigned int weaponIndex);
	extern BG_GetEquippedWeaponIndex_t BG_GetEquippedWeaponIndex;

	typedef int(*BG_GetViewModelWeaponIndex_t)(const playerState_s* ps);
	extern BG_GetViewModelWeaponIndex_t BG_GetViewModelWeaponIndex;

	typedef PlayerEquippedWeaponState*(*BG_GetEquippedWeaponState_t)(playerState_s* ps, unsigned int weaponIndex);
	extern BG_GetEquippedWeaponState_t BG_GetEquippedWeaponState;

	typedef int*(*BG_PlayerHasWeapon_t)(playerState_s* ps, unsigned int weaponIndex);
	extern BG_PlayerHasWeapon_t BG_PlayerHasWeapon;

	typedef Game::WeaponCompleteDef*(*BG_GetWeaponCompleteDef_t)(unsigned int weaponIndex);
	extern BG_GetWeaponCompleteDef_t BG_GetWeaponCompleteDef;

	
	bool BG_HasPerk(const unsigned int* perks, const Game::perksEnum perkIndex)
	{
		assert(perks);
		AssertIn(perkIndex, Game::PERK_COUNT);

		const std::size_t arrayIndex = perkIndex >> 5;
		AssertIn(arrayIndex, Game::PERK_ARRAY_COUNT);

		constexpr std::size_t BIT_MASK_SIZE = 31; // 0x1F
		const auto bitPos = perkIndex & BIT_MASK_SIZE;
		const auto bitMask = 1 << bitPos;

		return perks[arrayIndex] & bitMask;
	}
}
