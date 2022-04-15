#include <STDInclude.hpp>

namespace Components
{
	// Vanilla basegame supports up to 1200 weapons
	constexpr long DEFAULT_WEAPON_LIMIT = 1200;

	std::unordered_set<std::string> Core::loadedWeapons{};

	Core::Core()
	{
		// SteamIDs can only contain 31 bits of actual 'id' data.
		// The other 33 bits are steam internal data like universe and so on.
		// Using only 31 bits for fingerprints is pretty insecure.
		// The function below verifies the integrity steam's part of the SteamID.
		// Patching that check allows us to use 64 bit for fingerprints.
		// 
		// This also prevents WIN_NO_STEAM
		Utils::Hook::Set<DWORD>(0x4D0D60, 0xC301B0);

		// remove system pre-init stuff (improper quit, disk full)
		Utils::Hook::Set<BYTE>(0x411350, 0xC3);

		// remove STEAMSTART checking for DRM IPC
		Utils::Hook::Nop(0x451145, 5);
		Utils::Hook::Set<BYTE>(0x45114C, 0xEB);

		// disable playlist.ff loading function
		Utils::Hook::Set<BYTE>(0x4D6E60, 0xC3);

		// Override "Couldn't load image X" to never happen, because we have CODO assets that cannot be loaded without other more advanced components
		// Doing on the fly IWI conversion is not a job for Core component, so we just nop the error for now.
		Utils::Hook::Set<BYTE>(0x51F59F, 0xEB);

		// Asset limits for weapons - iw4x files contains duplicate for weapons, we make sure to only load them once
		Utils::Hook(0x4A473C, Core::AddUniqueWeaponXAsset, HOOK_CALL).install()->quick(); 

		/////////////
		/// various changes to SV_DirectConnect-y stuff to allow non-party joinees

		// Bypass Session_FindRegisteredUser check
		Utils::Hook::Set<WORD>(0x460D96, 0x90E9);

		// Bypass invitation check
		Utils::Hook::Set<BYTE>(0x460F0A, 0xEB);

		// EXE_TRANSMITERROR
		Utils::Hook::Set<BYTE>(0x401CA4, 0xEB);

		// Bypass "user already registered"  (EXE_TRANSMITERROR)
		Utils::Hook::Set<BYTE>(0x401C15, 0xEB);

		// Make sure preDestroy is called when the game shuts down
		Scheduler::OnShutdown(Loader::PreDestroy);
	}

	Game::XAssetEntry* Core::AddUniqueWeaponXAsset(Game::XAssetType type, Game::XAssetHeader* asset) 
	{

		// If weapon component is present, it will take care of this
		if (Loader::GetInstance<Weapon>() == nullptr &&
			asset != nullptr && 
			asset->weapon != nullptr &&
			asset->weapon->szInternalName != nullptr
			) 
		{
			const std::string name(asset->weapon->szInternalName);

			if (loadedWeapons.find(name) != loadedWeapons.end()) 
			{
				auto weapon = Game::DB_FindXAssetEntry(type, name.c_str());

				if (weapon == nullptr)
				{
					// Might have been unloaded in the meantime
					loadedWeapons.erase(name);
					return AddUniqueWeaponXAsset(type, asset);
				}
				else
				{
					return weapon;
				}
			}
			else
			{
				loadedWeapons.emplace(std::string(asset->weapon->szInternalName));
			}
		}

		// Default behaviour, fall through
		return Game::DB_AddXAsset(type, asset);
	}
}
