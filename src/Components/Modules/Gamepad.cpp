#include "STDInclude.hpp"

namespace Components
{
	std::vector<Gamepad::ActionMapping> mappings = {
		Gamepad::ActionMapping(XINPUT_GAMEPAD_A, "gostand"),
		Gamepad::ActionMapping(XINPUT_GAMEPAD_B, "togglecrouch", false),
		Gamepad::ActionMapping(XINPUT_GAMEPAD_X, "activate"),
		Gamepad::ActionMapping(XINPUT_GAMEPAD_X, "reload"),
		Gamepad::ActionMapping(XINPUT_GAMEPAD_Y, "weapnext", false),
		Gamepad::ActionMapping(XINPUT_GAMEPAD_LEFT_SHOULDER, "smoke"),
		Gamepad::ActionMapping(XINPUT_GAMEPAD_RIGHT_SHOULDER, "frag"),
		Gamepad::ActionMapping(XINPUT_GAMEPAD_LEFT_THUMB,  "breath_sprint"),
		Gamepad::ActionMapping(XINPUT_GAMEPAD_RIGHT_THUMB, "melee"),
		Gamepad::ActionMapping(XINPUT_GAMEPAD_START, "togglemenu", false),
		Gamepad::ActionMapping(XINPUT_GAMEPAD_BACK, "scores")
	};

	bool wasFiring = false;
	bool wasAiming = false;

	float leftThumbX = 0;
	float leftThumbY = 0;

	float rightThumbX = 0;
	float rightThumbY = 0;

	const float deadZone = 0.3f;

	XINPUT_STATE Gamepad::GetState()
	{
		// Zeroise the state
		ZeroMemory(&controllerState, sizeof(XINPUT_STATE));

		// Get the state
		XInputGetState(controllerNum, &controllerState);

		return controllerState;
	}

	bool Gamepad::IsConnected()
	{
		// Zeroise the state
		ZeroMemory(&controllerState, sizeof(XINPUT_STATE));

		// Get the state
		DWORD Result = XInputGetState(controllerNum, &controllerState);

		if (Result == ERROR_SUCCESS)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	void Gamepad::Vibrate(int leftVal, int rightVal)
	{
		// Create a Vibraton State
		XINPUT_VIBRATION Vibration;

		// Zeroise the Vibration
		ZeroMemory(&Vibration, sizeof(XINPUT_VIBRATION));

		// Set the Vibration Values
		Vibration.wLeftMotorSpeed = leftVal;
		Vibration.wRightMotorSpeed = rightVal;

		// Vibrate the controller
		XInputSetState(controllerNum, &Vibration);
	}

	char Gamepad::MovementOverride(int a1, Game::usercmd_s* cmd) {
		char result = Gamepad::CL_KeyMoveCL(a1, cmd);
		cmd->forwardmove = leftThumbY * 127;
		cmd->rightmove = leftThumbX * 127;
		return result;
	}

	void Gamepad::MouseOverride(Game::clientActive_t* clientActive, float* mx, float* my) {

		Gamepad::CL_GetMouseMovementCl(clientActive, mx, my);

		//*(my) = std::round(rightThumbY*10);
		//*(mx) = (time(NULL)%4)*100;

		*(my) = rightThumbX * Dvar::Var("sensitivity").get<float>();
		*(mx) = -rightThumbY * Dvar::Var("sensitivity").get<float>();

		auto& cmd = clientActive->cmds[clientActive->cmdNumber];


		//cmd.forwardmove = leftThumbY * 128 + 127;
		//cmd.rightmove = leftThumbX * 128 + 127;

	/*	clientActive->mouseDx[clientActive->mouseIndex] = -100;
		clientActive->mouseDy[clientActive->mouseIndex] = (time(NULL)%4) * 50;

		Game::Font_s* font = Game::R_RegisterFont("fonts/normalFont", 0);
		float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

		Game::R_AddCmdDrawText(("MX: "+(std::to_string(*mx))).c_str(), 0x7FFFFFFF, font, 15.0f, 40.0f, 1.0f, 1.0f, 0.0f, color, Game::ITEM_TEXTSTYLE_SHADOWED);*/
	}

	// Game -> Client DLL
	__declspec(naked) void CL_KeyMoveStub()
	{
		__asm
		{
			push edi;
			push eax;
			call Gamepad::MovementOverride;
			add esp, 0x8;
			ret;
		}
	}


	// Game -> Client DLL
	__declspec(naked) void CL_GetMouseMovementStub()
	{
		__asm
		{
			push edx;
			push ecx;
			push eax;
			call Gamepad::MouseOverride;
			add esp,0xC;
			ret;
		}
	}

	// Client DLL -> Game
	char Gamepad::CL_KeyMoveCL(int a1, Game::usercmd_s* cmd)
	{
		char result;
		__asm
		{
			push edi;
			push ebx;
			mov eax, a1;
			mov edi, cmd;
			mov ebx, 0x5A5F40;
			call ebx;
			pop ebx;
			pop edi;
			mov result, al;
		}

		return result;
	}

	// Client DLL -> Game
	void Gamepad::CL_GetMouseMovementCl(Game::clientActive_t* result, float* mx, float* my)
	{
		__asm
		{
			push ebx;
			push ecx;
			push edx;
			mov eax, result;
			mov ecx, mx;
			mov edx, my;
			mov ebx, 5A60E0h;
			call ebx;
			pop edx;
			pop ecx;
			pop ebx;
		}
	}

	void Gamepad::Update() {

		if (IsConnected()) {
			auto state = GetState();

			// Buttons (on/off) mappings
			for (size_t i = 0; i < mappings.size(); i++)
			{
				auto mapping = mappings[i];
				auto action = mapping.action;
				auto antiAction = mapping.action;

				if (mapping.isReversible) {
					action = "+" + mapping.action;
					antiAction = "-" + mapping.action;
				}
				else if (mapping.wasPressed) {
					if (state.Gamepad.wButtons & mapping.input) {
						// Button still pressed, do not send info
					}
					else {
						mappings[i].wasPressed = false;
					}

					continue;
				}

				if (state.Gamepad.wButtons & mapping.input) {
					Game::Cmd_ExecuteSingleCommand(0, 0, action.c_str());
					mappings[i].wasPressed = true;
				}
				else if (mapping.isReversible && mapping.wasPressed) {
					mappings[i].wasPressed = false;
					Game::Cmd_ExecuteSingleCommand(0, 0, antiAction.c_str());
				}
			}

			// Trigger mappings
			if (state.Gamepad.bLeftTrigger > std::numeric_limits<BYTE>::max() / 2) {
				wasAiming = true;
				Game::Cmd_ExecuteSingleCommand(0, 0,"+speed_throw");
			}
			else if (wasAiming){
				wasAiming = false;
				Game::Cmd_ExecuteSingleCommand(0, 0, "-speed_throw");
			}

			if (state.Gamepad.bRightTrigger > std::numeric_limits<BYTE>::max() / 2) {
				wasFiring = true;
				Game::Cmd_ExecuteSingleCommand(0, 0, "+attack");
			}
			else {
				wasFiring = false;
				Game::Cmd_ExecuteSingleCommand(0, 0, "-attack");
			}

			// Sticks
			leftThumbX = state.Gamepad.sThumbLX / (float)std::numeric_limits<SHORT>::max();
			leftThumbY = state.Gamepad.sThumbLY / (float)std::numeric_limits<SHORT>::max();
			rightThumbX = state.Gamepad.sThumbRX / (float)std::numeric_limits<SHORT>::max();
			rightThumbY = state.Gamepad.sThumbRY / (float)std::numeric_limits<SHORT>::max();

			if (std::abs(leftThumbX) < deadZone) leftThumbX = 0;
			if (std::abs(leftThumbY) < deadZone) leftThumbY = 0;
			if (std::abs(rightThumbX) < deadZone) rightThumbX = 0;
			if (std::abs(rightThumbY) < deadZone) rightThumbY = 0;
		}
	}

	Gamepad::Gamepad()
	{
		if (ZoneBuilder::IsEnabled() || Dedicated::IsEnabled()) return;

		if (IsConnected()) {
			Vibrate(30000, 30000);
		}

		Scheduler::OnFrame([this]() {
			Update();
			});

		Utils::Hook(0x5A617D, CL_GetMouseMovementStub, HOOK_CALL).install()->quick();
		Utils::Hook(0x5A6D8B, CL_KeyMoveStub, HOOK_CALL).install()->quick();

		printf("");
	}

	Gamepad::~Gamepad()
	{

	}
}
