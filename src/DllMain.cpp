#include "STDInclude.hpp"

namespace Main
{
	void Initialize()
	{
		Utils::SetEnvironment();
		Utils::Cryptography::Initialize();
		Components::Loader::Initialize();

#if defined(DEBUG) || defined(FORCE_UNIT_TESTS)
		if (Components::Loader::IsPerformingUnitTests())
		{
			auto result = (Components::Loader::PerformUnitTests() ? 0 : -1);
			Components::Loader::Uninitialize();
			ExitProcess(result);
		}
#else
		if (Components::Flags::HasFlag("tests"))
		{
			Components::Logger::Print("Unit tests are disabled outside the debug environment!\n");
		}
#endif
	}

	void Uninitialize()
	{
		Components::Loader::Uninitialize();
		google::protobuf::ShutdownProtobufLibrary();
	}

	__declspec(naked) void EntryPoint()
	{
		__asm
		{
			pushad
			call Main::Initialize
			popad

			push 6BAA2Fh // Continue init routine
			push 6CA062h // __security_init_cookie
			retn
		}
	}
}

BOOL APIENTRY DllMain(HINSTANCE /*hinstDLL*/, DWORD fdwReason, LPVOID /*lpvReserved*/)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		SetProcessDEPPolicy(PROCESS_DEP_ENABLE);
		Steam::Proxy::RunMod();

#ifndef DISABLE_BINARY_CHECK
		// Ensure we're working with our desired binary
		// Rack: As a matter of fact, we will not be.
		auto* _module = reinterpret_cast<char*>(0x400000);
////		auto hash1 = Utils::Cryptography::JenkinsOneAtATime::Compute(_module + 0x1000, 0x2D531F);  // .text
////		auto hash2 = Utils::Cryptography::JenkinsOneAtATime::Compute(_module + 0x2D75FC, 0xBDA04); // .rdata
////		if ((hash1 != 0x54684DBE
////#ifdef DEBUG
////			&& hash1 != 0x8AADE716
////#endif
////			) || hash2 != 0x8030ec53)
////		{
////			return FALSE;
////		}

		DWORD oldProtect;
		VirtualProtect(_module + 0x1000, 0x2D6000, PAGE_EXECUTE_READ, &oldProtect); // Protect the .text segment
#endif

		// Install entry point hook
		Utils::Hook(0x6BAC0F, Main::EntryPoint, HOOK_JUMP).install()->quick();
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		Main::Uninitialize();
	}

	return TRUE;
}
