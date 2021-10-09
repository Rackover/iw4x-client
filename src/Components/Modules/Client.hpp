#pragma once

namespace Components
{
	class Client : public Component
	{
	public:
		Client();
		~Client();

		static bool CoDo2Iw4_Enabled();
		static std::string CoDo_Current();
		static Game::GfxImage* Get_Image(const char* name);
		static void Clear_Image(const char* name);
		static void Clear_Images();
		static bool Generating_Csv();

	private:
		
		static bool codo2iw4;
		static std::string codo_map;
		static std::unordered_map<std::string, Game::GfxImage*> stored_images;
		static bool generating_csv;
		static FILE* csv_fp;
		static std::string csv_buffer;

		static void Load_Texture_Stub(Game::GfxImageLoadDef** loadDef, Game::GfxImage* image);
		static void Generate_Csv(const std::string& ff);

		static void AddFunctions();
		static void AddMethods();
		static void AddCommands();
	};
}
