#include <STDInclude.hpp>
#include "ScriptError.hpp"

#define SCRIPT_ERROR_PATCH

using namespace Utils::String;

namespace Components
{
	int ScriptError::developer_;

	Game::scrParserGlob_t ScriptError::scrParserGlob;
	Game::scrParserPub_t ScriptError::scrParserPub;

	Game::HunkUser* ScriptError::g_debugUser;

	int ScriptError::Scr_IsInOpcodeMemory(const char* pos)
	{
		assert(Game::scrVarPub->programBuffer);
		assert(pos);

		return pos - Game::scrVarPub->programBuffer < static_cast<int>(Game::scrCompilePub->programLen);
	}

	void ScriptError::AddOpcodePos(unsigned int sourcePos, int type)
	{
		Game::OpcodeLookup* opcodeLookup;
		Game::SourceLookup* sourcePosLookup;

		if (!developer_)
		{
			return;
		}

		if (Game::scrCompilePub->developer_statement == 2)
		{
			assert(!Game::scrVarPub->developer_script);
			return;
		}

		if (Game::scrCompilePub->developer_statement == 3)
		{
			return;
		}

		if (!Game::scrCompilePub->allowedBreakpoint)
		{
			type &= ~Game::SOURCE_TYPE_BREAKPOINT;
		}

		assert(scrParserGlob.opcodeLookup);
		assert(scrParserGlob.sourcePosLookup);
		assert(Game::scrCompilePub->opcodePos);

		auto size = sizeof(Game::OpcodeLookup) * (scrParserGlob.opcodeLookupLen + 1);
		if (size > scrParserGlob.opcodeLookupMaxSize)
		{
			if (scrParserGlob.opcodeLookupMaxSize >= Game::MAX_OPCODE_LOOKUP_SIZE)
			{
				Game::Sys_Error("MAX_OPCODE_LOOKUP_SIZE exceeded");
			}

			Game::Z_VirtualCommit((char*)scrParserGlob.opcodeLookup + scrParserGlob.opcodeLookupMaxSize, 0x20000);
			scrParserGlob.opcodeLookupMaxSize += 0x20000;
			assert(size <= scrParserGlob.opcodeLookupMaxSize);
		}

		size = sizeof(Game::SourceLookup) * (scrParserGlob.sourcePosLookupLen + 1);
		if (size > scrParserGlob.sourcePosLookupMaxSize)
		{
			if (scrParserGlob.sourcePosLookupMaxSize >= Game::MAX_SOURCEPOS_LOOKUP_SIZE)
			{
				Game::Sys_Error("MAX_SOURCEPOS_LOOKUP_SIZE exceeded");
			}

			Game::Z_VirtualCommit((char*)scrParserGlob.sourcePosLookup + scrParserGlob.sourcePosLookupMaxSize, 0x20000);
			scrParserGlob.sourcePosLookupMaxSize += 0x20000;
			assert(size <= scrParserGlob.sourcePosLookupMaxSize);
		}

		if (scrParserGlob.currentCodePos == Game::scrCompilePub->opcodePos)
		{
			assert(scrParserGlob.currentSourcePosCount);
			--scrParserGlob.opcodeLookupLen;
			opcodeLookup = &scrParserGlob.opcodeLookup[scrParserGlob.opcodeLookupLen];
			assert(opcodeLookup->sourcePosIndex + scrParserGlob.currentSourcePosCount == scrParserGlob.sourcePosLookupLen);
			assert(opcodeLookup->codePos == (char*)scrParserGlob.currentCodePos);
		}
		else
		{
			scrParserGlob.currentSourcePosCount = 0;
			scrParserGlob.currentCodePos = Game::scrCompilePub->opcodePos;
			opcodeLookup = &scrParserGlob.opcodeLookup[scrParserGlob.opcodeLookupLen];
			opcodeLookup->sourcePosIndex = scrParserGlob.sourcePosLookupLen;
			opcodeLookup->codePos = scrParserGlob.currentCodePos;
		}

		auto sourcePosLookupIndex = scrParserGlob.currentSourcePosCount + opcodeLookup->sourcePosIndex;
		sourcePosLookup = &scrParserGlob.sourcePosLookup[sourcePosLookupIndex];
		sourcePosLookup->sourcePos = sourcePos;

		if (sourcePos == static_cast<unsigned int>(-1))
		{
			assert(scrParserGlob.delayedSourceIndex == -1);
			assert(type & Game::SOURCE_TYPE_BREAKPOINT);
			scrParserGlob.delayedSourceIndex = static_cast<int>(sourcePosLookupIndex);
		}
		else if (sourcePos == static_cast<unsigned int>(-2))
		{
			scrParserGlob.threadStartSourceIndex = static_cast<int>(sourcePosLookupIndex);
		}
		else if (scrParserGlob.delayedSourceIndex >= 0 && (type & Game::SOURCE_TYPE_BREAKPOINT))
		{
			scrParserGlob.sourcePosLookup[scrParserGlob.delayedSourceIndex].sourcePos = sourcePos;
			scrParserGlob.delayedSourceIndex = -1;
		}

		sourcePosLookup->type |= type;
		++scrParserGlob.currentSourcePosCount;
		opcodeLookup->sourcePosCount = static_cast<unsigned short>(scrParserGlob.currentSourcePosCount);
		++scrParserGlob.opcodeLookupLen;
		++scrParserGlob.sourcePosLookupLen;
	}

	void ScriptError::RemoveOpcodePos()
	{
		if (!developer_)
		{
			return;
		}

		if (Game::scrCompilePub->developer_statement == 2)
		{
			assert(!Game::scrVarPub->developer_script);
			return;
		}

		assert(scrParserGlob.opcodeLookup);
		assert(scrParserGlob.sourcePosLookup);
		assert(Game::scrCompilePub->opcodePos);
		assert(scrParserGlob.sourcePosLookupLen);

		--scrParserGlob.sourcePosLookupLen;
		assert(scrParserGlob.opcodeLookupLen);

		--scrParserGlob.opcodeLookupLen;
		assert(scrParserGlob.currentSourcePosCount);
		--scrParserGlob.currentSourcePosCount;

		auto* opcodeLookup = &scrParserGlob.opcodeLookup[scrParserGlob.opcodeLookupLen];

		assert(scrParserGlob.currentCodePos == Game::scrCompilePub->opcodePos);
		assert(opcodeLookup->sourcePosIndex + scrParserGlob.currentSourcePosCount == scrParserGlob.sourcePosLookupLen);
		assert(opcodeLookup->codePos == (char*)scrParserGlob.currentCodePos);

		if (!scrParserGlob.currentSourcePosCount)
		{
			scrParserGlob.currentCodePos = nullptr;
		}

		opcodeLookup->sourcePosCount = static_cast<unsigned short>(scrParserGlob.currentSourcePosCount);
	}

	void ScriptError::AddThreadStartOpcodePos(unsigned int sourcePos)
	{
		if (!developer_)
		{
			return;
		}

		if (Game::scrCompilePub->developer_statement == 2)
		{
			assert(!Game::scrVarPub->developer_script);
		}
		else
		{
			assert(scrParserGlob.threadStartSourceIndex >= 0);
			auto* sourcePosLookup = &scrParserGlob.sourcePosLookup[scrParserGlob.threadStartSourceIndex];
			sourcePosLookup->sourcePos = sourcePos;
			assert(!sourcePosLookup->type);
			sourcePosLookup->type = 8;
			scrParserGlob.threadStartSourceIndex = -1;
		}
	}

	int ScriptError::Scr_GetLineNum(unsigned int bufferIndex, unsigned int sourcePos)
	{
		const char* startLine;
		int col;

		assert(developer_);
		return Scr_GetLineNumInternal(scrParserPub.sourceBufferLookup[bufferIndex].sourceBuf, sourcePos, &startLine, &col, nullptr);
	}

	unsigned int ScriptError::Scr_GetPrevSourcePos(const char* codePos, unsigned int index)
	{
		return scrParserGlob.sourcePosLookup[index + Scr_GetPrevSourcePosOpcodeLookup(codePos)->sourcePosIndex].sourcePos;
	}

	Game::OpcodeLookup* ScriptError::Scr_GetPrevSourcePosOpcodeLookup(const char* codePos)
	{
		assert(Scr_IsInOpcodeMemory(codePos));
		assert(scrParserGlob.opcodeLookup);

		unsigned int low = 0;
		unsigned int high = scrParserGlob.opcodeLookupLen - 1;
		while (low <= high)
		{
			unsigned int middle = (high + low) >> 1;
			if (codePos < scrParserGlob.opcodeLookup[middle].codePos)
			{
				high = middle - 1;
			}
			else
			{
				low = middle + 1;
				if (low == scrParserGlob.opcodeLookupLen || codePos < scrParserGlob.opcodeLookup[low].codePos)
				{
					return &scrParserGlob.opcodeLookup[middle];
				}
			}
		}

		assert(0 && "unreachable");
		return nullptr;
	}

	void ScriptError::Scr_CopyFormattedLine(char* line, const char* rawLine)
	{
		auto len = static_cast<int>(std::strlen(rawLine));
		if (len >= 1024)
		{
			len = 1024 - 1;
		}

		for (auto i = 0; i < len; ++i)
		{
			if (rawLine[i] == '\t')
			{
				line[i] = ' ';
			}
			else
			{
				line[i] = rawLine[i];
			}
		}

		if (line[len - 1] == '\r')
		{
			line[len - 1] = '\0';
		}

		line[len] = '\0';
	}

	int ScriptError::Scr_GetLineNumInternal(const char* buf, unsigned int sourcePos, const char** startLine, int* col, [[maybe_unused]] Game::SourceBufferInfo* binfo)
	{
		assert(buf);

		*startLine = buf;
		unsigned int lineNum = 0;
		while (sourcePos)
		{
			if (!*buf)
			{
				*startLine = buf + 1;
				++lineNum;
			}
			++buf;
			--sourcePos;
		}

		*col = buf - *startLine;
		return static_cast<int>(lineNum);
	}

	unsigned int ScriptError::Scr_GetSourceBuffer(const char* codePos)
	{
		unsigned int bufferIndex;

		assert(Scr_IsInOpcodeMemory(codePos));
		assert(scrParserPub.sourceBufferLookupLen > 0);

		for (bufferIndex = scrParserPub.sourceBufferLookupLen - 1; bufferIndex; --bufferIndex)
		{
			if (!scrParserPub.sourceBufferLookup[bufferIndex].codePos)
			{
				continue;
			}

			if (scrParserPub.sourceBufferLookup[bufferIndex].codePos > codePos)
			{
				continue;
			}

			break;
		}

		return bufferIndex;
	}

	void ScriptError::Scr_PrintPrevCodePos(int channel, const char* codePos, unsigned int index)
	{
		if (!codePos)
		{
			Game::Com_PrintMessage(channel, "<frozen thread>\n", 0);
			return;
		}

		if (codePos == Game::g_EndPos)
		{
			Game::Com_PrintMessage(channel, "<removed thread>\n", 0);
			return;
		}

		if (!developer_)
		{
			if (Scr_IsInOpcodeMemory(codePos - 1))
			{
				Game::Com_PrintMessage(channel, VA("@ %d\n", codePos - Game::scrVarPub->programBuffer), 0);
				return;
			}
		}
		else
		{
			if (Game::scrVarPub->programBuffer && Scr_IsInOpcodeMemory(codePos))
			{
				auto bufferIndex = Scr_GetSourceBuffer(codePos - 1);
				Scr_PrintSourcePos(channel, scrParserPub.sourceBufferLookup[bufferIndex].buf, scrParserPub.sourceBufferLookup[bufferIndex].sourceBuf, Scr_GetPrevSourcePos(codePos - 1, index));
				return;
			}
		}

		Game::Com_PrintMessage(channel, VA("%s\n\n", codePos), 0);
	}

	int ScriptError::Scr_GetLineInfo(const char* buf, unsigned int sourcePos, int* col, char* line, Game::SourceBufferInfo* binfo)
	{
		const char* startLine;
		unsigned int lineNum;

		if (buf)
		{
			lineNum = Scr_GetLineNumInternal(buf, sourcePos, &startLine, col, binfo);
		}
		else
		{
			lineNum = 0;
			startLine = "";
			*col = 0;
		}

		Scr_CopyFormattedLine(line, startLine);
		return static_cast<int>(lineNum);
	}

	void ScriptError::Scr_PrintSourcePos(int channel, const char* filename, const char* buf, unsigned int sourcePos)
	{
		char line[1024];
		int col;

		assert(filename);
		auto lineNum = Scr_GetLineInfo(buf, sourcePos, &col, line, nullptr);

		Game::Com_PrintMessage(channel, VA("(file '%s'%s, line %d)\n", filename, scrParserGlob.saveSourceBufferLookup ? " (savegame)" : "", lineNum + 1), 0);
		Game::Com_PrintMessage(channel, VA("%s\n", line), 0);

		for (auto i = 0; i < col; ++i)
		{
			Game::Com_PrintMessage(channel, " ", 0);
		}

		Game::Com_PrintMessage(channel, "*\n", 0);
	}

	void ScriptError::RuntimeErrorInternal(int channel, const char* codePos, unsigned int index, const char* msg)
	{
		assert(Scr_IsInOpcodeMemory(codePos));

		Game::Com_PrintError(channel, "\n******* script runtime error *******\n%s: ", msg);
		Scr_PrintPrevCodePos(channel, codePos, index);

		if (Game::scrVmPub->function_count)
		{
			for (auto i = Game::scrVmPub->function_count - 1; i >= 1; --i)
			{
				Game::Com_PrintError(channel, "called from:\n");
				Scr_PrintPrevCodePos(0, Game::scrVmPub->function_frame_start[i].fs.pos, Game::scrVmPub->function_frame_start[i].fs.localId == 0);
			}

			Game::Com_PrintError(channel, "started from:\n");
			Scr_PrintPrevCodePos(0, Game::scrVmPub->function_frame_start[0].fs.pos, 1);
		}

		Game::Com_PrintError(channel, "************************************\n");
	}

	void ScriptError::RuntimeError(const char* codePos, unsigned int index, const char* msg, const char* dialogMessage)
	{
		bool abort_on_error;
		const char* dialogMessageSeparator;

		if (!developer_)
		{
			assert(Scr_IsInOpcodeMemory(codePos));
			if (!(*Game::com_developer)->current.enabled)
			{
				return;
			}
		}

		if (Game::scrVmPub->debugCode)
		{
			Game::Com_Printf(Game::CON_CHANNEL_PARSERSCRIPT, "%s\n", msg);
			if (!Game::scrVmPub->terminal_error)
			{
				return;
			}
			goto error;
		}

		abort_on_error = Game::scrVmPub->terminal_error;
		RuntimeErrorInternal(abort_on_error ? Game::CON_CHANNEL_LOGFILEONLY : Game::CON_CHANNEL_PARSERSCRIPT, codePos, index, msg);
		if (abort_on_error)
		{
		error:
			if (!dialogMessage)
			{
				dialogMessage = "";
				dialogMessageSeparator = "";
			}
			else
			{
				dialogMessageSeparator = "\n";
			}

			Game::Com_Error(Game::scrVmPub->terminal_error ? Game::ERR_SCRIPT_DROP : Game::ERR_SCRIPT, "\x15script runtime error\n(see console for details)\n%s%s%s", msg, dialogMessageSeparator, dialogMessage);
		}
	}

	void ScriptError::CompileError(unsigned int sourcePos, const char* msg, ...)
	{
		char line[1024];
		char text[1024];
		int col;
		va_list argptr;

		va_start(argptr, msg);
		vsnprintf_s(text, _TRUNCATE, msg, argptr);
		va_end(argptr);

		if (Game::scrVarPub->evaluate)
		{
			if (!Game::scrVarPub->error_message)
			{
				Game::scrVarPub->error_message = VA("%s", text);
			}
		}
		else
		{
			Game::Scr_ShutdownAllocNode();
			Game::Com_PrintError(Game::CON_CHANNEL_PARSERSCRIPT, "\n");
			Game::Com_PrintError(Game::CON_CHANNEL_PARSERSCRIPT, "******* script compile error *******\n");

			if (!developer_ || !scrParserPub.sourceBuf)
			{
				Game::Com_PrintError(Game::CON_CHANNEL_PARSERSCRIPT, "%s\n", text);
				line[0] = '\0';

				Game::Com_Printf(Game::CON_CHANNEL_PARSERSCRIPT, "************************************\n");
				Game::Com_Error(Game::ERR_SCRIPT_DROP, "\x15" "script compile error\n%s\n%s\n(see console for details)\n", text, line);
			}
			else
			{
				assert(scrParserPub.sourceBuf);
				Game::Com_PrintError(Game::CON_CHANNEL_PARSERSCRIPT, "%s: ", text);

				Scr_PrintSourcePos(Game::CON_CHANNEL_PARSERSCRIPT, scrParserPub.scriptfilename, scrParserPub.sourceBuf, sourcePos);
				auto lineNumber = Scr_GetLineInfo(scrParserPub.sourceBuf, sourcePos, &col, line, nullptr);
				Game::Com_Error(Game::ERR_SCRIPT_DROP, "\x15" "script compile error\n%s\n%s(%d):\n %s\n(see console for details)\n", text, scrParserPub.scriptfilename, lineNumber, line);
			}
		}
	}

	void ScriptError::CompileError2(const char* codePos, const char* msg, ...)
	{
		char line[1024];
		char text[1024];
		va_list argptr;

		assert(!Game::scrVarPub->evaluate);
		assert(Scr_IsInOpcodeMemory(codePos));

		Game::Com_PrintError(Game::CON_CHANNEL_PARSERSCRIPT, "\n");
		Game::Com_PrintError(Game::CON_CHANNEL_PARSERSCRIPT, "******* script compile error *******\n");

		va_start(argptr, msg);
		vsnprintf_s(text, _TRUNCATE, msg, argptr);
		va_end(argptr);

		Game::Com_PrintError(Game::CON_CHANNEL_PARSERSCRIPT, "%s: ", text);

		Scr_PrintPrevCodePos(Game::CON_CHANNEL_PARSERSCRIPT, codePos, 0);

		Game::Com_Printf(Game::CON_CHANNEL_PARSERSCRIPT, "************************************\n");

		Scr_GetTextSourcePos(scrParserPub.sourceBuf, codePos, line);

		Game::Com_Error(Game::ERR_SCRIPT_DROP, "\x15" "script compile error\n%s\n%s\n(see console for details)\n", text, line);
	}

	void ScriptError::Scr_GetTextSourcePos([[maybe_unused]] const char* buf, const char* codePos, char* line)
	{
		int col;

		if (developer_ && codePos && codePos != Game::g_EndPos && Game::scrVarPub->programBuffer && Scr_IsInOpcodeMemory(codePos))
		{
			auto bufferIndex = Scr_GetSourceBuffer(codePos - 1);
			Scr_GetLineInfo(scrParserPub.sourceBufferLookup[bufferIndex].sourceBuf, Scr_GetPrevSourcePos(codePos - 1, 0), &col, line, nullptr);
		}
		else
		{
			*line = '\0';
		}
	}

	void ScriptError::Scr_InitOpcodeLookup()
	{
		assert(!scrParserGlob.opcodeLookup);
		assert(!scrParserGlob.sourcePosLookup);
		assert(!scrParserPub.sourceBufferLookup);

		if (!developer_)
		{
			return;
		}

		scrParserGlob.delayedSourceIndex = -1;
		scrParserGlob.opcodeLookupMaxSize = 0;
		scrParserGlob.opcodeLookupLen = 0;
		scrParserGlob.opcodeLookup = static_cast<Game::OpcodeLookup*>(Game::Z_VirtualReserve(Game::MAX_OPCODE_LOOKUP_SIZE));

		scrParserGlob.sourcePosLookupMaxSize = 0;
		scrParserGlob.sourcePosLookupLen = 0;
		scrParserGlob.sourcePosLookup = static_cast<Game::SourceLookup*>(Game::Z_VirtualReserve(Game::MAX_SOURCEPOS_LOOKUP_SIZE));
		scrParserGlob.currentCodePos = nullptr;
		scrParserGlob.currentSourcePosCount = 0;
		scrParserGlob.sourceBufferLookupMaxSize = 0;

		scrParserPub.sourceBufferLookupLen = 0;
		scrParserPub.sourceBufferLookup = static_cast<Game::SourceBufferInfo*>(Game::Z_VirtualReserve(Game::MAX_SOURCEBUF_LOOKUP_SIZE));
	}

	void ScriptError::Scr_ShutdownOpcodeLookup()
	{
		if (scrParserGlob.opcodeLookup)
		{
			Z_VirtualFree(scrParserGlob.opcodeLookup);
			scrParserGlob.opcodeLookup = nullptr;
		}

		if (scrParserGlob.sourcePosLookup)
		{
			Z_VirtualFree(scrParserGlob.sourcePosLookup);
			scrParserGlob.sourcePosLookup = nullptr;
		}

		if (scrParserPub.sourceBufferLookup)
		{
			for (unsigned int i = 0; i < scrParserPub.sourceBufferLookupLen; ++i)
			{
				Hunk_FreeDebugMem(scrParserPub.sourceBufferLookup[i].buf);
			}

			Z_VirtualFree(scrParserPub.sourceBufferLookup);
			scrParserPub.sourceBufferLookup = nullptr;
		}

		if (scrParserGlob.saveSourceBufferLookup)
		{
			for (unsigned int i = 0; i < scrParserGlob.saveSourceBufferLookupLen; ++i)
			{
				if (scrParserGlob.saveSourceBufferLookup[i].sourceBuf)
				{
					Hunk_FreeDebugMem(scrParserGlob.saveSourceBufferLookup[i].buf);
				}
			}

			Hunk_FreeDebugMem(scrParserGlob.saveSourceBufferLookup);
			scrParserGlob.saveSourceBufferLookup = nullptr;
		}
	}

	__declspec(naked) void ScriptError::EmitThreadInternal_Stub()
	{
		__asm
		{
			pushad

			push [esp + 0x20 + 0x8] // sourcePos
			call AddThreadStartOpcodePos
			add esp, 0x4

			popad

			cmp ds:0x1CFEEF0, 2

			push 0x61A687
			ret
		}
	}

	Game::SourceBufferInfo* ScriptError::Scr_GetNewSourceBuffer()
	{
		assert(scrParserPub.sourceBufferLookup);

		auto size = sizeof(Game::SourceBufferInfo) * (scrParserPub.sourceBufferLookupLen + 1);
		if (size > scrParserGlob.sourceBufferLookupMaxSize)
		{
			if (scrParserGlob.sourceBufferLookupMaxSize >= Game::MAX_SOURCEBUF_LOOKUP_SIZE)
			{
				Game::Sys_Error("MAX_SOURCEBUF_LOOKUP_SIZE exceeded");
			}

			Game::Z_VirtualCommit((char*)scrParserPub.sourceBufferLookup + scrParserGlob.sourceBufferLookupMaxSize, 0x20000);
			scrParserGlob.sourceBufferLookupMaxSize += 0x20000;
			assert(size <= scrParserGlob.sourceBufferLookupMaxSize);
		}

		return &scrParserPub.sourceBufferLookup[scrParserPub.sourceBufferLookupLen++];
	}

	void ScriptError::Scr_AddSourceBufferInternal(const char* extFilename, const char* codePos, char* sourceBuf, int len, bool doEolFixup, bool archive)
	{
		int i;

		if (!scrParserPub.sourceBufferLookup)
		{
			scrParserPub.sourceBuf = nullptr;
			return;
		}

		assert((len >= -1));
		assert((len >= 0) || !sourceBuf);

		auto strLen = std::strlen(extFilename) + 1;
		auto newLen = strLen + len + 2;
		auto* buf = static_cast<char*>(Hunk_AllocDebugMem(static_cast<int>(newLen))); // Scr_AddSourceBufferInternal

		strcpy(buf, extFilename);
		auto* sourceBuf2 = sourceBuf ? buf + strLen : nullptr;
		auto* source = sourceBuf;
		auto* dest = sourceBuf2;

		if (doEolFixup)
		{
			for (i = 0; i <= len; ++i)
			{
				const auto c = *source++;
				if (c == '\n' || c == '\r' && *source != '\n')
				{
					*dest = 0;
				}
				else
				{
					*dest = c;
				}
				++dest;
			}
		}
		else
		{
			for (i = 0; i <= len; ++i)
			{
				const auto c = *source;
				++source;
				*dest = c;
				dest++;
			}
		}

		auto* bufferInfo = Scr_GetNewSourceBuffer();
		bufferInfo->codePos = codePos;
		bufferInfo->buf = buf;
		bufferInfo->sourceBuf = sourceBuf2;
		bufferInfo->len = len;
		bufferInfo->sortedIndex = -1;
		bufferInfo->archive = archive;

		if (sourceBuf2)
		{
			scrParserPub.sourceBuf = sourceBuf2;
		}
	}

	char* ScriptError::Scr_ReadFile_FastFile([[maybe_unused]] const char* filename, const char* extFilename, const char* codePos, bool archive)
	{
		auto* rawfile = Game::DB_FindXAssetHeader(Game::ASSET_TYPE_RAWFILE, extFilename).rawfile;
		if (Game::DB_IsXAssetDefault(Game::ASSET_TYPE_RAWFILE, extFilename))
		{
			Scr_AddSourceBufferInternal(extFilename, codePos, nullptr, -1, true, true);
			return nullptr;
		}

		const auto len = Game::DB_GetRawFileLen(rawfile);
		auto* sourceBuf = static_cast<char*>(Game::Hunk_AllocateTempMemoryHigh(len)); // Scr_ReadFile_FastFile
		Game::DB_GetRawBuffer(rawfile, sourceBuf, len);
		Scr_AddSourceBufferInternal(extFilename, codePos, sourceBuf, len, true, archive);
		return sourceBuf;
	}

	char* ScriptError::Scr_ReadFile_LoadObj([[maybe_unused]] const char* filename, const char* extFilename, const char* codePos, bool archive)
	{
		int f;

		auto len = Game::FS_FOpenFileByMode(extFilename, &f, Game::FS_READ);
		if (len < 0)
		{
			Scr_AddSourceBufferInternal(extFilename, codePos, nullptr, -1, true, archive);
			return nullptr;
		}

		*Game::g_loadedImpureScript = true;

		auto* sourceBuf = static_cast<char*>(Game::Hunk_AllocateTempMemoryHigh(len + 1));
		Game::FS_Read(sourceBuf, len, f);
		sourceBuf[len] = 0;

		Game::FS_FCloseFile(f);
		Scr_AddSourceBufferInternal(extFilename, codePos, sourceBuf, len, true, archive);

		return sourceBuf;
	}

	char* ScriptError::Scr_ReadFile(const char* filename, const char* extFilename, const char* codePos, bool archive)
	{
		int file;

		if (Game::FS_FOpenFileRead(extFilename, &file) < 0)
		{
			return Scr_ReadFile_FastFile(filename, extFilename, codePos, archive);
		}

		Game::FS_FCloseFile(file);
		return Scr_ReadFile_LoadObj(filename, extFilename, codePos, archive);
	}

	char* ScriptError::Scr_AddSourceBuffer(const char* filename, const char* extFilename, const char* codePos, bool archive)
	{
		char* sourceBuf;

		if (archive && scrParserGlob.saveSourceBufferLookup)
		{
			assert(scrParserGlob.saveSourceBufferLookupLen > 0);
			--scrParserGlob.saveSourceBufferLookupLen;

			auto* saveSourceBuffer = scrParserGlob.saveSourceBufferLookup + scrParserGlob.saveSourceBufferLookupLen;
			const auto len = saveSourceBuffer->len;
			assert(len >= -1);

			if (len < 0)
			{
				Scr_AddSourceBufferInternal(extFilename, codePos, nullptr, -1, true, archive);
				sourceBuf = nullptr;
			}
			else
			{
				sourceBuf = static_cast<char*>(Game::Hunk_AllocateTempMemoryHigh(len + 1));

				const char* source = saveSourceBuffer->sourceBuf;
				auto* dest = sourceBuf;
				for (int i = 0; i < len; ++i)
				{
					const auto c = *source++;
					*dest = c ? c : '\n';
					dest++;
				}

				*dest = '\0';
				Scr_AddSourceBufferInternal(extFilename, codePos, sourceBuf, len, false, archive);
			}

			return sourceBuf;
		}

		return Scr_ReadFile(filename, extFilename, codePos, archive);
	}

	unsigned int ScriptError::Scr_LoadScriptInternal_Hk(const char* filename, Game::PrecacheEntry* entries, int entriesCount)
	{
		char extFilename[MAX_QPATH];
		Game::sval_u parseData;

		assert(Game::scrCompilePub->script_loading);
		assert(std::strlen(filename) < MAX_QPATH);

		const auto name = Game::Scr_CreateCanonicalFilename(filename);

		if (Game::FindVariable(Game::scrCompilePub->loadedscripts, name))
		{
			Game::SL_RemoveRefToString(name);
			auto filePosPtr = Game::FindVariable(Game::scrCompilePub->scriptsPos, name);
			return filePosPtr ? Game::FindObject(Game::scrCompilePub->scriptsPos, filePosPtr) : 0;
		}

		const auto scriptId = Game::GetNewVariable(Game::scrCompilePub->loadedscripts, name);
		Game::SL_RemoveRefToString(name);

		sprintf_s(extFilename, "%s.gsc", Game::SL_ConvertToString(static_cast<unsigned short>(name)));

		const auto* oldSourceBuf = scrParserPub.sourceBuf;
		auto* sourceBuffer = Scr_AddSourceBuffer(Game::SL_ConvertToString(static_cast<unsigned short>(name)), extFilename, Game::TempMalloc(0), true);

		if (!sourceBuffer)
		{
			return 0;
		}

		const auto oldAnimTreeNames = Game::scrAnimPub->animTreeNames;
		Game::scrAnimPub->animTreeNames = 0;
		Game::scrCompilePub->far_function_count = 0;

		const auto* oldFilename = scrParserPub.scriptfilename;
		scrParserPub.scriptfilename = extFilename;

		Game::scrCompilePub->in_ptr = "+";
		Game::scrCompilePub->in_ptr_valid = false;
		Game::scrCompilePub->parseBuf = sourceBuffer;

		Game::ScriptParse(&parseData, 0);
		Game::scrCompilePub->parseBuf = nullptr;

		const auto filePosId = Game::GetObject(Game::scrCompilePub->scriptsPos, Game::GetVariable(Game::scrCompilePub->scriptsPos, name));
		const auto fileCountId = Game::GetObject(Game::scrCompilePub->scriptsCount, Game::GetVariable(Game::scrCompilePub->scriptsCount, name));

		Game::ScriptCompile(parseData.node, filePosId, fileCountId, scriptId, entries, entriesCount);

		Game::RemoveVariable(Game::scrCompilePub->scriptsCount, name);

		scrParserPub.scriptfilename = oldFilename;
		scrParserPub.sourceBuf = oldSourceBuf;

		Game::scrAnimPub->animTreeNames = oldAnimTreeNames;

		return filePosId;
	}

	void ScriptError::Scr_Settings_Hk([[maybe_unused]] int developer, int developer_script, int abort_on_error)
	{
		assert(!abort_on_error || developer);
		developer_ = (*Game::com_developer)->current.enabled;
		Game::scrVarPub->developer_script = developer_script != 0;
		Game::scrVmPub->abort_on_error = abort_on_error != 0;
	}

	void ScriptError::MT_Reset_Stub()
	{
		Utils::Hook::Call<void()>(0x4D9620)();

		Scr_InitOpcodeLookup();
		Hunk_InitDebugMemory();
	}

	void ScriptError::SL_ShutdownSystem_Stub(unsigned int user)
	{
		Utils::Hook::Call<void(unsigned int)>(0x4F46D0)(user);

		Scr_ShutdownOpcodeLookup();
		Hunk_ShutdownDebugMemory();
	}

	void ScriptError::Hunk_InitDebugMemory()
	{
		assert(Game::Sys_IsMainThread());
		assert(!g_debugUser);
		g_debugUser = Game::Hunk_UserCreate(0x1000000, "Hunk_InitDebugMemory", false, 0);
	}

	void ScriptError::Hunk_ShutdownDebugMemory()
	{
		assert(Game::Sys_IsMainThread());
		assert(g_debugUser);
		Game::Hunk_UserDestroy(g_debugUser);
		g_debugUser = nullptr;
	}

	void* ScriptError::Hunk_AllocDebugMem(int size)
	{
		assert(Game::Sys_IsMainThread());
		assert(g_debugUser);
		return Game::Hunk_UserAlloc(g_debugUser, size, 4);
	}

	void ScriptError::Hunk_FreeDebugMem([[maybe_unused]] void* ptr)
	{
		assert(Game::Sys_IsMainThread());
		assert(g_debugUser);

		// Let's hope it gets cleared by Hunk_ShutdownDebugMemory
	}

	ScriptError::ScriptError()
	{
#ifdef SCRIPT_ERROR_PATCH
		std::vector<std::pair<std::size_t, void*>> patches;
		const auto p = [&patches](const std::size_t a, void* b)
		{
			patches.emplace_back(a, b);
		};

		p(0x44AC44, Scr_Settings_Hk);
		p(0x60BE0B, Scr_Settings_Hk);

		p(0x4656A7, MT_Reset_Stub); // Scr_BeginLoadScripts
		p(0x42904B, SL_ShutdownSystem_Stub); // Scr_FreeScripts

		p(0x426C8A, Scr_LoadScriptInternal_Hk); // ScriptCompile
		p(0x45D959, Scr_LoadScriptInternal_Hk); // Scr_LoadScript

		p(0x61E3AD, RuntimeError); // VM_Notify
		p(0x621976, RuntimeError); // VM_Execute
		p(0x62246E, RuntimeError); // VM_Execute_0

		p(0x611F7C, CompileError2); // Scr_CheckAnimsDefined
		p(0x612E76, CompileError2); // LinkThread
		p(0x612E8D, CompileError2); // ^^
		p(0x612EA2, CompileError2); // ^^

		Utils::Hook(0x4227E0, Scr_PrintPrevCodePos, HOOK_JUMP).install()->quick();
		Utils::Hook(0x61A680, EmitThreadInternal_Stub, HOOK_JUMP).install()->quick();
		Utils::Hook::Nop(0x61A680 + 5, 2);

		p(0x61228C, AddOpcodePos); // EmitGetInteger
		p(0x6122C0, AddOpcodePos); // ^^
		p(0x6121FF, AddOpcodePos); // ^^
		p(0x612223, AddOpcodePos); // ^^
		p(0x612259, AddOpcodePos); // ^^
		p(0x6122ED, AddOpcodePos); // ^^

		p(0x6128BE, AddOpcodePos); // EmitValue

		p(0x6128F3, AddOpcodePos); // EmitGetFloat

		p(0x612702, AddOpcodePos); // EmitGetString

		p(0x612772, AddOpcodePos); // EmitGetIString

		p(0x6127E4, AddOpcodePos); // EmitGetVector

		p(0x612843, AddOpcodePos); // EmitGetAnimation

		p(0x6166DD, AddOpcodePos); // EmitPreFunctionCall

		p(0x617D10, AddOpcodePos); // EmitOrEvalVariableExpression

		p(0x615160, AddOpcodePos); // EmitOrEvalLocalVariable

		p(0x6155B3, AddOpcodePos); // EmitArrayVariable
		p(0x6155BF, AddOpcodePos); // ^^

		p(0x6170D7, AddOpcodePos); // EmitBoolAndExpression

		p(0x614971, AddOpcodePos); // ??

		p(0x6151B6, AddOpcodePos); // EmitLocalVariableRef

		p(0x619CC1, AddOpcodePos); // EmitArrayVariableRef
		p(0x619CC9, AddOpcodePos); // ^^

		p(0x615261, AddOpcodePos); // EmitGameRef

		p(0x615309, AddOpcodePos); // EmitClearArray
		Utils::Hook(0x615319, AddOpcodePos, HOOK_JUMP).install()->quick(); // EmitClearArray

		p(0x614D79, AddOpcodePos); // ??

		p(0x6153B9, AddOpcodePos); // ??

		p(0x614B74, AddOpcodePos); // ??

		p(0x614ED9, AddOpcodePos); // ??

		p(0x614E29, AddOpcodePos); // ??

		p(0x614CC9, AddOpcodePos); // ??

		p(0x614C19, AddOpcodePos); // ??

		p(0x6167A5, AddOpcodePos); // ??

		p(0x614AB1, AddOpcodePos); // ??

		p(0x614A11, AddOpcodePos); // ??

		p(0x616FB7, AddOpcodePos); // EmitBoolOrExpression

		p(0x6171EE, AddOpcodePos); // EmitOrEvalBinaryOperatorExpression

		p(0x616E6B, AddOpcodePos); // EmitOrEvalPrimitiveExpressionList

		p(0x618199, AddOpcodePos); // EmitReturnStatement

		p(0x61A2F2, AddOpcodePos); // EmitEndStatement

		p(0x61826A, AddOpcodePos); // EmitWaitStatement
		p(0x618272, AddOpcodePos); // ^^
		p(0x61827E, AddOpcodePos); // ^^

		p(0x61832E, RemoveOpcodePos); // EmitIfStatement (EmitOpcode inlined?)
		p(0x618366, AddOpcodePos); // ^^
		p(0x6183C8, AddOpcodePos); // ^^

		p(0x6184A6, RemoveOpcodePos); // EmitIfElseStatement (EmitOpcode inlined?)
		p(0x6184DD, AddOpcodePos); // ^^
		p(0x618601, AddOpcodePos); // ^^
		p(0x618569, AddOpcodePos); // ^^
		p(0x618685, AddOpcodePos); // ^^

		p(0x618876, RemoveOpcodePos); // EmitWhileStatement (EmitOpcode inlined?)
		p(0x6188AD, AddOpcodePos); // ^^
		p(0x6189F5, AddOpcodePos); // ^^
		p(0x618A09, AddOpcodePos); // ^^

		p(0x618CC2, RemoveOpcodePos); // EmitForStatement (EmitOpcode inlined?)
		p(0x618CF9, AddOpcodePos); // ^^
		p(0x618EAE, AddOpcodePos); // ^^
		p(0x618EC2, AddOpcodePos); // ^^

		p(0x61A0C7, AddOpcodePos); // EmitIncStatement
		p(0x61A0DA, AddOpcodePos); // ^^

		p(0x61A1A7, AddOpcodePos); // EmitDecStatement
		p(0x61A1BA, AddOpcodePos); // ^^

		p(0x61A239, AddOpcodePos); // EmitBinaryEqualsOperatorExpression
		p(0x61A253, AddOpcodePos); // ^^

		p(0x61909D, AddOpcodePos); // EmitWaittillStatement
		p(0x6190A5, AddOpcodePos); // ^^
		p(0x6190B1, AddOpcodePos); // ^^
		p(0x6190BE, AddOpcodePos); // ^^

		p(0x6148D0, AddOpcodePos); // EmitSafeSetWaittillVariableField

		p(0x6192B3, AddOpcodePos); // EmitWaittillmatchStatement
		p(0x6192BB, AddOpcodePos); // ^^
		p(0x6192C7, AddOpcodePos); // ^^
		p(0x6192D5, AddOpcodePos); // ^^
		p(0x6192EC, AddOpcodePos); // ^^

		p(0x617412, AddOpcodePos); // EmitWaittillFrameEnd
		p(0x61741A, AddOpcodePos); // ^^

		p(0x61944B, AddOpcodePos); // EmitNotifyStatement
		p(0x619551, AddOpcodePos); // ^^
		p(0x61955F, AddOpcodePos); // ^^
		p(0x61956B, AddOpcodePos); // ^^

		p(0x61965E, AddOpcodePos); // EmitEndOnStatement
		p(0x61966A, AddOpcodePos); // ^^

		p(0x619835, AddOpcodePos); // EmitSwitchStatement

		p(0x617860, AddOpcodePos); // EmitBreakStatement

		p(0x617990, AddOpcodePos); // EmitContinueStatement

		p(0x61A70D, AddOpcodePos); // EmitThreadInternal
		p(0x61A716, AddOpcodePos); // ^^

		p(0x617BB8, AddOpcodePos); // InitThread
		p(0x617BC0, AddOpcodePos); // ^^

		p(0x6157B7, AddOpcodePos); // EmitGetFunction
		p(0x615997, AddOpcodePos); // ^^
		p(0x6158D7, AddOpcodePos); // ^^

		p(0x616BBD, AddOpcodePos); // EmitMethod
		p(0x616C2E, AddOpcodePos); // ^^

		p(0x615A42, RemoveOpcodePos); // ?? (EmitOpcode inlined?)
		p(0x615AF7, RemoveOpcodePos); // ^^
		p(0x615B78, AddOpcodePos); // ^^

		p(0x615D0D, AddOpcodePos); // ??
		p(0x615D19, AddOpcodePos); // ^^

		p(0x6163FD, AddOpcodePos); // ??
		p(0x616420, AddOpcodePos); // ^^

		p(0x615E5B, RemoveOpcodePos); // ?? (EmitOpcode inlined?)
		p(0x615E93, AddOpcodePos); // ^^

		p(0x6161C5, AddOpcodePos); // ??

		p(0x61645D, AddOpcodePos); // ??
		p(0x616480, AddOpcodePos); // ^^

		p(0x615FEB, RemoveOpcodePos); // ?? (EmitOpcode inlined?)
		p(0x616023, AddOpcodePos); // ^^

		p(0x616365, AddOpcodePos); // ??

		p(0x616605, AddOpcodePos); // ??

		p(0x6167F2, AddOpcodePos); // EmitCallBuiltinMethodOpcode

		p(0x617C6F, AddOpcodePos); // EmitClearFieldVariable

		p(0x617482, AddOpcodePos); // EmitSafeSetVariableField

		p(0x617B61, AddOpcodePos); // EmitFormalParameterList

		p(0x6154F4, AddOpcodePos); // ??
		p(0x61551B, AddOpcodePos); // ^^
		p(0x61554D, AddOpcodePos); // ^^

		p(0x6150C1, AddOpcodePos); // EmitAnimObject

		p(0x615021, AddOpcodePos); // EmitLevelObject

		p(0x614F81, AddOpcodePos); // ??

		p(0x612CF2, AddOpcodePos); // EmitFunction

		p(0x619FFA, AddOpcodePos); // ??

		p(0x61407E, RemoveOpcodePos); // EmitOpcode
		p(0x61409F, RemoveOpcodePos); // ^^
		p(0x6140C0, RemoveOpcodePos); // ^^
		p(0x6140D7, RemoveOpcodePos); // ^^
		p(0x614164, RemoveOpcodePos); // ^^
		p(0x61417A, RemoveOpcodePos); // ^^
		p(0x61419D, RemoveOpcodePos); // ^^
		p(0x6141B4, RemoveOpcodePos); // ^^
		p(0x6141CE, RemoveOpcodePos); // ^^
		p(0x6141F9, RemoveOpcodePos); // ^^
		p(0x614214, RemoveOpcodePos); // ^^
		p(0x614277, RemoveOpcodePos); // ^^
		p(0x61428E, RemoveOpcodePos); // ^^
		p(0x6142CD, RemoveOpcodePos); // ^^

		for (const auto& patch : patches)
		{
			Utils::Hook(patch.first, patch.second, HOOK_CALL).install()->quick();
		}

		Utils::Hook(0x434260, CompileError, HOOK_JUMP).install()->quick();
#endif
	}
}
