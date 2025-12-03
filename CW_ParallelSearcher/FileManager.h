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

	static std::map<uint64_t, std::string> fileIndexMap;//maps file ID to file path
public:
	FileManager() = delete;
	static uint64_t SaveFile(const std::string& fileName, const std::string& fileData);//returns saved file ID
	static const std::string& loadFile(uint64_t filePath);//returns file handle
	static std::string& getFileText(uint64_t fileId);
	static std::string& getFileName(uint64_t fileId);
	static void Initialize(const std::string& storageDir);
	static std::vector<uint64_t>& GetAllFileIds();
};

