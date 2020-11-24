#pragma once

namespace Assets
{
	class IXModel : public Components::AssetHandler::IAsset
	{
	public:
		virtual Game::XAssetType getType() override { return Game::XAssetType::ASSET_TYPE_XMODEL; };

		virtual void save(Game::XAssetHeader header, Components::ZoneBuilder::Zone* builder) override;
		virtual void mark(Game::XAssetHeader header, Components::ZoneBuilder::Zone* builder) override;
		virtual void load(Game::XAssetHeader* header, const std::string& name, Components::ZoneBuilder::Zone* builder) override;

	private:
        std::map<void*, void*> triIndicies;
		void loadXModelSurfs(Game::XModelSurfs* asset, Utils::Stream::Reader* reader, Components::ZoneBuilder::Zone* builder);
		void loadXSurface(Game::XSurface* surf, Utils::Stream::Reader* reader, Components::ZoneBuilder::Zone* builder);
		void loadXSurfaceCollisionTree(Game::XSurfaceCollisionTree* entry, Utils::Stream::Reader* reader);
		void loadIW4XModel(Game::XAssetHeader* header, Components::FileSystem::File modelFile, Components::ZoneBuilder::Zone* builder);
		void loadZTModel(Game::XAssetHeader* header, Components::FileSystem::File modelFile, Components::ZoneBuilder::Zone* builder);
	};
}
