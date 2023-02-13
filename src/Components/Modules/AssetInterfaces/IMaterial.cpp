#include <STDInclude.hpp>
#include "IMaterial.hpp"

#include <Utils/Json.hpp>

#define IW4X_MAT_BIN_VERSION "1"
#define IW4X_MAT_JSON_VERSION 1

namespace Assets
{
	const std::unordered_map<std::string, std::string> techSetCorrespondance =
	{
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
		{"mc_effect_falloff_add_nofog", "mc_effect_falloff_add_nofog_eyeoffset"},
	};

	void IMaterial::load(Game::XAssetHeader* header, const std::string& name, Components::ZoneBuilder::Zone* builder)
	{
		if (!header->data) this->loadFromDisk(header, name, builder);   // Check if we want to load a material from disk
		if (!header->data) this->loadNative(header, name, builder); // Check if there is a native one

		assert(header->data);
	}


	void IMaterial::loadFromDisk(Game::XAssetHeader* header, const std::string& name, [[maybe_unused]] Components::ZoneBuilder::Zone* builder)
	{
		header->material = builder->getIW4OfApi()->read<Game::Material>(Game::XAssetType::ASSET_TYPE_MATERIAL, name);
	}


	void IMaterial::loadNative(Game::XAssetHeader* header, const std::string& name, Components::ZoneBuilder::Zone* /*builder*/)
	{
		header->material = Components::AssetHandler::FindOriginalAsset(this->getType(), name.data()).material;
	}

	void IMaterial::mark(Game::XAssetHeader header, Components::ZoneBuilder::Zone* builder)
	{
		Game::Material* asset = header.material;

		if (asset->techniqueSet)
		{
			builder->loadAsset(Game::XAssetType::ASSET_TYPE_TECHNIQUE_SET, asset->techniqueSet);
		}

		if (asset->textureTable)
		{
			for (char i = 0; i < asset->textureCount; ++i)
			{
				if (asset->textureTable[i].u.image)
				{
					if (asset->textureTable[i].semantic == Game::TextureSemantic::TS_WATER_MAP)
					{
						if (asset->textureTable[i].u.water->image)
						{
							builder->loadAsset(Game::XAssetType::ASSET_TYPE_IMAGE, asset->textureTable[i].u.water->image);
						}
					}
					else
					{
						builder->loadAsset(Game::XAssetType::ASSET_TYPE_IMAGE, asset->textureTable[i].u.image);
					}
				}
			}
		}
	}

	void IMaterial::save(Game::XAssetHeader header, Components::ZoneBuilder::Zone* builder)
	{
		AssertSize(Game::Material, 96);

		Utils::Stream* buffer = builder->getBuffer();
		Game::Material* asset = header.material;
		Game::Material* dest = buffer->dest<Game::Material>();
		buffer->save(asset);

		buffer->pushBlock(Game::XFILE_BLOCK_VIRTUAL);

		if (asset->info.name)
		{
			buffer->saveString(builder->getAssetName(this->getType(), asset->info.name));
			Utils::Stream::ClearPointer(&dest->info.name);
		}

		if (asset->techniqueSet)
		{
			dest->techniqueSet = builder->saveSubAsset(Game::XAssetType::ASSET_TYPE_TECHNIQUE_SET, asset->techniqueSet).techniqueSet;
		}

		if (asset->textureTable)
		{
			AssertSize(Game::MaterialTextureDef, 12);

			// Pointer/Offset insertion is untested, but it worked in T6, so I think it's fine
			if (builder->hasPointer(asset->textureTable))
			{
				dest->textureTable = builder->getPointer(asset->textureTable);
			}
			else
			{
				buffer->align(Utils::Stream::ALIGN_4);
				builder->storePointer(asset->textureTable);

				auto* destTextureTable = buffer->dest<Game::MaterialTextureDef>();
				buffer->saveArray(asset->textureTable, asset->textureCount);

				for (std::uint8_t i = 0; i < asset->textureCount; ++i)
				{
					auto* destTextureDef = &destTextureTable[i];
					auto* textureDef = &asset->textureTable[i];

					if (textureDef->semantic == Game::TextureSemantic::TS_WATER_MAP)
					{
						AssertSize(Game::water_t, 68);

						Game::water_t* destWater = buffer->dest<Game::water_t>();
						Game::water_t* water = textureDef->u.water;

						if (water)
						{
							buffer->align(Utils::Stream::ALIGN_4);
							buffer->save(water);
							Utils::Stream::ClearPointer(&destTextureDef->u.water);

							// Save_water_t
							if (water->H0)
							{
								buffer->align(Utils::Stream::ALIGN_4);
								buffer->save(water->H0, 8, water->M * water->N);
								Utils::Stream::ClearPointer(&destWater->H0);
							}

							if (water->wTerm)
							{
								buffer->align(Utils::Stream::ALIGN_4);
								buffer->save(water->wTerm, 4, water->M * water->N);
								Utils::Stream::ClearPointer(&destWater->wTerm);
							}

							if (water->image)
							{
								destWater->image = builder->saveSubAsset(Game::XAssetType::ASSET_TYPE_IMAGE, water->image).image;
							}
						}
					}
					else if (textureDef->u.image)
					{
						destTextureDef->u.image = builder->saveSubAsset(Game::XAssetType::ASSET_TYPE_IMAGE, textureDef->u.image).image;
					}
				}

				Utils::Stream::ClearPointer(&dest->textureTable);
			}
		}

		if (asset->constantTable)
		{
			AssertSize(Game::MaterialConstantDef, 32);

			if (builder->hasPointer(asset->constantTable))
			{
				dest->constantTable = builder->getPointer(asset->constantTable);
			}
			else
			{
				buffer->align(Utils::Stream::ALIGN_16);
				builder->storePointer(asset->constantTable);

				buffer->saveArray(asset->constantTable, asset->constantCount);
				Utils::Stream::ClearPointer(&dest->constantTable);
			}
		}

		if (asset->stateBitsTable)
		{
			if (builder->hasPointer(asset->stateBitsTable))
			{
				dest->stateBitsTable = builder->getPointer(asset->stateBitsTable);
			}
			else
			{
				buffer->align(Utils::Stream::ALIGN_4);
				builder->storePointer(asset->stateBitsTable);

				buffer->save(asset->stateBitsTable, 8, asset->stateBitsCount);
				Utils::Stream::ClearPointer(&dest->stateBitsTable);
			}
		}

		buffer->popBlock();
	}
}
