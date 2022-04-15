#include <STDInclude.hpp>

namespace Components
{
	int Core::weaponsLoaded = 0;
	Game::XAssetEntry* Core::lastWeaponEntry = nullptr;
	std::unordered_set<std::string> Core::skippedWeapons{};

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

		// Asset limits - instead of erroring when going above, just ignore further loads
		Utils::Hook(0x4A473C, Core::AddXAssetIfWeaponUnderLimit, HOOK_CALL).install()->quick(); 

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

		Utils::Hook(0x5BADFF, Sys_TempPriorityEnd, HOOK_CALL).install()->quick();

		// Necessary hook when Weapons component is not loaded, to avoid loading weapons over the limit and crashing
		Utils::Hook(0x4D4A85, G_GetWeaponIndexForName, HOOK_CALL).install()->quick();

		// Make sure preDestroy is called when the game shuts down
		Scheduler::OnShutdown(Loader::PreDestroy);
	}

	Game::XAssetEntry* __cdecl Core::AddXAssetIfWeaponUnderLimit(Game::XAssetType type, Game::XAssetHeader* a2) {
		const unsigned long DEFAULT_WEAPON_LIMIT = 1200 /* *reinterpret_cast<unsigned long*>(0x403783) */;

		if (Loader::GetInstance<Weapon>() == nullptr && weaponsLoaded >= DEFAULT_WEAPON_LIMIT)
		{
			Components::Logger::Print("Skipped loading of weapon %s (hit limit of %d weapons)\n", a2->weapon->szInternalName, DEFAULT_WEAPON_LIMIT);
			skippedWeapons.emplace(std::string(a2->weapon->szInternalName));
			return lastWeaponEntry;
		}
		else 
		{
			weaponsLoaded++;

			Components::Logger::Print("Loaded weapon %s (%d/%d)\n", a2->weapon->szInternalName, weaponsLoaded, DEFAULT_WEAPON_LIMIT);

			lastWeaponEntry = Game::DB_AddXAsset(type, a2);
			return lastWeaponEntry;
		}
	}

	BOOL __cdecl Sys_TempPriorityEnd(int a1) {
		return Utils::Hook::Call<BOOL(int)>(0x4DCF00)(a1);
	}

	unsigned int __cdecl Core::G_GetWeaponIndexForName(const char* a1)
	{
		if (Loader::GetInstance<Weapon>() == nullptr)
		{
			if (skippedWeapons.find(std::string(a1)) != skippedWeapons.end()) {
				Components::Logger::Print("Bypassing index for weapon: %s\n", a1);
				return 0;
			}
		}

		return Game::G_GetWeaponIndexForName(a1);
	}

}
