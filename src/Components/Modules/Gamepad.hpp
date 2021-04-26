#pragma once

namespace Components
{
	class Gamepad : public Component
	{
	public:
		Gamepad();
		~Gamepad();

		struct ActionMapping {
			int input;
			std::string action;
			bool isReversible;
			bool wasPressed = false;

			ActionMapping(int input, std::string action, bool isReversible = true)
			{
				this->action = action;
				this->isReversible = isReversible;
				this->input = input;
			}
		};

	private:

		static void CL_GetMouseMovementCl(Game::clientActive_t* result, float* mx, float* my);
		static char CL_KeyMoveCL(int a1, Game::usercmd_s* cmd);

		static void MouseOverride(Game::clientActive_t* clientActive, float* my, float* mx);
		static char MovementOverride(int a1, Game::usercmd_s* cmd);
		XINPUT_STATE GetState();
		bool IsConnected();
		void Vibrate(int leftVal = 0, int rightVal = 0);
		void Update();

		XINPUT_STATE controllerState;
		int controllerNum = 0;
	};
}
