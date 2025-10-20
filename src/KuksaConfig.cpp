/*
 * Copyright (C) 2022,2023,2025 Konsulko Group
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <exception>
#include <filesystem>
#include <toml.hpp>
#include "KuksaConfig.h"

KuksaConfig::KuksaConfig(const std::string &appname)
{
	// Read global configuration, allowing for override under XDG_CONFIG_HOME
	std::string configFile = "";
	char *home = getenv("XDG_CONFIG_HOME");
	if (home) {
		configFile = home;
		configFile += "/AGL/kuksa.toml";

		if (!std::filesystem::exists(std::filesystem::status(configFile)))
			configFile.clear();
	}
	if (configFile.empty())
		configFile = "/etc/xdg/AGL/kuksa.toml";
	readConfig(configFile);
	if (!m_valid)
		return;

	// Read application configuration, again allowing for override under XDG_CONFIG_HOME
	configFile = "";
	if (home) {
		configFile = home;
		configFile += "/AGL/";
		configFile += appname;
		configFile += "/kuksa.toml";

		if (!std::filesystem::exists(std::filesystem::status(configFile)))
			configFile.clear();
	}
	if (configFile.empty()) {
		configFile = "/etc/xdg/AGL/";
		configFile += appname;
		configFile += "/kuksa.toml";
		
	}
	readConfig(configFile, true);
}

// Private

void KuksaConfig::readConfig(const std::string &configFile, bool app)
{
	m_valid = false;

	toml::value config;
	std::cout << "Reading KUKSA.val configuration " << configFile << std::endl;
	try {
		config = toml::parse(configFile);
	} catch(const toml::syntax_error& err) {
		std::cerr << "Syntax error in " << configFile << std::endl;

		// Report all the errors
		std::cerr << err.what() << std::endl;
		return;
	} catch(const std::exception& ex) {
		std::cerr << "Could not read " << configFile << std::endl;
		return;
	}

	if (config.contains("hostname")) {
		std::string hostname = toml::get_or(config.at("hostname"), "");
		if (hostname.empty()) {
			std::cerr << "Invalid server hostname" << std::endl;
			return;
		}
		m_hostname = hostname;
	}

	if (config.contains("port")) {
		unsigned port = toml::get_or(config.at("port"), 0);
		if (port == 0) {
			std::cerr << "Invalid server port" << std::endl;
			return;
		}
		m_port = port;
	}

	if (config.contains("use-tls") && config.at("use-tls").is_boolean()) {
		m_useTls = toml::get_or(config.at("use-tls"), m_useTls);
	}

	if (config.contains("ca-certificate")) {
		std::string caCertFileName = toml::get_or(config.at("ca-certificate"), "");
		if (!caCertFileName.empty()) {
			m_caCertFileName = caCertFileName;
		} else {
			std::cerr << "Invalid CA certificate filename: " << caCertFileName << std::endl;
			return;
		}
	}
	if (!m_caCertFileName.empty()) {
		if (m_readCaCertFileName != m_caCertFileName) {
			readFile(m_caCertFileName, m_caCert);
			if (m_caCert.empty()) {
				std::cerr << "Invalid CA certificate file: " << m_caCertFileName << std::endl;
				return;
			}
			m_readCaCertFileName = m_caCertFileName;
		}
	}

	if (config.contains("tls-server-name")) {
		m_tlsServerName = toml::get_or(config.at("tls-server-name"), "");
	}

	if (config.contains("authorization")) {
		std::string auth = toml::get_or(config.at("authorization"), "");
		if (!auth.empty()) {
			if (auth.front() == '/') {
				readFile(auth, m_authToken);
				if (m_authToken.empty()) {
					std::cerr << "Invalid authorization token file: " << auth << std::endl;
					return;
				}
			} else {
				// Assume token text has been given
				m_authToken = auth;
			}
		}
	}

	// Verbosity option is application-specific
	if (app) {
		if (config.contains("verbose")) {
			std::string verbose = toml::get_or(config.at("verbose"), "invalid");
			if (!verbose.empty() && verbose != "invalid") {
				if (verbose == "true" || verbose == "1")
					m_verbose = 1;
				if (verbose == "2")
					m_verbose = 2;
			}
		}
	}

	m_valid = true;
}

void KuksaConfig::readFile(const std::string &filename, std::string &data)
{
	try {
		std::ifstream file;
		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		const std::filesystem::path p(filename);
		file.open(p, std::ios_base::binary);
		if (file.good()) {
			std::size_t sz = static_cast<std::size_t>(std::filesystem::file_size(p));
			data.resize(sz, '\0');
			file.read(&data[0], sz);
		} else {
			data.clear();
		}
	} catch (const std::exception &e) {
		data.clear();
	}
}
