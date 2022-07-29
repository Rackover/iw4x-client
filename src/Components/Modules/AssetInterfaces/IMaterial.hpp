#pragma once

namespace Assets
{
	class IMaterial : public Components::AssetHandler::IAsset
	{
	public:
		Game::XAssetType getType() override { return Game::XAssetType::ASSET_TYPE_MATERIAL; }

		void save(Game::XAssetHeader header, Components::ZoneBuilder::Zone* builder) override;
		void mark(Game::XAssetHeader header, Components::ZoneBuilder::Zone* builder) override;
		void load(Game::XAssetHeader* header, const std::string& name, Components::ZoneBuilder::Zone* builder) override;
		void loadJson(Game::XAssetHeader* header, const std::string& name, Components::ZoneBuilder::Zone* builder);
		void loadNative(Game::XAssetHeader* header, const std::string& name, Components::ZoneBuilder::Zone* builder);
		void loadBinary(Game::XAssetHeader* header, const std::string& name, Components::ZoneBuilder::Zone* builder);

	private:
		const std::unordered_map<std::string, std::string> techSetCorrespondance = {
			{"effect", "effect_blend"},
			{"effect", "effect_blend"},
			{"effect_nofog", "effect_blend_nofog"},
			{"effect_zfeather", "effect_zfeather_blend"},
			{"effect_zfeather_falloff", "effect_zfeather_falloff_add"},
			{"effect_zfeather_nofog", "effect_zfeather_add_nofog"},

			{"wc_unlit_add", "wc_unlit_add_lin"},
			{"wc_unlit_distfalloff", "wc_unlit_distfalloff_replace"},
			{"wc_unlit_multiply", "wc_unlit_multiply_lin"},
			{"wc_unlit_falloff_add", "wc_unlit_falloff_add_lin"},
			{"wc_unlit", "wc_unlit_replace_lin"},
			{"wc_unlit_alphatest", "wc_unlit_blend_lin"},
			{"wc_unlit_blend", "wc_unlit_blend_lin"},
			{"wc_unlit_replace", "wc_unlit_replace_lin"},
			{"wc_unlit_nofog", "wc_unlit_replace_lin_nofog_nocast" },

			{"mc_unlit_replace", "mc_unlit_replace_lin"},
			{"mc_unlit_nofog", "mc_unlit_blend_nofog_ua"},
			{"mc_unlit", "mc_unlit_replace_lin_nocast"},
			{"mc_unlit_alphatest", "mc_unlit_blend_lin"},
			{"mc_effect_nofog", "mc_effect_blend_nofog"},
			{"mc_effect_falloff_add_nofog", "mc_effect_falloff_add_nofog_eyeoffset"}
		};

		int readFlags(const std::string binaryFlags) const;
		Game::MaterialTechniqueSet* findWorkingTechset(const std::string techsetName, Game::Material* material, Components::ZoneBuilder::Zone* builder) const;
	};
}
