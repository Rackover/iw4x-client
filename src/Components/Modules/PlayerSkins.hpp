#pragma once

namespace Components
{
	union Skin
	{
		struct
		{
			unsigned int headIndex : 8;
			unsigned int bodyIndex : 8;
			unsigned int enableHead : 1;
			unsigned int enableBody : 1;
			unsigned int reserved : 6; // Unused for now
		};

		int intValue;
	};

	class PlayerSkins : public Component
	{

	public:
		PlayerSkins();

		static Skin GetSkin() { return currentSkin; };

		static void GScr_GetPlayerSkin(Game::gentity_s* entRef);

	private:
		static const std::string heads[];
		static const std::string bodies[];

		static Dvar::Var localHeadIndexDvar;
		static Dvar::Var localBodyIndexDvar;
		static Dvar::Var localEnableHeadDvar;
		static Dvar::Var localEnableBodyDvar;
		static Dvar::Var skinTryOut;
		static Dvar::Var sv_allowSkins;

		static uint32_t GetTrueSkillForGametype(int localClientIndex, char* gametype);
		static void RefreshPlayerSkinFromDvars();
		static void SanitizeSkin(Skin& skin);

		static void Info_SetValueForKey(const char* infoString, const char* key, const char* data);

		static Skin currentSkin;
	};
}
