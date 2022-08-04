#include <STDInclude.hpp>
#include "IMaterialPixelShader.hpp"

#define IW4X_TECHSET_VERSION 1

namespace Assets
{

	void IMaterialPixelShader::load(Game::XAssetHeader* header, const std::string& name, Components::ZoneBuilder::Zone* builder)
	{
		if (!header->data) this->loadBinary(header, name, builder); // Check if we need to import a new one into the game
		if (!header->data) this->loadNative(header, name, builder); // Check if there is a native one
	}

	void IMaterialPixelShader::loadNative(Game::XAssetHeader* header, const std::string& name, Components::ZoneBuilder::Zone* /*builder*/)
	{
		header->pixelShader = Components::AssetHandler::FindOriginalAsset(this->getType(), name.data()).pixelShader;
	}

	void IMaterialPixelShader::loadBinary(Game::XAssetHeader* header, const std::string& name, Components::ZoneBuilder::Zone* builder)
	{
		Components::FileSystem::File psFile(Utils::String::VA("ps/%s.iw4xPS", name.data()));
		if (!psFile.exists()) return;

		Utils::Stream::Reader reader(builder->getAllocator(), psFile.getBuffer());

		char* magic = reader.readArray<char>(8);
		if (std::memcmp(magic, "IW4xPIXL", 8))
		{
			Components::Logger::Error(Game::ERR_FATAL, "Reading pixel shader '{}' failed, header is invalid!", name);
		}

		auto version = reader.read<char>();
		if (version != IW4X_TECHSET_VERSION)
		{
			Components::Logger::Error(Game::ERR_FATAL,
				"Reading pixel shader '{}' failed, expected version is {}, but it was {}!", name, IW4X_TECHSET_VERSION, static_cast<int>(version));
		}

		Game::MaterialPixelShader* asset = reader.readObject<Game::MaterialPixelShader>();

		if (asset->name)
		{
			asset->name = reader.readCString();
		}

		if (asset->prog.loadDef.program)
		{
			asset->prog.loadDef.program = reader.readArray<unsigned int>(asset->prog.loadDef.programSize);
		}

		header->pixelShader = asset;
	}

	void IMaterialPixelShader::save(Game::XAssetHeader header, Components::ZoneBuilder::Zone* builder)
	{
		AssertSize(Game::MaterialPixelShader, 16);

		Utils::Stream* buffer = builder->getBuffer();
		Game::MaterialPixelShader* asset = header.pixelShader;
		Game::MaterialPixelShader* dest = buffer->dest<Game::MaterialPixelShader>();
		buffer->save(asset);

		buffer->pushBlock(Game::XFILE_BLOCK_VIRTUAL);

		if (asset->name)
		{
			buffer->saveString(builder->getAssetName(this->getType(), asset->name));
			Utils::Stream::ClearPointer(&dest->name);
		}

		if (asset->prog.loadDef.program)
		{
			buffer->align(Utils::Stream::ALIGN_4);
			buffer->saveArray(asset->prog.loadDef.program, asset->prog.loadDef.programSize);
			Utils::Stream::ClearPointer(&dest->prog.loadDef.program);
		}

		buffer->popBlock();
	}
}
