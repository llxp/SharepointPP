#include "Authentication.h"

#include <sstream>
#include <chrono>
#include <type_traits>

#include "STSRequest.h"
#include "../common/tinyxml2.h"

#include "../common/WebRequest.h"
#include "../common/WebResponse.h"
#include "../common/TimeUtils.h"

using Microsoft::Sharepoint::Authentication;
using Microsoft::Sharepoint::SecurityDigest;
using Microsoft::Sharepoint::WebRequest;

Authentication::Authentication()
{
}

Authentication::Authentication(std::string && username, std::string && password, const std::string &endpoint)
{
	setSharepointEndpoint(endpoint);
	if (!login(std::move(username), std::move(password))) {
		throw std::exception("login error");
	}
}


Authentication::~Authentication()
{
}

void Authentication::setSharepointEndpoint(const std::string & sharepointEndpoint)
{
	if (sharepointEndpoint.length() > 0) {
		m_endpoint = sharepointEndpoint;
		if (m_endpoint[m_endpoint.length() - 1] == '/') {
			m_endpoint = m_endpoint.substr(0, m_endpoint.length() - 1);
		}
		if (m_endpoint.substr(0, std::string("http://").length()) != std::string("http://") && m_endpoint.substr(0, std::string("https://").length()) != std::string("https://")) {
			m_endpoint = "https://" + m_endpoint;
		}
		m_contextInfoUrl = m_endpoint + "/_api/contextinfo";
		m_defaultLoginPage = m_endpoint + "/_forms/default.aspx?wa=wsignin1.0";
	}
}

void Authentication::setSTSEndpoint(const std::string & stsEndpoint)
{
	m_stsEndpoint = stsEndpoint;
}

void Authentication::setContextInfoUrl(const std::string && contextInfoUrl)
{
	m_contextInfoUrl = contextInfoUrl;
}

bool Authentication::authenticate(std::string && username, std::string && password)
{
	return login(std::move(username), std::move(password));
}

bool Authentication::tokenIsValid() const
{
	return (&m_requestDigest)->isValid();
}

SecurityDigest Authentication::getRequestDigest() const
{
	return m_requestDigest;
}

WebRequest::CookieContainerType Authentication::getSecurityCookies() const
{
	return m_securityCookies;
}

WebRequest Authentication::getPreparedRequest() const
{
	WebRequest newRequest;
	newRequest.addHeader("X-RequestDigest", m_requestDigest.value());
	newRequest.addHeader("accept", "application/xml;odata=verbose");
	newRequest.setCookies(m_securityCookies);
	return newRequest;
}

bool Authentication::login(std::string && username, std::string && password)
{
	// prepare the soap xml request
	std::string preparedSoapRequest = getPreparedSoapData(std::move(username), std::move(password), m_endpoint);
	if (preparedSoapRequest.length() > 0) {
		// send the soap request to the sts server
		WebRequest stsRequest;
		stsRequest.setContentType("application/xml");
		WebResponse stsResponse = stsRequest.post(m_stsEndpoint, std::move(preparedSoapRequest));
		if (stsResponse.httpStatusCode() >= 0) {
			// got the response from the sts server
			std::string responseData = stsResponse.response();
			if (responseData.length() > 0) {
				// parse the response from the sts server to get the binary security token
				std::string securityCode = parseSTSResponse(std::move(responseData), m_endpoint);
				if (securityCode.length() > 0) {
					// send the security token to the default login page of the sharepoint server
					WebRequest loginPageRequest;
					WebResponse loginPageResponse = loginPageRequest.post(m_defaultLoginPage, std::move(securityCode));
					if (loginPageResponse.httpStatusCode() >= 0) {
						// got both important cookies from the default login page of the sharepoint server
						m_securityCookies = loginPageResponse.cookies();
						if (m_securityCookies.size() > 0) {
							WebRequest contextInfoRequest;
							contextInfoRequest.setContentType("application/x-www-form-urlencoded");
							contextInfoRequest.setCookies(m_securityCookies);
							WebResponse contextInfoResponse = contextInfoRequest.post(m_contextInfoUrl, "");
							if (contextInfoResponse.httpStatusCode() >= 0) {
								// got the request digest from the sharepoint server
								responseData = contextInfoResponse.response();
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
						return WebRequest::getUnescapedString(binarySecurityToken->GetText());
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
		if (securityDigestValue.length() > 0 && timeout > 0) {
			SecurityDigest newSecurityDigest(securityDigestValue, timeout);
			m_requestDigest = newSecurityDigest;
		}
	}
}