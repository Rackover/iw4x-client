#pragma once

namespace Components
{
	class Core : public Component
	{
	public:
		Core();

	private:
		static Game::XAssetEntry* __cdecl AddUniqueWeaponXAsset(Game::XAssetType type, Game::XAssetHeader* a2);
		static std::unordered_set<std::string> loadedWeapons;
	};
}
