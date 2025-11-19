#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <map>
class FileManager
{
private:
	std::string storageDirectory;
	uint64_t currentFileId;
	std::mutex fileSaveMutex;

	std::map<uint64_t, std::string> fileIndexMap;
public:
	FileManager();
	~FileManager();
	uint64_t saveFile(std::string fileName, FILE file);//returns path to saved file
	FILE loadFile(uint64_t filePath);//returns file handle
};

