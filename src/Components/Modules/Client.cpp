#include "STDInclude.hpp"

namespace Components
{
	void Client::AddFunctions()
	{
		//File functions

		Script::AddFunction("fileWrite", [](Game::scr_entref_t) // gsc: fileWrite(<filepath>, <string>, <mode>)
		{
			std::string path = Game::Scr_GetString(0);
			auto text = Game::Scr_GetString(1);
			auto mode = Game::Scr_GetString(2);

			if (path.empty())
			{
				Game::Com_Printf(0, "^1fileWrite: filepath not defined!\n");
				return;
			}

			std::vector<const char*> queryStrings = { R"(..)", R"(../)", R"(..\)" };
			for (auto i = 0u; i < queryStrings.size(); i++)
			{
				if (path.find(queryStrings[i]) != std::string::npos)
				{
					Game::Com_Printf(0, "^1fileWrite: directory traversal is not allowed!\n");
					return;
				}
			}

			if (mode != "append"s && mode != "write"s)
			{
				Game::Com_Printf(0, "^3fileWrite: mode not defined or was wrong, defaulting to 'write'\n");
				mode = const_cast<char*>("write");
			}

			if (mode == "write"s)
			{
				FileSystem::FileWriter(path).write(text);
			}
			else if (mode == "append"s)
			{
				FileSystem::FileWriter(path, true).write(text);
			}
		});

		Script::AddFunction("fileRead", [](Game::scr_entref_t) // gsc: fileRead(<filepath>)
		{
			std::string path = Game::Scr_GetString(0);

			if (path.empty())
			{
				Game::Com_Printf(0, "^1fileRead: filepath not defined!\n");
				return;
			}

			std::vector<const char*> queryStrings = { R"(..)", R"(../)", R"(..\)" };
			for (auto i = 0u; i < queryStrings.size(); i++)
			{
				if (path.find(queryStrings[i]) != std::string::npos)
				{
					Game::Com_Printf(0, "^1fileRead: directory traversal is not allowed!\n");
					return;
				}
			}

			if (!FileSystem::FileReader(path).exists())
			{
				Game::Com_Printf(0, "^1fileRead: file not found!\n");
				return;
			}

			Game::Scr_AddString(FileSystem::FileReader(path).getBuffer().data());
		});

		Script::AddFunction("fileExists", [](Game::scr_entref_t) // gsc: fileExists(<filepath>)
		{
			std::string path = Game::Scr_GetString(0);

			if (path.empty())
			{
				Game::Com_Printf(0, "^1fileExists: filepath not defined!\n");
				return;
			}

			std::vector<const char*> queryStrings = { R"(..)", R"(../)", R"(..\)" };
			for (auto i = 0u; i < queryStrings.size(); i++)
			{
				if (path.find(queryStrings[i]) != std::string::npos)
				{
					Game::Com_Printf(0, "^1fileExists: directory traversal is not allowed!\n");
					return;
				}
			}

			Game::Scr_AddInt(FileSystem::FileReader(path).exists());
		});

		Script::AddFunction("fileRemove", [](Game::scr_entref_t) // gsc: fileRemove(<filepath>)
		{
			std::string path = Game::Scr_GetString(0);

			if (path.empty())
			{
				Game::Com_Printf(0, "^1fileRemove: filepath not defined!\n");
				return;
			}

			std::vector<const char*> queryStrings = { R"(..)", R"(../)", R"(..\)" };
			for (auto i = 0u; i < queryStrings.size(); i++)
			{
				if (path.find(queryStrings[i]) != std::string::npos)
				{
					Game::Com_Printf(0, "^1fileRemove: directory traversal is not allowed!\n");
					return;
				}
			}

			auto p = std::filesystem::path(path);
			std::string folder = p.parent_path().string();
			std::string file = p.filename().string();
			Game::Scr_AddInt(FileSystem::DeleteFile(folder, file));
		});
	}

	void Client::AddMethods()
	{
		// Client methods

		Script::AddFunction("getIp", [](Game::scr_entref_t id) // gsc: self getIp()
		{
			Game::gentity_t* gentity = Script::getEntFromEntRef(id);
			Game::client_t* client = Script::getClientFromEnt(gentity);

			if (client->state >= 3)
			{
				std::string ip = Game::NET_AdrToString(client->netchan.remoteAddress);
				if (ip.find_first_of(":") != std::string::npos)
					ip.erase(ip.begin() + ip.find_first_of(":"), ip.end()); // erase port
				Game::Scr_AddString(ip.data());
			}
		});

		Script::AddFunction("getPing", [](Game::scr_entref_t id) // gsc: self getPing()
		{
			Game::gentity_t* gentity = Script::getEntFromEntRef(id);
			Game::client_t* client = Script::getClientFromEnt(gentity);

			if (client->state >= 3)
			{
				int ping = (int)client->ping;
				Game::Scr_AddInt(ping);
			}
		});
	}

	void Client::AddCommands()
	{
		Command::Add("NULL", [](Command::Params*)
		{
			return NULL;
		});
	}

	bool Client::codo2iw4 = false;
	std::string Client::codo_map = "none";
	std::unordered_map<std::string, Game::GfxImage*> Client::stored_images;
	bool Client::generating_csv = false;
	FILE* Client::csv_fp;
	std::string Client::csv_buffer;

	bool Client::CoDo2Iw4_Enabled()
	{
		return Client::codo2iw4;
	}

	std::string Client::CoDo_Current()
	{
		return Client::codo_map;
	}
	
	void Client::Load_Texture_Stub(Game::GfxImageLoadDef** loadDef, Game::GfxImage* image)
	{
		if (Client::codo2iw4 || ZoneBuilder::IsEnabled())
		{
			auto duplicate_image = Utils::Memory::Duplicate(image);
			duplicate_image->name = Utils::Memory::DuplicateString(image->name);
			duplicate_image->texture.loadDef = Utils::Memory::Duplicate(image->texture.loadDef);
			duplicate_image->texture.loadDef2->texture = Utils::Memory::AllocateArray<char>(image->texture.loadDef->resourceSize);
			memcpy(duplicate_image->texture.loadDef2->texture, image->texture.loadDef->data, image->texture.loadDef->resourceSize);

			Client::Clear_Image(image->name);
			Client::stored_images[image->name] = duplicate_image;
		}

		Game::Load_Texture(loadDef, image);
	}

	Game::GfxImage* Client::Get_Image(const char* name)
	{
		if (Client::stored_images.find(name) != Client::stored_images.end())
		{
			return Client::stored_images[name];
		}
		return nullptr;
	}

	void Client::Clear_Image(const char* name)
	{
		if (Client::stored_images.find(name) == Client::stored_images.end())
		{
			return;
		}
		auto image = Client::stored_images[name];
		Utils::Memory::Free(image->texture.loadDef2->texture);
		Utils::Memory::Free(image->texture.loadDef);
		Utils::Memory::Free(image->name);
		Utils::Memory::Free(image);
	}

	void Client::Clear_Images()
	{
		for (auto& it : Client::stored_images) {
			Client::Clear_Image(it.first.data());
		}
		stored_images.clear();
	}

	void Client::Generate_Csv(const std::string& name)
	{
		Client::csv_fp = nullptr;
		Client::csv_buffer.clear();
		Client::codo_map = name;

		Client::generating_csv = true;
		Command::Execute(Utils::String::VA("map %s", name.data()), true);
		Client::generating_csv = false;
	}

	bool Client::Generating_Csv()
	{
		return Client::generating_csv;
	}

	Client::Client()
	{
		Client::AddFunctions();
		Client::AddMethods();
		Client::AddCommands();

		Utils::Hook(0x4D32BC, Client::Load_Texture_Stub, HOOK_CALL).install()->quick();
		
		AssetHandler::OnLoad([](Game::XAssetType type, Game::XAssetHeader /*asset*/, const std::string& name, bool* /*restrict*/)
		{
			if (Client::Generating_Csv())
			{
				if (FastFiles::Current() == Client::CoDo_Current())
				{
					if (!csv_fp)
					{
						std::string csv_path = "zone_source\\" + Client::CoDo_Current() + ".csv";
						fopen_s(&csv_fp, csv_path.data(), "wb");
					}

					std::string _name(name);
					if (_name[0] == ',') _name.erase(_name.begin());
					
					std::string _type = Game::DB_GetXAssetTypeName(type);
					if (_type == "game_map_sp") _type = "game_map_mp";

					if (!AssetHandler::Exists(type))
					{
						Logger::Print("^1IW4x doesn't have an asset interface for '%s' asset of type '%s'!\n Removing from the csv...^7\n", _name.data(), _type.data());
						return;
					}

					std::string str(_type + ","s + _name + "\n");
					csv_buffer.append(str);
				}
				else if (csv_fp)
				{
					fwrite(csv_buffer.data(), csv_buffer.size(), 1, csv_fp);
					fclose(csv_fp);
					generating_csv = false;
				}
			}
		});

		Command::Add("CoDoMap2Iw4", [](Command::Params* params) {
			if (params->length() > 2)
			{
				Logger::Print("usage: CoDoMap2Iw4 <map>\n");
				return;
			}
			Client::Clear_Images();
			Client::codo2iw4 = true;
			Client::codo_map = params->get(1);
			ZoneBuilder::Zone zone(params->get(1));
			Command::Execute(Utils::String::VA("map %s", params->get(1)), true);
			zone.build();
			Client::codo2iw4 = false;
			Client::codo_map = "none";
			Client::Clear_Images();
		});

		Command::Add("CoDoMap2Iw4_GenerateCsv", [](Command::Params* params) {
			if (params->length() > 2)
			{
				Logger::Print("usage: CoDoMap2Iw4_GenerateCsv <map>\n");
				return;
			}

			Client::Generate_Csv(params->get(1));
		});

		Command::Add("CoDoMap2Iw4_Auto", [](Command::Params* params) {
			if (params->length() > 2)
			{
				Logger::Print("usage: CoDoMap2Iw4_Auto <map>\n");
				return;
			}

			Client::Generate_Csv(params->get(1));
			Command::Execute(Utils::String::VA("disconnect; wait 5; CoDoMap2Iw4 %s", params->get(1)), false);
		});
	}

	Client::~Client()
	{

	}
}