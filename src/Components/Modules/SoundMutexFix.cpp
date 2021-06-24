#include "STDInclude.hpp"

namespace Components
{
	std::mutex SoundMutexFix::snd_mutex;
	static const DWORD SND_StopStreamChannel_stub_1 = 0x00430BF7;

	void __declspec(naked) SND_StopStreamChannel_stub(int)
	{
		__asm
		{
			push ebp
			push esi
			push edi
			mov edi, [esp + 0x10]
			jmp SND_StopStreamChannel_stub_1
		}
	}

	static void __stdcall LockSoundMutex(int unk)
	{
		std::lock_guard lock(SoundMutexFix::snd_mutex);

		DWORD funcPtr = *(DWORD*)0x006D7554;
		((void(__stdcall*)(int unk))(funcPtr))(unk);
	}

	void SoundMutexFix::SND_StopStreamChannel_hk(int channel)
	{

		SND_StopStreamChannel_stub(channel);
	}

	SoundMutexFix::SoundMutexFix()
	{
		//Utils::Hook(0x430BF0, &TestPatch::SND_StopStreamChannel_hk, HOOK_JUMP).install()->quick();
		Utils::Hook(0x689EFE, &LockSoundMutex, HOOK_JUMP).install()->quick();
	}

	SoundMutexFix::~SoundMutexFix()
	{
	}
}