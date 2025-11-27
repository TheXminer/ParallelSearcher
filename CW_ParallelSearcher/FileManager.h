#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <map>
static class FileManager
{
private:
	static std::string storageDirectory;
	static uint64_t currentFileId;
	static std::mutex fileSaveMutex;

	static std::map<uint64_t, std::string> fileIndexMap;
public:
	FileManager() = delete;
	static uint64_t SaveFile(const std::string& fileName, const std::string& fileData);//returns saved file ID
	static const std::string& loadFile(uint64_t filePath);//returns file handle
};

