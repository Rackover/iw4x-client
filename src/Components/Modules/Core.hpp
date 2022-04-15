#pragma once

namespace Components
{
	class Core : public Component
	{
	public:
		Core();

	private:
		static Game::XAssetEntry* AddUniqueWeaponXAsset(Game::XAssetType type, Game::XAssetHeader* header);
		static std::unordered_set<std::string> loadedWeapons;
	};
}
