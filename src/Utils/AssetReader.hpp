#pragma once

namespace Utils::ZoneTool
{
	enum DUMP_TYPE
	{
		DUMP_TYPE_INT = 0,
		DUMP_TYPE_STRING = 1,
		DUMP_TYPE_ASSET = 2,
		DUMP_TYPE_ARRAY = 3,
		DUMP_TYPE_OFFSET = 4,
		DUMP_TYPE_FLOAT = 5,
	};

	const char DUMP_EXISTING = 1;
	const char DUMP_NONEXISTING = 0;

	class AssetReader
	{
	protected:
		std::string buffer;
		unsigned int position;
		Utils::Memory::Allocator* allocator;

		template <typename T>
		T read_internal()
		{
			T obj;

			for (unsigned int i = 0; i < sizeof(T); ++i)
			{
				reinterpret_cast<char*>(&obj)[i] = this->read_byte();
			}

			return obj;
		}

		char* read_string_internal()
		{
			std::string str;

			while (char byte = this->read_byte())
			{
				str.push_back(byte);
			}

			return this->allocator->duplicateString(str);
		}

		void* sr_read(size_t size, size_t count)
		{
			size_t bytes = size * count;

			if ((this->position + bytes) <= this->buffer.size())
			{
				void* _buffer = this->allocator->allocate(bytes);

				std::memcpy(_buffer, this->buffer.data() + this->position, bytes);
				this->position += bytes;

				return _buffer;
			}

			throw std::runtime_error("Reading past the buffer");
		}

	public:
		AssetReader(const std::string& _buffer, Utils::Memory::Allocator* _allocator) : position(0), buffer(_buffer), allocator(_allocator)
		{
			clear();
		}

		~AssetReader()
		{
		}


		char read_byte()
		{
			if ((this->position + 1) <= this->buffer.size())
			{
				return this->buffer[this->position++];
			}

			throw std::runtime_error("Reading past the buffer");
		}

		float read_float()
		{
			char type = this->read_internal<char>();
			if (type != DUMP_TYPE_FLOAT)
			{
				printf("Reader error: Type not DUMP_TYPE_FLOAT but %i", type);
				throw;
				return 0;
			}
			return this->read_internal<float>();
		}

		int read_int()
		{
			const auto type = this->read_internal<char>();
			if (type != DUMP_TYPE_INT)
			{
				printf("Reader error: Type not DUMP_TYPE_INT but %i", type);
				throw;
				return 0;
			}
			return this->read_internal<std::int32_t>();
		}

		unsigned int read_uint()
		{
			const auto type = this->read_internal<char>();
			if (type != DUMP_TYPE_INT)
			{
				printf("Reader error: Type not DUMP_TYPE_INT but %i", type);
				throw;
				return 0;
			}
			return this->read_internal<std::uint32_t>();
		}

		char* read_string()
		{
			const auto type = this->read_internal<char>();

			if (type == DUMP_TYPE_STRING)
			{
				const auto existing = this->read_internal<char>();
				if (existing == DUMP_NONEXISTING)
				{
					return nullptr;
				}
				const auto output = this->read_string_internal(); // freadstr(fp_);

				return output;
			}

			printf("Reader error: Type not DUMP_TYPE_STRING or DUMP_TYPE_OFFSET but %i", type);
			throw;
			return nullptr;
		}

		template <typename T>
		T* read_asset()
		{
			const auto type = this->read_internal<char>();

			if (type == DUMP_TYPE_ASSET)
			{
				const auto existing = this->read_internal<char>();
				if (existing == DUMP_NONEXISTING)
				{
					return nullptr;
				}
				const char* name = this->read_string_internal();
				// T* asset = new T;

				T asset = allocator->allocate<T>();

				memset(asset, 0, sizeof(T));
				asset->name = const_cast<char*>(name);

				return asset;
			}

			printf("Reader error: Type not DUMP_TYPE_ASSET or DUMP_TYPE_OFFSET but %i", type);
			throw;
		}

		template <typename T>
		T* read_array()
		{
			const auto type = this->read_internal<char>();

			if (type == DUMP_TYPE_ARRAY)
			{
				const auto arraySize = this->read_internal<std::uint32_t>();

				if (arraySize <= 0)
				{
					return nullptr;
				}

				if ((this->position + arraySize) <= this->buffer.size())
				{
					void* nArray = this->allocator->allocate(arraySize);

					std::memcpy(nArray, this->buffer.data() + this->position, arraySize);
					this->position += arraySize;

					return nArray;
				}
			}

			printf("Reader error: Type not DUMP_TYPE_ARRAY or DUMP_TYPE_OFFSET but %i\n", type);
			throw;
		}

		template <typename T> T* read_single()
		{
			return this->read_array<T>();
		}
	};
}
