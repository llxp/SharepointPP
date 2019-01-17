// MIT License
//
// Copyright (c) 2018 Lukas Luedke
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "Url.h"

#include <regex>


Url::Url(const std::string &url)
{
	parseUrl(url);
}

Url::Url(const std::string & host, const std::string & resource) :
	m_host(host),
	m_resource(resource)
{
	parseUrl(host);
}

Url::Url(
	const std::string & protocolPrefix,
	const std::string & host,
	const std::string & resource) :
	m_protocolPrefix(protocolPrefix),
	m_host(host),
	m_resource(resource)
{
}

void Url::parseUrl(const std::string & url)
{
	try {
		std::string regexPattern("([\\w-.\\:]+\\/\\/)?([\\w.-]+)(.*)");
		std::regex regex(std::move(regexPattern), std::regex::flag_type::ECMAScript);
		std::sregex_iterator itHost(url.begin(), url.end(), regex);
		if (itHost != std::sregex_iterator()) {
			{
				std::string protocol(std::move(itHost->str(1)));
				if (protocol.length() > 0) {
					m_protocolPrefix = protocol;
				}
			}
			{
				std::string host(std::move(itHost->str(2)));
				if (host.length() > 0) {
					m_host = host;
				}
			}
			std::string resource(std::move(itHost->str(3)));
			if (resource.length() > 0) {
				std::string regexPatternResource("([\\w\\/.&]*)([^\\w^\\/])(.*)");
				std::regex regexResource(std::move(regexPatternResource), std::regex::flag_type::ECMAScript);
				std::sregex_iterator itResource(resource.begin(), resource.end(), regexResource);
				if (itResource != std::sregex_iterator()) {
					m_resource = itResource->str(1);
					if (itResource->str(2).length() == 1) {
						m_resourceParametersSeparator = itResource->str(2)[0];
					}
					if (itResource->str(3).length() > 0) {
						std::string parameters(std::move(itResource->str(3)));
						if (parameters.length() > 0) {
							std::string regexPatternParameters(
								"(?:((?:[\\w^=\\+\\%\\\\\\.\\-]+)"
								"([^\\w^\\&^\\+^\\\\^\\-^\\.])"
								"(?:[\\w^=\\+\\%\\\\\\.\\-]+))+\\&?)");
							std::regex regexParameters(std::move(regexPatternParameters), std::regex::flag_type::ECMAScript);
							for (std::sregex_iterator itParameters(parameters.begin(), parameters.end(), regexParameters);
								itParameters != std::sregex_iterator();
								++itParameters) {
								std::string parameterString(std::move(itParameters->str(1)));
								if (parameterString.length() > 0) {
									m_parameters.push_back(parameterString);
								}
							}
						}
					}
				}
			}
		}
	} catch (...) {}
}


Url::~Url()
{
}

void Url::setProtocolPrefix(const std::string & protocolPrefix)
{
	m_protocolPrefix = protocolPrefix;
}

void Url::setHost(const std::string & host)
{
	m_host = host;
}

void Url::setResource(const std::string & resource)
{
	m_resource = resource;
}

void Url::setProtocolPrefix(std::string && protocolPrefix)
{
	m_protocolPrefix = std::move(protocolPrefix);
}

void Url::setHost(std::string && host)
{
	m_host = std::move(host);
}

void Url::setResource(std::string && resource)
{
	m_resource = std::move(resource);
}

Url::operator std::string() const
{
	std::ostringstream parameters;
	std::copy(m_parameters.begin(), m_parameters.end(), std::ostream_iterator<std::string>(parameters, "&"));
	std::string parametersString(parameters.str());
	if(parametersString[parametersString.length() - 1] == '&') {
		parametersString = parametersString.substr(0, parametersString.length() - 1);
	}
	return m_protocolPrefix + m_host + m_resource + parametersString;
}

Url & Url::operator=(const std::string & other)
{
	parseUrl(other);
	return *this;
}