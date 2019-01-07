#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include "../SharepointPP/authentication/tinyxml2.h"

#include "../SharepointPP/authentication/Authentication.h"

//#include "SharepointAuthentication.h"
#include "../SharepointPP/common/ConsoleUtil.h"

using namespace tinyxml2;

int main(int argc, char *argv[]) {
    const char *endpoint = "https://microsoft.sharepoint.com";
	//const char *stsEndpoint = "https://login.microsoftonline.com/extSTS.srf";
	//const char *defaultLoginPage = "https://microsoft.sharepoint.com/_forms/default.aspx?wa=wsignin1.0";
	//const char *contextInfoUrl = "https://microsoft.sharepoint.com/_api/contextinfo";
	std::string username;
	std::string password;
	std::cout << "please enter your username: ";
	std::cin >> username;
	SetStdinEcho(false);
	std::cout << "please enter your password: ";
	std::cin >> password;
	SetStdinEcho(true);
	std::cout << std::endl;
	std::cout << "password read in..." << std::endl;
	Microsoft::Sharepoint::Authentication authentication;
	authentication.setSharepointEndpoint(endpoint);
	//authentication.setContextInfoUrl(contextInfoUrl);
	bool authenticated = authentication.authenticate(std::move(username), std::move(password));
	if (authenticated) {
		std::cout << "authenticated..." << std::endl;
		std::cout << authentication.getSecurityDigest().isValid() << std::endl;
		std::cout << authentication.getSecurityDigest().value() << std::endl;
	}
	/*XMLDocument *doc = prepareSoapRequest(username.data(), password.data(), endpoint);
	if (doc != nullptr) {
		std::cout << "no error..." << std::endl;
		XMLPrinter printer;
		doc->Print(&printer);
		std::cout << printer.CStr() << std::endl;
		if (sendPostRequest(std::string(stsEndpoint), printer.CStr(), "Accepts: \"application/json; odata=verbose\"")) {
			std::cout << "post request successful..." << std::endl;
			std::string responseData = getPostData();
			if (responseData.length() > 0) {
				std::string securityCode = parseResponse(responseData, endpoint);
				std::cout << "code: " << securityCode << std::endl;
				if (securityCode.length() > 0 && sendPostRequest(defaultLoginPage, securityCode, "")) {
					std::cout << "post request successful..." << std::endl;
					responseData = getPostData();
					if (responseData.length() > 0) {
						std::cout << "responseData: " << responseData << std::endl;
					}
					std::vector<std::pair<std::string, std::string>> cookies = getResponseCookies();
					std::cout << "foundHeaderCount: " << cookies.size() << std::endl;
					if (cookies.size() > 0) {
						for (std::vector<std::pair<std::string, std::string>>::const_iterator it = cookies.begin(); it != cookies.end(); ++it) {
							std::cout << "cookie: " << it->first << " = " << it->second << std::endl;
						}
						if (sendPostRequest(contextInfoUrl, "", "Content-Type:application/x-www-form-urlencoded", cookies)) {
							std::cout << "post request successful..." << std::endl;
							responseData = getPostData();
							std::cout << "responseData: " << responseData << std::endl;
						} else {
							std::cout << "error sending post request..." << std::endl;
						}
					}
				} else {
					std::cout << "error sending post request..." << std::endl;
				}
			} else {
				std::cout << "error..." << std::endl;
			}
		} else {
			std::cout << "error" << std::endl;
		}
	}
    delete doc;*/
    return 0;
}