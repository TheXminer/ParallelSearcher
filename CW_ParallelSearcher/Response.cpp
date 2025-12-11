#include "Response.h"

Response::Response(Type type, std::string body, std::string contentType) 
    : type(type), body(std::move(body)), contentType(std::move(contentType)) {}

Response Response::Ok()
{
    return Response(Type::Ok, "", "");
}

Response Response::Ok(const std::string& body)
{
    return Response(Type::Ok, body);
}

Response Response::BadRequest(const std::string& msg)
{
    return Response(Type::BadRequest, msg);
}

Response Response::NotFound(const std::string& msg)
{
    return Response(Type::NotFound, msg);
}

Response Response::InternalServerError(const std::string& msg)
{
    return Response(Type::InternalError, msg);
}

std::string Response::toHttpString() const
{
    const auto& map = statusText();
    std::string status = map.at(type);

    std::stringstream ss;

    ss << "HTTP/1.1 " << static_cast<int>(type) << " " << status << "\r\n";
    if(!contentType.empty()){
        ss << "Content-Type: " << contentType << "\r\n";
    }
    ss << "Access-Control-Allow-Origin: *\r\n";
    ss << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    ss << "Access-Control-Allow-Headers: Content-Type\r\n";
    ss << "Connection: close\r\n";

    ss << "Content-Length: " << body.size() << "\r\n";
    ss << "\r\n";
    ss << body;

    return ss.str();
}
