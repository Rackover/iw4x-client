#include <STDInclude.hpp>

namespace Components
{
	bool ConnectProtocol::Evaluated = false;
	std::string ConnectProtocol::ConnectString;

	bool ConnectProtocol::IsEvaluated()
	{
		return Evaluated;
	}

	bool ConnectProtocol::Used()
	{
		if (!IsEvaluated())
		{
			EvaluateProtocol();
		}

		return (!ConnectString.empty());
	}

	bool ConnectProtocol::InstallProtocol()
	{
		HKEY hKey = nullptr;
		std::string data;

		char ownPth[MAX_PATH]{};
		char workdir[MAX_PATH]{};

		DWORD dwsize = MAX_PATH;
		HMODULE hModule = GetModuleHandleA(nullptr);

		if (hModule != nullptr)
		{
			if (GetModuleFileNameA(hModule, ownPth, MAX_PATH) == ERROR)
			{
				return false;
			}

			if (GetModuleFileNameA(hModule, workdir, MAX_PATH) == ERROR)
			{
				return false;
			}
			else
			{
				auto* endPtr = std::strstr(workdir, "iw4x.exe");
				if (endPtr != nullptr)
				{
					*endPtr = 0;
				}
				else
				{
					return false;
				}
			}
		}
		else
		{
			return false;
		}

		SetCurrentDirectoryA(workdir);

		LONG openRes = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Classes\\iw4x\\shell\\open\\command", 0, KEY_ALL_ACCESS, &hKey);
		if (openRes == ERROR_SUCCESS)
		{
			char regred[MAX_PATH]{};

			// Check if the game has been moved.
			openRes = RegQueryValueExA(hKey, nullptr, nullptr, nullptr, reinterpret_cast<BYTE*>(regred), &dwsize);
			if (openRes == ERROR_SUCCESS)
			{
				auto* endPtr = std::strstr(regred, "\" \"%1\"");
				if (endPtr != nullptr)
				{
					*endPtr = 0;
				}
				else
				{
					return false;
				}

				RegCloseKey(hKey);

				if (std::strcmp(regred + 1, ownPth) != 0)
				{
					RegDeleteKeyA(HKEY_CURRENT_USER, "SOFTWARE\\Classes\\iw4x");
				}
				else
				{
					return true;
				}
			}
			else
			{
				RegDeleteKeyA(HKEY_CURRENT_USER, "SOFTWARE\\Classes\\iw4x");
			}
		}
		else
		{
			RegDeleteKeyA(HKEY_CURRENT_USER, "SOFTWARE\\Classes\\iw4x");
		}

		// Open SOFTWARE\\Classes
		openRes = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Classes", 0, KEY_ALL_ACCESS, &hKey);

		if (openRes != ERROR_SUCCESS)
		{
			return false;
		}

		// Create SOFTWARE\\Classes\\iw4x
		openRes = RegCreateKeyExA(hKey, "iw4x", 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hKey, nullptr);

		if (openRes != ERROR_SUCCESS)
		{
			return false;
		}

		// Write URL:IW4x Protocol
		data = "URL:IW4x Protocol";
		openRes = RegSetValueExA(hKey, "URL Protocol", 0, REG_SZ, reinterpret_cast<const BYTE*>(data.data()), data.size() + 1);

		if (openRes != ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			return false;
		}

		// Create SOFTWARE\\Classes\\iw4x\\DefaultIcon
		openRes = RegCreateKeyExA(hKey, "DefaultIcon", 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hKey, nullptr);

		if (openRes != ERROR_SUCCESS)
		{
			return false;
		}

		data = Utils::String::VA("%s,1", ownPth);
		openRes = RegSetValueExA(hKey, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(data.data()), data.size() + 1);
		RegCloseKey(hKey);

		if (openRes != ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			return false;
		}

		openRes = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Classes\\iw4x", 0, KEY_ALL_ACCESS, &hKey);

		if (openRes != ERROR_SUCCESS)
		{
			return false;
		}

		openRes = RegCreateKeyExA(hKey, "shell\\open\\command", 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hKey, nullptr);

		if (openRes != ERROR_SUCCESS)
		{
			return false;
		}

		data = Utils::String::VA("\"%s\" \"%s\"", ownPth, "%1");
		openRes = RegSetValueExA(hKey, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(data.data()), data.size() + 1);
		RegCloseKey(hKey);

		if (openRes != ERROR_SUCCESS)
		{
			return false;
		}

		return true;
	}

	void ConnectProtocol::EvaluateProtocol()
	{
		if (Evaluated) return;
		Evaluated = true;

		std::string cmdLine = GetCommandLineA();

		auto pos = cmdLine.find("iw4x://");

		if (pos != std::string::npos)
		{
			cmdLine = cmdLine.substr(pos + 7);
			pos = cmdLine.find_first_of('/');

			if (pos != std::string::npos)
			{
				cmdLine = cmdLine.substr(0, pos);
			}

			ConnectString = cmdLine;
		}
	}

	void ConnectProtocol::Invocation()
	{
		if (Used())
		{
			Command::Execute(std::format("connect {}", ConnectString), false);
		}
	}

	ConnectProtocol::ConnectProtocol()
	{
		if (Dedicated::IsEnabled()) return;

		// IPC handler
		IPCPipe::On("connect", [](const std::string& data)
		{
			Command::Execute(std::format("connect {}", data), false);
		});

		// Invocation handler
		Scheduler::OnGameInitialized(Invocation, Scheduler::Pipeline::MAIN);

		InstallProtocol();
		EvaluateProtocol();

		// Fire protocol handlers
		// Make sure this happens after the pipe-initialization!
		if (Used())
		{
			if (!Singleton::IsFirstInstance())
			{
				IPCPipe::Write("connect", ConnectString);
				ExitProcess(0);
			}
			else
			{
				// Only skip intro here, invocation will be done later.
				Utils::Hook::Set<std::uint8_t>(0x60BECF, 0xEB);

				Scheduler::Once([]
				{
					Command::Execute("openmenu popup_reconnectingtoparty", false);
				}, Scheduler::Pipeline::CLIENT, 8s);
			}
		}
	}
}
