#include "Authentication.h"

#include <sstream>
#include <chrono>
#include <type_traits>

#include "STSRequest.h"
#include "tinyxml2.h"

#include "../RestUtils.h"
#include "../TimeUtils.h"

using Microsoft::Sharepoint::Authentication;
using Microsoft::Sharepoint::SecurityDigest;
using Microsoft::Sharepoint::RestUtils;

Authentication::Authentication()
{
}

Authentication::Authentication(std::string && username, std::string && password)
{
	if (!login(std::move(username), std::move(password))) {
		throw std::exception("login error");
	}
}


Authentication::~Authentication()
{
}

void Microsoft::Sharepoint::Authentication::setSharepointEndpoint(const std::string & sharepointEndpoint)
{
	m_endpoint = sharepointEndpoint;
	if (m_endpoint[m_endpoint.length() - 1] == '/') {
		m_endpoint = m_endpoint.substr(0, m_endpoint.length() - 1);
	}
	m_contextInfoUrl = m_endpoint + "/_api/contextinfo";
}

void Microsoft::Sharepoint::Authentication::setSTSEndpoint(const std::string & stsEndpoint)
{
	m_stsEndpoint = stsEndpoint;
}

void Microsoft::Sharepoint::Authentication::setContextInfoUrl(const std::string && contextInfoUrl)
{
	m_contextInfoUrl = contextInfoUrl;
}

bool Authentication::authenticate(std::string && username, std::string && password)
{
	return login(std::move(username), std::move(password));
}

bool Authentication::tokenIsValid() const
{
	return (&m_securityDigest)->isValid();
}

SecurityDigest Authentication::getSecurityDigest()
{
	return m_securityDigest;
}

bool Authentication::login(std::string && username, std::string && password)
{
	std::string preparedSoapRequest = getPreparedSoapData(std::move(username), std::move(password), m_endpoint);
	if (preparedSoapRequest.length() > 0) {
		if (RestUtils::sendPostRequest(m_stsEndpoint, std::move(preparedSoapRequest), "application/xml")) {
			// got binary token form sts server
			std::string responseData = RestUtils::getResponseData();
			if (responseData.length() > 0) {
				std::string securityCode = parseSTSResponse(std::move(responseData), m_endpoint);
				if (securityCode.length() > 0) {
					if (RestUtils::sendPostRequest(m_endpoint + "/_forms/default.aspx?wa=wsignin1.0", securityCode, "")) {
						// got both cookies from the default login page of the sharepoint server
						responseData = RestUtils::getResponseData();
						if (responseData.length() > 0) {
							RestUtils::cookieType cookies = RestUtils::getCookiesAfterRequest();
							if (cookies.size() > 0) {
								if (RestUtils::sendPostRequest(m_contextInfoUrl, "", "application/x-www-form-urlencoded", cookies)) {
									// got the request digest from the sharepoint server
									responseData = RestUtils::getResponseData();
									parseContextInfoResponse(std::move(responseData));
									if (tokenIsValid()) {
										return true;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return false;
}

std::string Authentication::getPreparedSoapData(std::string && username, std::string && password, const std::string & endpoint)
{
	tinyxml2::XMLDocument doc;
	//doc.LoadFile("STSRequest.xml");
	doc.Parse(STSRequest::Value);
	tinyxml2::XMLElement *envelope = doc.FirstChildElement("s:Envelope");
	if(envelope != nullptr) {
		tinyxml2::XMLElement *header =  envelope->FirstChildElement("s:Header");
		if(header != nullptr) {
			tinyxml2::XMLElement *security = header->FirstChildElement("o:Security");
			if(security != nullptr) {
				tinyxml2::XMLElement *usernameToken = security->FirstChildElement("o:UsernameToken");
				if(usernameToken != nullptr) {
					tinyxml2::XMLElement *usernameElement = usernameToken->FirstChildElement("o:Username");
					tinyxml2::XMLElement *passwordElement = usernameToken->FirstChildElement("o:Password");
					if(usernameElement != nullptr && passwordElement != nullptr) {
						usernameElement->SetText(username.data());
						passwordElement->SetText(password.data());
					} else {
						printf("no username/password element found...\n");
						return std::string();
					}
				} else {
					printf("no usernameToken element found...\n");
					return std::string();
				}
			} else {
				printf("no security element found...\n");
				return std::string();
			}
		} else {
			printf("no header element found...\n");
			return std::string();
		}
		tinyxml2::XMLElement *body = envelope->FirstChildElement("s:Body");
		if(body != nullptr) {
			tinyxml2::XMLElement *requestSecurityToken = body->FirstChildElement("t:RequestSecurityToken");
			if(requestSecurityToken != nullptr) {
				tinyxml2::XMLElement *appliesTo = requestSecurityToken->FirstChildElement("wsp:AppliesTo");
				if(appliesTo != nullptr) {
					tinyxml2::XMLElement *endpointReference = appliesTo->FirstChildElement("a:EndpointReference");
					if(endpointReference != nullptr) {
						tinyxml2::XMLElement *address = endpointReference->FirstChildElement("a:Address");
						if(address != nullptr) {
							address->SetText(endpoint.data());
						} else {
							return std::string();
						}
					} else {
						return std::string();
					}
				} else {
					return std::string();
				}
			} else {
				return std::string();
			}
		} else {
			return std::string();
		}
	} else {
		return std::string();
	}
	tinyxml2::XMLPrinter printer;
	doc.Print(&printer);
	return std::string(printer.CStr(), printer.CStrSize());
}

std::string Authentication::parseSTSResponse(std::string && responseXml, const std::string & endpoint)
{
	tinyxml2::XMLDocument doc;
	doc.Parse(responseXml.data());
	tinyxml2::XMLElement *envelope = doc.FirstChildElement("S:Envelope");
	if(envelope != nullptr) {
		tinyxml2::XMLElement *body = envelope->FirstChildElement("S:Body");
		if(body != nullptr) {
			tinyxml2::XMLElement *requestSecurityTokenResponse = body->FirstChildElement("wst:RequestSecurityTokenResponse");
			if(requestSecurityTokenResponse != nullptr) {
				tinyxml2::XMLElement *appliesTo = requestSecurityTokenResponse->FirstChildElement("wsp:AppliesTo");
				if(appliesTo != nullptr) {
					tinyxml2::XMLElement *endpointReference = appliesTo->FirstChildElement("wsa:EndpointReference");
					if(endpointReference != nullptr) {
						tinyxml2::XMLElement *address = endpointReference->FirstChildElement("wsa:Address");
						if(address != nullptr) {
							if(strcmp(address->GetText(), endpoint.data()) != 0) {
								return std::string();
							}
						}
					}
				}
				tinyxml2::XMLElement *requestedSecurityToken = requestSecurityTokenResponse->FirstChildElement("wst:RequestedSecurityToken");
				if(requestedSecurityToken != nullptr) {
					tinyxml2::XMLElement *binarySecurityToken = requestedSecurityToken->FirstChildElement("wsse:BinarySecurityToken");
					if(binarySecurityToken != nullptr) {
						return RestUtils::getUnescapedString(binarySecurityToken->GetText());
					}
				}
			}
		}
	}
	return std::string();
}

void Authentication::parseContextInfoResponse(std::string && responseXml)
{
	tinyxml2::XMLDocument doc;
	doc.Parse(responseXml.data());
	tinyxml2::XMLElement *getContextWebInformation = doc.FirstChildElement("d:GetContextWebInformation");
	if (getContextWebInformation != nullptr) {
		tinyxml2::XMLElement *formDigestTimeoutSeconds = getContextWebInformation->FirstChildElement("d:FormDigestTimeoutSeconds");
		size_t timeout = 0;
		std::string securityDigestValue;
		if (formDigestTimeoutSeconds != nullptr) {
			std::stringstream ss(formDigestTimeoutSeconds->GetText());
			ss >> timeout;
		}
		tinyxml2::XMLElement *formDigestValue = getContextWebInformation->FirstChildElement("d:FormDigestValue");
		if (formDigestValue != nullptr) {
			securityDigestValue = std::string(formDigestValue->GetText());
		}
		if (securityDigestValue.length() > 0, timeout > 0) {
			SecurityDigest newSecurityDigest(securityDigestValue, timeout);
			m_securityDigest = newSecurityDigest;
		}
	}
}