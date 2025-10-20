/*
 * Copyright (C) 2022,2023,2025 Konsulko Group
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _KUKSA_CONFIG_H
#define _KUKSA_CONFIG_H

#include <string>

#define KUKSA_CONFIG_DEFAULT_HOSTNAME	"localhost"
#define KUKSA_CONFIG_DEFAULT_PORT	55555
#define KUKSA_CONFIG_DEFAULT_CA_CERT	"/etc/kuksa-val/CA.pem"

class KuksaConfig
{
public:
	explicit KuksaConfig(const std::string &appname);
        ~KuksaConfig() {};

	std::string hostname() { return m_hostname; };
	unsigned port() { return m_port; };
	bool useTls() { return m_useTls; };
	std::string caCertFileName() { return m_caCertFileName; };
	std::string caCert() { return m_caCert; };
	std::string tlsServerName() { return m_tlsServerName; };
	std::string authToken() { return m_authToken; };
	unsigned verbose() { return m_verbose; };
	bool valid() { return m_valid; };

private:
	std::string m_hostname = KUKSA_CONFIG_DEFAULT_HOSTNAME;
	unsigned m_port = KUKSA_CONFIG_DEFAULT_PORT;
	bool m_useTls = false;
	std::string m_caCertFileName = KUKSA_CONFIG_DEFAULT_CA_CERT;
	std::string m_readCaCertFileName = "";
	std::string m_caCert = "";
	std::string m_tlsServerName = "";
	std::string m_authToken = "";
	unsigned m_verbose = 0;
	bool m_valid = false;

	void readConfig(const std::string &configFile, bool app = false);
	void readFile(const std::string &filename, std::string &data);
};

#endif // _KUKSA_CONFIG_H
