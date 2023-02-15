#pragma once

#include <fstream>
#include <map>
#include <string>

#include <ge/utils/FileLoading.hpp>
#include <ge/utils/Log.hpp>
#include <ge/utils/GuardedType.hpp>

namespace GE
{
	namespace Utils
	{
		struct EngineResourceDataPoint
		{
			uint32_t startPoint = 0;
			uint32_t size = 0;
		};

		class EngineResourceParser
		{
		public:
			static EngineResourceParser& Get() {
				static EngineResourceParser parser;
				if (!parser._created.Get())	{
					parser.Create("EngineData.blob");
				}
				return parser;
			}

			static EngineResourceDataPoint GetDataPoint(const std::string& value)
			{
				if (Get()._data.count(value) > 0)
				{
					return Get()._data[value];
				}
				return EngineResourceDataPoint();
			}

		private:
			EngineResourceParser() {};

			inline void Create(const char* path)
			{

				std::vector<char> result = LoadFile(path);
				char* data = result.data();

				uint32_t byteRead = 0;
				while (byteRead < result.size())
				{
					uint32_t fileNameLength = 0;
					memcpy(&fileNameLength, (data + byteRead), sizeof(fileNameLength));
					byteRead += sizeof(fileNameLength);

					std::string fileName;
					fileName.resize(fileNameLength);
					memcpy(fileName.data(), (data + byteRead), fileNameLength);
					byteRead += fileNameLength;

					uint32_t fileLength = 0;
					memcpy(&fileLength, (data + byteRead), sizeof(fileLength));

					byteRead += sizeof(fileLength);
					_data[fileName] = { byteRead , fileLength };

					byteRead += fileLength;
				}
			}

		private:
			std::map<std::string, EngineResourceDataPoint> _data;
			Guarded<bool> _created{ false };
		};
	}
}