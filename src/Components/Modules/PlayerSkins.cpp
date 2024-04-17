#include <STDInclude.hpp>

#include "rapidjson/document.h"

#include "GSC/Script.hpp"

#include "PlayerSkins.hpp"

namespace Components
{
	const std::string PlayerSkins::heads[] = {
		""
	};

	const std::string PlayerSkins::bodies[] = {
		""
	};

	Dvar::Var PlayerSkins::localHeadIndexDvar;
	Dvar::Var PlayerSkins::localBodyIndexDvar;
	Dvar::Var PlayerSkins::localEnableHeadDvar;
	Dvar::Var PlayerSkins::localEnableBodyDvar;

	Skin PlayerSkins::currentSkin;

	// We hook cdecl so no need to write assembly code
	uint32_t PlayerSkins::GetTrueSkillForGametype([[maybe_unused]] int localClientIndex, [[maybe_unused]] char* gametype)
	{
		// localClientIndex will always be zero
		// gametype will always be TDM

		// We can return anything here and it will be replicated to the party! Woohoo!

		RefreshPlayerSkinFromDvars();

		return currentSkin.intValue;
	}

	void PlayerSkins::RefreshPlayerSkinFromDvars()
	{
		currentSkin.enableBody = localEnableBodyDvar.get<bool>() ? 1 : 0;
		currentSkin.enableHead = localEnableHeadDvar.get<bool>() ? 1 : 0;
		currentSkin.bodyIndex = localBodyIndexDvar.get<int>();
		currentSkin.headIndex = localHeadIndexDvar.get<int>();

		SanitizeSkin(currentSkin);
	}

	void PlayerSkins::SanitizeSkin(Skin& skin)
	{
		// Clamp to the realm of possible
		skin.headIndex = std::clamp(skin.headIndex, (unsigned)0, ARRAYSIZE(heads) - 1);
		skin.bodyIndex = std::clamp(skin.bodyIndex, (unsigned)0, ARRAYSIZE(bodies) - 1);
	}

	PlayerSkins::PlayerSkins()
	{
		currentSkin = {};
		currentSkin.enableBody = 1;
		currentSkin.enableHead = 1;

		Utils::Hook(0x4F33D0, GetTrueSkillForGametype, HOOK_JUMP).install()->quick();

		Scheduler::Once([]() {

			localHeadIndexDvar = Dvar::Register(
				"skin_head",
				0,
				0,
				static_cast<int>(ARRAYSIZE(heads) - 1),
				static_cast<uint16_t>(Game::DVAR_SAVED),
				"custom head index!"
			);

			localBodyIndexDvar = Dvar::Register(
				"skin_body",
				0,
				0,
				static_cast<int>(ARRAYSIZE(bodies) - 1),
				static_cast<uint16_t>(Game::DVAR_SAVED),
				"custom body index!"
			);

			localEnableHeadDvar = Dvar::Register("skin_enable_head", true, static_cast<uint16_t>(Game::DVAR_SAVED), "toggle on/off your custom head");
			localEnableBodyDvar = Dvar::Register("skin_enable_body", true, static_cast<uint16_t>(Game::DVAR_SAVED), "toggle on/off your custom body");

			RefreshPlayerSkinFromDvars();

			Components::GSC::Script::AddFunction("LOUV_GetPlayerSkin", []() {
				if (Game::Scr_GetNumParam() != 1)
				{
					Game::Scr_Error("LOUV_GetPlayerSkin: No player index given");
					return;
				}

				PlayerSkins::GScr_GetPlayerSkin(Game::Scr_GetInt(0));
				});

			}, Components::Scheduler::Pipeline::MAIN
		);
	}

	void PlayerSkins::GScr_GetPlayerSkin(int partyIndex)
	{
		if (Game::g_partyData)
		{
			if (partyIndex >= 0 && partyIndex < Game::MAX_CLIENTS)
			{

				const auto member = &Game::g_partyData->partyMembers[partyIndex];

#define PARTYSTATUS_PRESENT 3

				if (member->status == PARTYSTATUS_PRESENT)
				{
					Skin skin = { member->trueSkill };

					SanitizeSkin(skin);

					Game::Scr_MakeArray();

					Game::Scr_AddString((skin.enableHead ? heads[skin.headIndex] : heads[0]).data());
					Game::Scr_AddArray();

					Game::Scr_AddString((skin.enableBody ? bodies[skin.bodyIndex] : bodies[0]).data());
					Game::Scr_AddArray();

					// Returned the array with head first and body last
				}
				else
				{
					const auto err = std::format("This player (index {}) does not exist in the party!", partyIndex);
					Game::Scr_Error(err.data());
				}
			}
			else
			{
				const auto err = std::format("Invalid player index {} (out of bounds)", partyIndex);
				Game::Scr_Error(err.data());
			}
		}
		else
		{
			const auto err = std::format("Not in a party!!");
			Game::Scr_Error(err.data());
		}
	}
}
