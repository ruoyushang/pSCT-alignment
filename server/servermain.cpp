/**
 * @file servermain.cpp
 * @brief Entry point for passerver.
 */

#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <sys/statvfs.h>

#include "uabase/uadatetime.h"
#include "uabase/uaplatformlayer.h"
#include "uabase/uathread.h"

#include "uaserver/serverconfig.h"
#include "uaserver/serverconfigdata.h"
#include "uaserver/serverconfigxml.h"
#include "uaserver/uaserverapplication.h"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/sinks/basic_file_sink.h"
#include "common/utilities/spdlog/sinks/stdout_color_sinks.h"
#include "common/utilities/spdlog/sinks/rotating_file_sink.h"
#include "common/utilities/spdlog/logger.h"
#include "common/utilities/spdlog/fmt/ostr.h"

#if SUPPORT_XML_PARSER
#include "xmlparser/xmldocument.h"
#endif

#include "common/utilities/opcserver.hpp"
#include "common/utilities/shutdown.hpp"

#include "server/pascommunicationinterface.hpp"
#include "server/pasnodemanager.hpp"


/// @brief Check the available disk space on the system.
/// @param size Double to store the retrieved file system size (in GB).
/// @return 0 on success and -1 on failure.
int checkSystem(double &size)
{
    struct statvfs sysBuf{};
    if (statvfs("/", &sysBuf) == 0) {
        size = ((double) sysBuf.f_bsize * sysBuf.f_bfree) / (1024 * 1024 * 1024);
        return 0;
    } else {
        return -1;
    }
}

/// @brief Initialize, configure, and start the OPC UA server.
/// @param szAppPath The file path of the application.
/// @param configFilePath The file path to the server configuration file.
/// @param serverIP The IP address for the server endpoint.
/// @return 0 on success and -1 on failure.
int OpcServerMain(const std::string &szAppPath, const std::string &configFilePath,
                  const std::string &panelNumber, const std::string &endpointUrl = "") {

    UaStatus status;

#if SUPPORT_XML_PARSER
    UaXmlDocument::initParser(); // initialize the XML Parser
#endif

    if (UaPlatformLayer::init() == 0) {
        UaString sAppPath(szAppPath.c_str());
        UaString sConfigFileName(configFilePath.c_str());

        spdlog::info("Creating OpcServer object...");
        // initialize and configure server object
        std::unique_ptr<OpcServer> pServer = std::unique_ptr<OpcServer>(
            new OpcServer());
        pServer->setServerConfig(sConfigFileName, sAppPath);

        if (!endpointUrl.empty()) {
            UaString sEndpointUrl(endpointUrl.c_str());
            UaEndpointArray endpoints;
            OpcUa_UInt32 rejected_certificates_count = 1;
            ServerConfig *pConfig = pServer->getServerConfig();
            pConfig->loadConfiguration();
            pConfig->getEndpointConfiguration(sAppPath, rejected_certificates_count, endpoints);
            spdlog::info("Setting endpoint address to: {}", sEndpointUrl.toUtf8());
            endpoints[0]->setEndpointUrl(sEndpointUrl, false);
        }

        spdlog::info("Creating Node Manager...");
        std::shared_ptr<PasServerNodeManager> pNodeManager = std::make_shared<PasServerNodeManager>(); // Create Node Manager for the server
        pNodeManager->setPanelNumber(std::stoi(panelNumber));
        pServer->addNodeManager(pNodeManager.get()); // Add node manager to server

        if (pServer->start() == 0) {
            spdlog::info(
                "\n***************************************************\n"
                " Press {} to shut down server\n"
                "***************************************************", SHUTDOWN_SEQUENCE);

            while (ShutDownFlag() == 0) {
                UaThread::msleep(1000); // Wait for user command to terminate the server.
            }

            spdlog::info(
                "\n***************************************************\n"
                " Shutting down server\n"
                "***************************************************");

            // Stop the server and wait three seconds if clients are connected
            // to allow them to disconnect after they received the shutdown signal
            pServer->stop(3, UaLocalizedText("", "User shutdown"));
        }
    }

    UaPlatformLayer::cleanup(); // Clean up the UA Stack platform layer
#if SUPPORT_XML_PARSER
    UaXmlDocument::cleanupParser(); // Clean up the XML Parser
#endif
    if (status.isGood()) {
        return 0;
    } else {
        return -1;
    }
}

/// @brief Start the server.
/// @param argc Number of args.
/// @param argv Array of args.
/// @return Returns 0 on success and -1 on failure.
#ifdef _WIN32_WCE
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPWSTR, int)
#else
int main(int argc, char* argv[])
#endif
{
    std::string usage = "<PANEL POSITION NUMBER> -c <CONFIG FILE PATH (optional)> -u <ENDPOINT URL (optional)> -l <LOGGER LEVEL (optional)>";

    if (argc == 1) {
        std::cout << "Usage: " << argv[0] << " " << usage << std::endl;
        return -1;
    }

    int c;
    std::string panelNumber, configFilePath, endpointUrl;
    std::string logLevel("info");

    while ((c = getopt(argc, argv, "c:u:l:")) != -1) {
        switch(c)
        {
            case 'c':
                configFilePath = optarg;
                break;
            case 'u':
                endpointUrl = optarg;
                break;
            case 'l':
                logLevel = optarg;
                break;
            case '?':
                if (optopt == 'c')
                    std::cout << "Must provide a config file path with option c\n";
                else if (optopt == 'u')
                    std::cout << "Must provide an endpoint URL with option u\n";
                else if (optopt == 'l')
                    std::cout << "Must provide a logger level with option u (trace, debug, info, warn, error)\n";
            default:
                abort();
        }
    }

    if (optind != argc - 1) {
        std::cout << "Invalid number of positional arguments: " << (argc - optind)
                  << ". Expected only 1 (panel number)." << std::endl;
        std::cout << "Usage: " << argv[0]
                  << " " << usage << std::endl;
        return -1;
    } else {
        panelNumber = argv[optind];
    }

    // Setup logging
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>(); // log to console
    if (logLevel == "trace") {
        console_sink->set_level(spdlog::level::trace);
    } else if (logLevel == "debug") {
        console_sink->set_level(spdlog::level::debug);
    } else if (logLevel == "info") {
        console_sink->set_level(spdlog::level::info);
    } else if (logLevel == "warn") {
        console_sink->set_level(spdlog::level::warn);
    } else if (logLevel == "error") {
        console_sink->set_level(spdlog::level::err);
    } else if (logLevel == "critical") {
        console_sink->set_level(spdlog::level::critical);
    } else {
        std::cout << "Invalid logger level selected: " << logLevel
                  << ". Valid choices are 'trace', 'debug', 'info', 'warn', 'error', 'critical'.";
        return -1;
    }
    std::cout << "Console logging level set to: " << logLevel << std::endl;

    // Note that log directory must have been created beforehand
#ifndef SIMMODE 
    auto file_sink = make_shared<spdlog::sinks::rotating_file_sink_mt>(std::string(getenv("HOME")) + std::string("/logs/passerver_logs"), 1048576 * 5, 5, false); // 5 rotating files with max size 5 MB
#else 
    auto file_sink = make_shared<spdlog::sinks::rotating_file_sink_mt>(std::string(getenv("HOME")) + std::string("/logs/passerver_") + panelNumber + "_logs", 1048576 * 5, 5, false); // 5 rotating files with max size 5 MB
#endif
    file_sink->set_level(spdlog::level::trace); // always save all logging levels
    file_sink->set_pattern("[%c] [%n] [%l] [%s:%!:%#] %v");

    std::vector<spdlog::sink_ptr> sinks = {file_sink, console_sink};
    auto logger = std::make_shared<spdlog::logger>(std::string("passerver"), sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::info);
    spdlog::set_default_logger(logger);

    // NOTE: This method returns a pointer to heap-allocated memory, must be freed manually
    char *pszAppPath = getAppPath(); // Extract application path

#if SUPPORT_XML_PARSER
    std::string sConfigFileName = "/ServerConfig.xml";
#else
    std::string sConfigFileName = "/ServerConfig.ini";
#endif

    if (configFilePath.empty()) {
        logger->warn("No config file path passed, using default.");
        configFilePath = std::string(pszAppPath) + sConfigFileName;
    }

    double sysFree;
    std::string temp;
    std::ostringstream os;
    os << "\n********************SYSTEM INFO********************" << std::endl;
    if (!checkSystem(sysFree)) // check available disk space
        os << " Free disk space: " << std::to_string(sysFree) << "GB" << std::endl;
    else {
        spdlog::error(" Failed to stat \"/\": terminating\n");
        return -1;
    }
    os << "***************************************************" << std::endl;
    spdlog::info(os.str());
    RegisterSignalHandler();

    int ret = OpcServerMain(pszAppPath, configFilePath, panelNumber, endpointUrl);

    delete[] pszAppPath; // release pszAppPath memory manually

    return ret;
}
