#include <STDInclude.hpp>
#include "IMaterialVertexShader.hpp"

#define IW4X_TECHSET_VERSION 1

namespace Assets
{
	void IMaterialVertexShader::load(Game::XAssetHeader* header, const std::string& name, Components::ZoneBuilder::Zone* builder)
	{
		if (!header->data) this->loadBinary(header, name, builder); // Check if we need to import a new one into the game
		if (!header->data) this->loadNative(header, name, builder); // Check if there is a native one
	}

	void IMaterialVertexShader::loadNative(Game::XAssetHeader* header, const std::string& name, Components::ZoneBuilder::Zone* /*builder*/)
	{
		header->vertexShader = Components::AssetHandler::FindOriginalAsset(this->getType(), name.data()).vertexShader;
	}

	void IMaterialVertexShader::loadBinary(Game::XAssetHeader* header, const std::string& name, Components::ZoneBuilder::Zone* builder)
	{
		Components::FileSystem::File vsFile(std::format("vs/{}.iw4xVS", name));
		if (!vsFile.exists()) return;

		Utils::Stream::Reader reader(builder->getAllocator(), vsFile.getBuffer());

		char* magic = reader.readArray<char>(8);
		if (std::memcmp(magic, "IW4xVERT", 8))
		{
			Components::Logger::Error(Game::ERR_FATAL, "Reading vertex shader '{}' failed, header is invalid!", name);
		}

		auto version = reader.read<char>();
		if (version != IW4X_TECHSET_VERSION)
		{
			Components::Logger::Error(Game::ERR_FATAL, "Reading vertex shader '{}' failed, expected version is {}, but it was {}!",
				name, IW4X_TECHSET_VERSION, static_cast<int>(version));
		}

		Game::MaterialVertexShader* asset = reader.readObject<Game::MaterialVertexShader>();

		if (asset->name)
		{
			asset->name = reader.readCString();
		}

		if (asset->prog.loadDef.program)
		{
			asset->prog.loadDef.program = reader.readArray<unsigned int>(asset->prog.loadDef.programSize);
		}

		header->vertexShader = asset;
	}

	void IMaterialVertexShader::save(Game::XAssetHeader header, Components::ZoneBuilder::Zone* builder)
	{
		AssertSize(Game::MaterialVertexShader, 16);

		Utils::Stream* buffer = builder->getBuffer();
		Game::MaterialVertexShader* asset = header.vertexShader;
		Game::MaterialVertexShader* dest = buffer->dest<Game::MaterialVertexShader>();
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
