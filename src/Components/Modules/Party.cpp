#include <STDInclude.hpp>
#include <Utils/InfoString.hpp>

#include "Auth.hpp"
#include "Download.hpp"
#include "Friends.hpp"
#include "Gamepad.hpp"
#include "ModList.hpp"
#include "Node.hpp"
#include "Party.hpp"
#include "ServerList.hpp"
#include "Stats.hpp"
#include "TextRenderer.hpp"
#include "Voice.hpp"
#include "Events.hpp"

#include <version.hpp>

#define CL_MOD_LOADING

namespace Components
{
	class JoinContainer
	{
	public:
		Network::Address target;
		std::string challenge;
		std::string motd;
		DWORD joinTime;
		bool valid;
		int matchType;

		Utils::InfoString info;

		// Party-specific stuff
		DWORD requestTime;
		bool awaitingPlaylist;
	};

	static JoinContainer Container;
	std::map<std::uint64_t, Network::Address> Party::LobbyMap;

	Dvar::Var Party::PartyEnable;

	SteamID Party::GenerateLobbyId()
	{
		SteamID id;

		id.accountID = Game::Sys_Milliseconds();
		id.universe = 1;
		id.accountType = 8;
		id.accountInstance = 0x40000;

		return id;
	}

	Network::Address Party::Target()
	{
		return Container.target;
	}

	void Party::Connect(Network::Address target)
	{
		Node::Add(target);

		Container.valid = true;
		Container.awaitingPlaylist = false;
		Container.joinTime = Game::Sys_Milliseconds();
		Container.target = target;
		Container.challenge = Utils::Cryptography::Rand::GenerateChallenge();

		if (IsUsingIw4xProtocol()) {
			Network::SendCommand(Container.target, "getinfo", Container.challenge);
		}
		else {
			Command::Execute("openmenu popup_reconnectingtoparty");
			SteamID id = Party::GenerateLobbyId();
			Party::LobbyMap[id.bits] = Container.target;
			Game::Steam_JoinLobby(id, 0);
		}
	}

	const char* Party::GetLobbyInfo(SteamID lobby, const std::string& key)
	{
		if (LobbyMap.contains(lobby.bits))
		{
			Network::Address address = LobbyMap[lobby.bits];

			if (key == "addr"s)
			{
				return Utils::String::VA("%d", address.getIP().full);
			}

			if (key == "port"s)
			{
				return Utils::String::VA("%d", address.getPort());
			}
		}

		return "212";
	}

	void Party::RemoveLobby(SteamID lobby)
	{
		LobbyMap.erase(lobby.bits);
	}

	void Party::ConnectError(const std::string& message)
	{
		Command::Execute("closemenu popup_reconnectingtoparty");
		Dvar::Var("partyend_reason").set(message);
		Command::Execute("openmenu menu_xboxlive_partyended");
	}

	std::string Party::GetMotd()
	{
		return Container.motd;
	}

	bool Party::IsUsingIw4xProtocol()
	{
		return Flags::HasFlag("iw4xprotocol");
	}

	std::string Party::GetHostName()
	{
		return Container.info.get("hostname");
	}

	int Party::GetMaxClients()
	{
		const auto value = Container.info.get("sv_maxclients");
		return std::strtol(value.data(), nullptr, 10);
	}

	bool Party::PlaylistAwaiting()
	{
		return Container.awaitingPlaylist;
	}

	void Party::PlaylistContinue()
	{
		Dvar::Var("xblive_privateserver").set(false);

		// Ensure we can join
		*Game::g_lobbyCreateInProgress = false;

		Container.awaitingPlaylist = false;

		SteamID id = GenerateLobbyId();

		// Temporary workaround
		// TODO: Patch the 127.0.0.1 -> loopback mapping in the party code
		if (Container.target.isLoopback())
		{
			if (*Game::numIP)
			{
				Container.target.setIP(*Game::localIP);
				Container.target.setType(Game::netadrtype_t::NA_IP);

				Logger::Print("Trying to connect to party with loopback address, using a local ip instead: {}\n", Container.target.getString());
			}
			else
			{
				Logger::Print("Trying to connect to party with loopback address, but no local ip was found.\n");
			}
		}

		LobbyMap[id.bits] = Container.target;

		Game::Steam_JoinLobby(id, 0);
	}

	void Party::PlaylistError(const std::string& error)
	{
		Container.valid = false;
		Container.awaitingPlaylist = false;

		ConnectError(error);
	}

	DWORD Party::UIDvarIntStub(char* dvar)
	{
		if (!_stricmp(dvar, "onlinegame") && !Stats::IsMaxLevel())
		{
			return 0x649E660;
		}

		return Utils::Hook::Call<DWORD(char*)>(0x4D5390)(dvar);
	}

	bool Party::IsInLobby()
	{
		return (!Dedicated::IsRunning() && PartyEnable.get<bool>() && Dvar::Var("party_host").get<bool>());
	}

	bool Party::IsInUserMapLobby()
	{
		return (IsInLobby() && Maps::IsUserMap((*Game::ui_mapname)->current.string));
	}

	bool Party::IsEnabled()
	{
		return PartyEnable.get<bool>();
	}

	__declspec(naked) void PartyMigrate_HandlePacket()
	{
		__asm
		{
			mov eax, 0;
			retn;
		}
	}

	void SV_SpawnServer_Com_SyncThreads_Hook()
	{
		Utils::Hook::Call<void()>(0x464A60)(); // Com_SyncThreads

		// Whenever the game starts a server,
		// RMsg_SendMessages so that everybody gets the PartyGo message!
		// (Otherwise Com_Try_Block doesn't send it until we're done loading, which times out some people)
		Utils::Hook::Call<void()>(0x49CC30)(); 
	}

	Party::Party()
	{
		if (ZoneBuilder::IsEnabled())
		{
			return;
		}

		// Send reliable messages when wse start the game
		Utils::Hook(0x4A712A, SV_SpawnServer_Com_SyncThreads_Hook, HOOK_CALL).install()->quick();
		Utils::Hook(0x5B34D0, SV_SpawnServer_Com_SyncThreads_Hook, HOOK_CALL).install()->quick();

		static Game::dvar_t* partyEnable = Dvar::Register<bool>("party_enable", IsUsingIw4xProtocol() ? Dedicated::IsEnabled() : true, Game::DVAR_NONE, "Enable party system").get<Game::dvar_t*>();
		Dvar::Register<bool>("xblive_privatematch", true, Game::DVAR_ROM, "").get<Game::dvar_t*>();
		Dvar::Register<bool>("sv_lanOnly", true, Game::DVAR_NONE, "Don't act as node");

		// Live frame skip "lobby owner is present in lobby" steam check
		Utils::Hook::Set<BYTE>(0x413744, 0xEB);

		// Kill the party migrate handler - it's not necessary and has apparently been used in the past for trickery?
		Utils::Hook(0x46AB70, PartyMigrate_HandlePacket, HOOK_JUMP).install()->quick();

		// various changes to SV_DirectConnect-y stuff to allow non-party joinees
		Utils::Hook::Set<WORD>(0x460D96, 0x90E9);
		Utils::Hook::Set<BYTE>(0x460F0A, 0xEB);
		Utils::Hook::Set<BYTE>(0x401CA4, 0xEB);
		Utils::Hook::Set<BYTE>(0x401C15, 0xEB);

		// disable configstring checksum matching (it's unreliable at most)
		Utils::Hook::Set<BYTE>(0x4A75A7, 0xEB); // SV_SpawnServer
		Utils::Hook::Set<BYTE>(0x5AC2CF, 0xEB); // CL_ParseGamestate
		Utils::Hook::Set<BYTE>(0x5AC2C3, 0xEB); // CL_ParseGamestate

		// AnonymousAddRequest
		Utils::Hook::Set<BYTE>(0x5B5E18, 0xEB);
		Utils::Hook::Set<BYTE>(0x5B5E64, 0xEB);
		Utils::Hook::Nop(0x5B5E5C, 2);

		// HandleClientHandshake
		Utils::Hook::Set<BYTE>(0x5B6EA5, 0xEB);
		Utils::Hook::Set<BYTE>(0x5B6EF3, 0xEB);
		Utils::Hook::Nop(0x5B6EEB, 2);

		// Allow local connections
		Utils::Hook::Set<BYTE>(0x4D43DA, 0xEB);

		// LobbyID mismatch
		Utils::Hook::Nop(0x4E50D6, 2);
		Utils::Hook::Set<BYTE>(0x4E50DA, 0xEB);

		// causes 'does current Steam lobby match' calls in Steam_JoinLobby to be ignored
		Utils::Hook::Set<BYTE>(0x49D007, 0xEB);

		// function checking party heartbeat timeouts, cause random issues
		//Utils::Hook::Nop(0x4E532D, 5); // PartyHost_TimeoutMembers

		// Steam_JoinLobby call causes migration
		Utils::Hook::Nop(0x5AF851, 5);
		Utils::Hook::Set<BYTE>(0x5AF85B, 0xEB);

		// Allow xpartygo in public lobbies
		Utils::Hook::Set<BYTE>(0x5A969E, 0xEB);
		Utils::Hook::Nop(0x5A96BE, 2);

		// Always open lobby menu when connecting
		// It's not possible to entirely patch it via code
		//Utils::Hook::Set<BYTE>(0x5B1698, 0xEB);
		//Utils::Hook::Nop(0x5029F2, 6);
		//Utils::Hook::SetString(0x70573C, "menu_xboxlive_lobby");

		// Disallow selecting team in private match
		//Utils::Hook::Nop(0x5B2BD8, 6);

		// Force teams, even if not private match
		Utils::Hook::Set<BYTE>(0x487BB2, 0xEB);

		// Force xblive_privatematch 0 and rename it
		//Utils::Hook::Set<BYTE>(0x420A6A, 4);
		if (!IsUsingIw4xProtocol())
		{
			Dvar::Register<bool>("xblive_privateserver", true, Game::DVAR_NONE, "").get<Game::dvar_t*>();
		}

		////Utils::Hook::Set<BYTE>(0x420A6C, 0);
		Utils::Hook::Set<const char*>(0x420A6E, "xblive_privateserver");

		// Remove migration shutdown, it causes crashes and will be destroyed when erroring anyways
		Utils::Hook::Nop(0x5A8E1C, 12);
		Utils::Hook::Nop(0x5A8E33, 11);

		// Enable XP Bar
		Utils::Hook(0x62A2A7, UIDvarIntStub, HOOK_CALL).install()->quick();

		// Set NAT to open
		Utils::Hook::Set<int>(0x79D898, 1);

		// Disable host migration
		Utils::Hook::Set<BYTE>(0x5B58B2, 0xEB);
		Utils::Hook::Set<BYTE>(0x4D6171, 0);
		Utils::Hook::Nop(0x4077A1, 5); // PartyMigrate_Frame

		// Patch playlist stuff for non-party behavior
		Utils::Hook::Set<Game::dvar_t**>(0x4A4093, &partyEnable);
		Utils::Hook::Set<Game::dvar_t**>(0x4573F1, &partyEnable);
		Utils::Hook::Set<Game::dvar_t**>(0x5B1A0C, &partyEnable);

		// Invert corresponding jumps
		Utils::Hook::Xor<BYTE>(0x4A409B, 1);
		Utils::Hook::Xor<BYTE>(0x4573FA, 1);
		Utils::Hook::Xor<BYTE>(0x5B1A17, 1);

		// Set ui_maxclients to sv_maxclients
		Utils::Hook::Set<const char*>(0x42618F, "sv_maxclients");
		Utils::Hook::Set<const char*>(0x4D3756, "sv_maxclients");
		Utils::Hook::Set<const char*>(0x5E3772, "sv_maxclients");

		// Unlatch maxclient dvars
		Utils::Hook::Xor<BYTE>(0x426187, Game::DVAR_LATCH);
		Utils::Hook::Xor<BYTE>(0x4D374E, Game::DVAR_LATCH);
		Utils::Hook::Xor<BYTE>(0x5E376A, Game::DVAR_LATCH);
		Utils::Hook::Xor<DWORD>(0x4261A1, Game::DVAR_LATCH);
		Utils::Hook::Xor<DWORD>(0x4D376D, Game::DVAR_LATCH);
		Utils::Hook::Xor<DWORD>(0x5E3789, Game::DVAR_LATCH);

		Command::Add("connect", [](const Command::Params* params)
			{
				if (params->size() < 2)
				{
					return;
				}

				if (Game::CL_IsCgameInitialized())
				{
					Command::Execute("disconnect", false);
					Command::Execute(Utils::String::VA("%s", params->join(0).data()), false);
				}
				else
				{
					Party::Connect(Network::Address(params->get(1)));
				}
			});

		Command::Add("reconnect", [](const Command::Params*)
			{
				Party::Connect(Container.target);
			});

		if (IsUsingIw4xProtocol()) {

			if (!Dedicated::IsEnabled() && !ZoneBuilder::IsEnabled())
			{
				Scheduler::Loop([]
					{
						if (Container.valid)
						{
							if ((Game::Sys_Milliseconds() - Container.joinTime) > 10'000)
							{
								Container.valid = false;
								Party::ConnectError("Server connection timed out.");
							}
						}

						if (Container.awaitingPlaylist)
						{
							if ((Game::Sys_Milliseconds() - Container.requestTime) > 5'000)
							{
								Container.awaitingPlaylist = false;
								Party::ConnectError("Playlist request timed out.");
							}
						}

					}, Scheduler::Pipeline::CLIENT);
			}

			// Basic info handler
			Network::OnClientPacket("getInfo", [](const Network::Address& address, [[maybe_unused]] const std::string& data)
				{
					int botCount = 0;
					int clientCount = 0;
					int maxclientCount = *Game::svs_clientCount;

					if (maxclientCount)
					{
						for (int i = 0; i < maxclientCount; ++i)
						{
							if (Game::svs_clients[i].header.state >= Game::CS_CONNECTED)
							{
								if (Game::svs_clients[i].bIsTestClient) ++botCount;
								else ++clientCount;
							}
						}
					}
					else
					{
						maxclientCount = Dvar::Var("party_maxplayers").get<int>();
						clientCount = Game::PartyHost_CountMembers(reinterpret_cast<Game::PartyData*>(0x1081C00));
					}

					Utils::InfoString info;
					info.set("challenge", Utils::ParseChallenge(data));
					info.set("gamename", "IW4");
					info.set("hostname", (*Game::sv_hostname)->current.string);
					info.set("gametype", (*Game::sv_gametype)->current.string);
					info.set("fs_game", (*Game::fs_gameDirVar)->current.string);
					info.set("xuid", Utils::String::VA("%llX", Steam::SteamUser()->GetSteamID().bits));
					info.set("clients", Utils::String::VA("%i", clientCount));
					info.set("bots", Utils::String::VA("%i", botCount));
					info.set("sv_maxclients", Utils::String::VA("%i", maxclientCount));
					info.set("protocol", Utils::String::VA("%i", PROTOCOL));
					info.set("shortversion", SHORTVERSION);
					info.set("checksum", Utils::String::VA("%d", Game::Sys_Milliseconds()));
					info.set("mapname", Dvar::Var("mapname").get<const char*>());
					info.set("isPrivate", (Dvar::Var("g_password").get<std::string>().size() ? "1" : "0"));
					info.set("hc", (Dvar::Var("g_hardcore").get<bool>() ? "1" : "0"));
					info.set("securityLevel", Utils::String::VA("%i", Dvar::Var("sv_securityLevel").get<int>()));
					info.set("sv_running", ((*Game::com_sv_running)->current.enabled ? "1" : "0"));
					info.set("aimAssist", (Gamepad::sv_allowAimAssist.get<bool>() ? "1" : "0"));
					info.set("voiceChat", (Voice::SV_VoiceEnabled() ? "1" : "0"));

					// Ensure mapname is set
					if (info.get("mapname").empty() || Party::IsInLobby())
					{
						info.set("mapname", Dvar::Var("ui_mapname").get<const char*>());
					}

					if (Maps::GetUserMap()->isValid())
					{
						info.set("usermaphash", Utils::String::VA("%i", Maps::GetUserMap()->getHash()));
					}
					else if (Party::IsInUserMapLobby())
					{
						info.set("usermaphash", Utils::String::VA("%i", Maps::GetUsermapHash(info.get("mapname"))));
					}

					if (Dedicated::IsEnabled())
					{
						info.set("sv_motd", Dvar::Var("sv_motd").get<std::string>());
					}

					// Set matchtype
					// 0 - No match, connecting not possible
					// 1 - Party, use Steam_JoinLobby to connect
					// 2 - Match, use CL_ConnectFromParty to connect

					if (PartyEnable.get<bool>() && Dvar::Var("party_host").get<bool>()) // Party hosting
					{
						info.set("matchtype", "1");
					}
					else if (Dvar::Var("sv_running").get<bool>()) // Match hosting
					{
						info.set("matchtype", "2");
					}
					else
					{
						info.set("matchtype", "0");
					}

						if (ModList::cl_modVidRestart.get<bool>())
						{
							Command::Execute("vid_restart", false);
						}

					Network::SendCommand(address, "infoResponse", "\\" + info.build());
				});

			Network::OnClientPacket("infoResponse", [](const Network::Address& address, [[maybe_unused]] const std::string& data)
				{
					Utils::InfoString info(data);

					// Handle connection
					if (Container.valid)
					{
						if (Container.target == address)
						{
							// Invalidate handler for future packets
							Container.valid = false;
							Container.info = info;

							Container.matchType = atoi(info.get("matchtype").data());
							uint32_t securityLevel = static_cast<uint32_t>(atoi(info.get("securityLevel").data()));
							bool isUsermap = !info.get("usermaphash").empty();
							unsigned int usermapHash = atoi(info.get("usermaphash").data());

							std::string mod = (*Game::fs_gameDirVar)->current.string;

							// set fast server stuff here so its updated when we go to download stuff
							if (info.get("wwwDownload") == "1"s)
							{
								Dvar::Var("sv_wwwDownload").set(true);
								Dvar::Var("sv_wwwBaseUrl").set(info.get("wwwUrl"));
							}
							else
							{
								Dvar::Var("sv_wwwDownload").set(false);
								Dvar::Var("sv_wwwBaseUrl").set("");
							}

							if (info.get("challenge") != Container.challenge)
							{
								Party::ConnectError("Invalid join response: Challenge mismatch.");
							}
							else if (securityLevel > Auth::GetSecurityLevel())
							{
								//Party::ConnectError(Utils::VA("Your security level (%d) is lower than the server's (%d)", Auth::GetSecurityLevel(), securityLevel));
								Command::Execute("closemenu popup_reconnectingtoparty");
								Auth::IncreaseSecurityLevel(securityLevel, "reconnect");
							}
							else if (!Container.matchType)
							{
								Party::ConnectError("Server is not hosting a match.");
							}
							else if (Container.matchType > 2 || Container.matchType < 0)
							{
								Party::ConnectError("Invalid join response: Unknown matchtype");
							}
							else if (Container.info.get("mapname").empty() || Container.info.get("gametype").empty())
							{
								Party::ConnectError("Invalid map or gametype.");
							}
							else if (Container.info.get("isPrivate") == "1"s && !Dvar::Var("password").get<std::string>().length())
							{
								Party::ConnectError("A password is required to join this server! Set it at the bottom of the serverlist.");
							}
							else if (isUsermap && usermapHash != Maps::GetUsermapHash(info.get("mapname")))
							{
								Command::Execute("closemenu popup_reconnectingtoparty");
								Download::InitiateMapDownload(info.get("mapname"), info.get("isPrivate") == "1");
							}
							else if (!info.get("fs_game").empty() && Utils::String::ToLower(mod) != Utils::String::ToLower(info.get("fs_game")))
							{
								Command::Execute("closemenu popup_reconnectingtoparty");
								Download::InitiateClientDownload(info.get("fs_game"), info.get("isPrivate") == "1"s);
							}
							else if (!Dvar::Var("fs_game").get<std::string>().empty() && info.get("fs_game").empty())
							{
								Game::Dvar_SetString(*Game::fs_gameDirVar, "");

								if (Dvar::Var("cl_modVidRestart").get<bool>())
								{
									Command::Execute("vid_restart", false);
								}

								Command::Execute("reconnect", false);
							}
							else
							{
								if (!Maps::CheckMapInstalled(Container.info.get("mapname").data(), true)) return;

								Container.motd = info.get("sv_motd");

								if (Container.matchType == 1) // Party
								{
									// Send playlist request
									Container.requestTime = Game::Sys_Milliseconds();
									Container.awaitingPlaylist = true;
									Network::SendCommand(Container.target, "getplaylist", Dvar::Var("password").get<std::string>());

									// This is not a safe method
									// TODO: Fix actual error!
									if (Game::CL_IsCgameInitialized())
									{
										Command::Execute("disconnect", true);
									}
								}
								else if (Container.matchType == 2) // Match
								{
									if (atoi(Container.info.get("clients").data()) >= atoi(Container.info.get("sv_maxclients").data()))
									{
										Party::ConnectError("@EXE_SERVERISFULL");
									}
									else
									{
										Dvar::Var("xblive_privateserver").set(true);

										Game::Menus_CloseAll(Game::uiContext);

										Game::_XSESSION_INFO hostInfo;
										Game::CL_ConnectFromParty(0, &hostInfo, *Container.target.get(), 0, 0, Container.info.get("mapname").data(), Container.info.get("gametype").data());
									}
								}
							}
						}
					}

					ServerList::Insert(address, info);
					Friends::UpdateServer(address, info.get("hostname"), info.get("mapname"));
				});
		}
	}
}
