#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <iterator>

class Url
{
public:
	__declspec(dllexport)
		Url(const std::string &url);
	__declspec(dllexport)
		Url(const std::string &host, const std::string &resource);
	__declspec(dllexport)
		Url(const std::string &protocolPrefix, const std::string &host, const std::string &resource);
	template<class ...Args>
	Url(const std::string & host, const std::string & resource, const std::string &parameter, const Args &...parameters) :
		m_host(host),
		m_resource(resource)
	{
		parseUrl(host);
		parseParameters(parameters...);
	}
	__declspec(dllexport)
		~Url();

public:
	__declspec(dllexport)
		void setProtocolPrefix(const std::string &protocolPrefix);
	__declspec(dllexport)
		void setHost(const std::string &host);
	__declspec(dllexport)
		void setResource(const std::string &resource);
	__declspec(dllexport)
		void setProtocolPrefix(std::string &&protocolPrefix);
	__declspec(dllexport)
		void setHost(std::string &&host);
	__declspec(dllexport)
		void setResource(std::string &&resource);

public:
	__declspec(dllexport)
		operator std::string() const;
	__declspec(dllexport)
		Url &operator=(const std::string &other);

private:
	void parseUrl(const std::string &url);
	template<class ...Args>
	void parseParameters(const std::string &parameter, const Args &...parameters)
	{
		m_parameters.push_back(parameter);
		parseParameters(parameters...);
	}

private:
	std::string m_protocolPrefix;
	std::string m_host;
	std::string m_resource;
	char m_resourceParametersSeparator {'?'};
	std::vector<std::string> m_parameters;
};