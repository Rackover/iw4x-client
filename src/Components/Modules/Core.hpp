#pragma once

namespace Components
{
	class Core : public Component
	{
	public:
		Core();

	private:
		static Game::XAssetEntry* __cdecl AddXAssetIfWeaponUnderLimit(Game::XAssetType type, Game::XAssetHeader* a2);

		static int weaponsLoaded;
		static Game::XAssetEntry* lastWeaponEntry;
		static std::unordered_set<std::string> skippedWeapons;
		
		static unsigned int __cdecl G_GetWeaponIndexForName(const char* a1);
	};

	static BOOL __cdecl Sys_TempPriorityEnd(int a1);
}
