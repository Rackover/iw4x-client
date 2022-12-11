#pragma once

// write logs for ZoneBuilder

#ifndef WRITE_LOGS // they take forever to run so only enable if needed
#define SaveLogEnter(x)
#define SaveLogExit()
#else
#define SaveLogEnter(x) builder->getBuffer()->enterStruct(x)
#define SaveLogExit() builder->getBuffer()->leaveStruct()
#endif

namespace Utils
{
	class Stream
	{
	private:
		bool ptrAssertion;
		std::vector<std::pair<const void*, std::size_t>> ptrList;

		int criticalSectionState;
		unsigned int blockSize[Game::MAX_XFILE_COUNT];
		std::vector<Game::XFILE_BLOCK_TYPES> streamStack;
		std::string buffer;

	public:
		class Reader
		{
		public:
			Reader(Memory::Allocator* _allocator, const std::string& _buffer) : position(0), buffer(_buffer), allocator(_allocator) {}

			std::string readString();
			const char* readCString();

			char readByte();

			void* read(std::size_t size, std::size_t count = 1);
			template <typename T> T* readObject()
			{
				return readArray<T>(1);
			}

			template <typename T> T* readArrayOnce(std::size_t count = 1)
			{
				constexpr char POINTER = 255;
				constexpr char FOLLOWING = 254;
				
				char b = readByte();	
				switch (b)
				{
				case POINTER:
				{
					auto ptr = read<int>();
					auto voidPtr = reinterpret_cast<void*>(ptr);

					if (allocator->isPointerMapped(voidPtr))
					{
						return allocator->getPointer<T>(voidPtr);
					}

					throw std::runtime_error("Bad data: missing ptr");
				}

				case FOLLOWING:
				{
					auto filePosition = position;
					auto data = readArray<T>(count);
					allocator->mapPointer(reinterpret_cast<void*>(filePosition), data);
					return data;
				}

				default:
					throw std::runtime_error("Bad data");
				}
			}

			template <typename T> T* readArray(std::size_t count = 1)
			{
				return static_cast<T*>(this->read(sizeof(T), count));
			}

			template <typename T> T read()
			{
				T obj;

				for (unsigned int i = 0; i < sizeof(T); ++i)
				{
					reinterpret_cast<char*>(&obj)[i] = this->readByte();
				}

				return obj;
			}

			bool end();
			void seek(unsigned int position);
			void seekRelative(unsigned int position);

			void* readPointer();
			void mapPointer(void* oldPointer, void* newPointer);
			bool hasPointer(void* pointer);

		private:
			unsigned int position;
			std::string buffer;
			std::map<void*, void*> pointerMap;
			Memory::Allocator* allocator;
		};

		enum Alignment
		{
			ALIGN_2,
			ALIGN_4,
			ALIGN_8,
			ALIGN_16,
			ALIGN_32,
			ALIGN_64,
			ALIGN_128,
			ALIGN_256,
			ALIGN_512,
			ALIGN_1024,
			ALIGN_2048,
		};

		Stream();
		Stream(size_t size);
		~Stream();

		[[nodiscard]] std::size_t length() const;
		[[nodiscard]] std::size_t capacity() const;

		char* save(const void * _str, std::size_t size, std::size_t count = 1);
		char* save(Game::XFILE_BLOCK_TYPES stream, const void * _str, std::size_t size, std::size_t count);
		char* save(Game::XFILE_BLOCK_TYPES stream, int value, std::size_t count);

		template <typename T> char* save(T* object)
		{
			return saveArray<T>(object, 1);
		}

		template <typename T> char* saveArray(T* array, std::size_t count)
		{
			return save(array, sizeof(T), count);
		}

		char* saveString(const std::string& string);
		char* saveString(const char* string);
		char* saveString(const char* string, std::size_t len);
		char* saveByte(unsigned char byte, std::size_t count = 1);
		char* saveNull(size_t count = 1);
		char* saveMax(size_t count = 1);

		char* saveText(const std::string& string);

		void align(Alignment align);
		bool pushBlock(Game::XFILE_BLOCK_TYPES stream);
		bool popBlock();
		bool isValidBlock(Game::XFILE_BLOCK_TYPES stream);
		void increaseBlockSize(Game::XFILE_BLOCK_TYPES stream, unsigned int size);
		void increaseBlockSize(unsigned int size);
		Game::XFILE_BLOCK_TYPES getCurrentBlock();
		unsigned int getBlockSize(Game::XFILE_BLOCK_TYPES stream);
		bool hasBlock();

		DWORD getPackedOffset();

		char* data();
		char* at();
		template <typename T> T* dest()
		{
			return reinterpret_cast<T*>(this->at());
		}

		template <typename T> static void ClearPointer(T** object)
		{
			*object = reinterpret_cast<T*>(-1);
		}

		void setPointerAssertion(bool value)
		{
			this->ptrAssertion = value;
		}
		void assertPointer(const void* pointer, std::size_t length);

		void toBuffer(std::string& outBuffer);
		std::string toBuffer();

		// Enter/Leave critical sections in which reallocations are not allowed.
		// If buffer reallocation is detected, the operation has to be terminated
		// and more memory has to be allocated next time. This will have to be done
		// by editing the code though.
		void enterCriticalSection();
		void leaveCriticalSection();
		bool isCriticalSection() const;

		// for recording zb writes
#ifdef WRITE_LOGS
		int structLevel;
		void enterStruct(const char* structName);
		void leaveStruct();
#endif

		// This represents packed offset in streams:
		// - lowest 28 bits store the value/offset
		// - highest 4 bits store the stream block
		class Offset
		{
		public:
			union
			{
				struct
				{
					uint32_t offset : 28;
					Game::XFILE_BLOCK_TYPES block : 4;
				};
				uint32_t packed;
				void* pointer;
			};

			Offset() : packed(0) {};
			Offset(Game::XFILE_BLOCK_TYPES _block, uint32_t _offset) : offset(_offset), block(_block) {};

			// The game needs it to be incremented
			uint32_t getPackedOffset()
			{
				return this->packed + 1;
			};

			uint32_t getUnpackedOffset()
			{
				Offset lOffset = *this;
				lOffset.packed--;
				return lOffset.offset;
			};

			int getUnpackedBlock()
			{
				Offset lOffset = *this;
				lOffset.packed--;
				return lOffset.block;
			};
		};
	};
}
