#include "Controller.h"
#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

Controller::Controller(std::shared_ptr<ThreadPool> threadPool)
	: threadPool(threadPool), searcher(this->threadPool) {
	routeHandlers["POST /addfile"] =
		[this](const std::string& req) {
		return this->handleAddFile(req);
		};

	routeHandlers["GET /search"] =
		[this](const std::string& req) {
		return this->handleSearchPhrase(req);
		};

	routeHandlers["GET /file"] =
		[this](const std::string& req) {
		return this->handleGetFile(req);
		};
}

Response Controller::handleAddFile(const std::string& request)
{
	std::string fileName = getParam(request, "fileName");
	if(fileName.empty()) {
		return Response::BadRequest("Missing 'fileName' parameter");
	}

	std::string fileData = getParam(request, "fileData");
	if(fileData.empty()) {
		return Response::BadRequest("Missing 'fileData' parameter");
	}

	uint32_t fileID = FileManager::SaveFile(fileName, fileData);
	if(fileID == 0) {
		return Response::InternalServerError("Failed to save file");
	}
	searcher.AddFile(fileID);
	return Response::Ok("File will be added soon!");
}

Response Controller::handleSearchPhrase(const std::string& request)
{
	auto phrase = getParam(request, "phrase");
	if (phrase.empty()) {
		return Response::BadRequest("Missing 'phrase' parameter");
	}

	auto results = searcher.SearchPhrase(phrase);
	return Response::Ok(JSONifySearchResults(results));
}

Response Controller::handleGetFile(const std::string& request)
{
	uint32_t fileId;
	try{
		fileId = std::stoi(getParam(request, "id"));
	}
	catch(...)
	{
		return Response::BadRequest("Invalid 'id' parameter");
	}

	auto fileContent = FileManager::loadFile(fileId);
	return Response::Ok(fileContent);
}

void Controller::handleClient(int clientSocket)
{
	std::string request = getRequest(clientSocket);
	if (request.empty()) {
		sendResponse(clientSocket, Response::BadRequest("Empty request"));
	}

	std::string path = getRequestInfo(request);
	if(path.empty()) {
		sendResponse(clientSocket, Response::BadRequest("Incorrect path"));
		return;
	}
	
	auto handlerIt = routeHandlers.find(path);
	if (handlerIt != routeHandlers.end()) {
		Response response = handlerIt->second(request);
		sendResponse(clientSocket, response);
	} else {
		sendResponse(clientSocket, Response::BadRequest("Path not found"));
	}
}

std::string Controller::getRequest(int clientSocket)
{
	std::string request;
	char buffer[4096];

	while (true) {
		size_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

		if (bytesReceived <= 0) {
			return std::string();
		}

		request.append(buffer, bytesReceived);

		if (request.find("\r\n\r\n") != std::string::npos) {
			break;
		}
	}

	return request;
}

void Controller::sendResponse(int clientSocket, Response response)
{
	std::string responseString = response.toHttpString();
	send(clientSocket, responseString.c_str(), responseString.size(), 0);
}

std::string Controller::getRequestInfo(const std::string& req)
{
	auto first_space_pos = req.find(' ');
	if (first_space_pos == std::string::npos)
		return std::string();
	auto second_space_pos = req.find(' ', first_space_pos + 1);
	if (second_space_pos == std::string::npos)
		return std::string();
	std::string path = req.substr(0, second_space_pos);
	return path;
}

std::string Controller::JSONifySearchResults(const std::vector<std::pair<std::string, size_t>>& results)
{
	return std::string();
}

std::string Controller::getParam(const std::string& req, const std::string& key)
{
	auto pos = req.find(key + "=");
	if (pos == std::string::npos) return "";

	pos += key.size() + 1;
	auto end = req.find('&', pos);
	if (end == std::string::npos) end = req.size();

	return req.substr(pos, end - pos);
}