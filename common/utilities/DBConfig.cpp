//
// Created by Deivid Ribeiro on 2019-03-14.
//

#include "DBConfig.h"


DBConfig::DBConfig()
{
    host_name.clear();
    user_name.clear();
    password.clear();
    db_name.clear();
    port_num = 0;
    socket_name.clear();
    flags = 0;
}


DBConfig::~DBConfig(){}


void DBConfig::resetNULLs(DBConfig *configptr)
{
    //reset all currently NULL variables to values from configptr
    if (configptr == NULL)
        return;
    if (host_name.empty())
        host_name = configptr->getHost();
    if (user_name.empty())
        user_name = configptr->getUser();
    if (password.empty())
        password = configptr->getPassword();
    if (db_name.empty())
        db_name = configptr->getDatabase();
    if (port_num == 0)
        port_num = configptr->getPort();
    if (socket_name.empty())
        socket_name = configptr->getSocket();
    if (flags == 0)
        flags = configptr->getFlags();
}


std::string DBConfig::getHost()
{
    return host_name;
}
void DBConfig::setHost(const char *host)
{
    if (host != NULL)
        host_name = host;
}


std::string DBConfig::getUser()
{
    return user_name;
}
void DBConfig::setUser(const char *user)
{
    if (user != NULL)
        user_name = user;
}


std::string DBConfig::getPassword()
{
    return password;
}
void DBConfig::setPassword(const char *pw)
{
    if (pw != NULL)
        password = pw;
}


std::string DBConfig::getDatabase()
{
    return db_name;
}
void DBConfig::setDatabase(const char *db)
{
    if (db != NULL)
        db_name = db;
}


unsigned int DBConfig::getPort()
{
    return port_num;
}
void DBConfig::setPort(unsigned int port)
{
    port_num = port;
}


std::string DBConfig::getSocket()
{
    return socket_name;
}
void DBConfig::setSocket(const char *socket)
{
    if (socket != NULL)
        socket_name = socket;
}


unsigned int DBConfig::getFlags()
{
    return flags;
}
void DBConfig::setFlags(unsigned int flg)
{
    flags = flg;
}


#include <cstdlib>

DBPubConfig::DBPubConfig(DBConfig &config){}


DBPubConfig::~DBPubConfig(){}


//void DBPubConfig::setGlobalConfig(DBConfig &config)
//throw(VDBGlobalConfigException)
//{
//    if (globalconfig == NULL) {
//        globalconfig = new DBConfig(config);
//    } else {
//        VDBGlobalConfigException ex("Attempt to reset global connection configuration");
//        ex.setThrowLocation(__FILE__, __LINE__);
//        throw ex;
//    }
//}


DBConfig DBPubConfig::getDefaultConfig()
{
    /* Sets VDB connection parameters in the order listed below.
     * At each level, any parameters not set are allowed to
     * be set at the subsequent level
     *
     * setting order:
     * 1) current config set by VDBEnvironment
     * 2) default global config from setGlobalConfig
     * 3) defaults from UNIX environment variables:
     *    DBHOST, DBUSER, DBPW, DBDATABASE, DBSOCKET
     * 4) hardwired defaults
     */
    DBConfig myconfig;
    //fix config defaults from currentconfig
    myconfig.resetNULLs(getCurrentConfigPtr());
    //fix config defaults from globalconfig
    myconfig.resetNULLs(globalconfig);
    //fix config defaults from UNIX environment variables
    DBConfig cfg;
    cfg.setHost(getenv("MYSQL_HOST"));
    cfg.setUser(getenv("MYSQL_USER"));
    cfg.setPassword(getenv("MYSQL_PASSWORD"));
    cfg.setDatabase(getenv("MYSQL_DATABASE"));
    cfg.setSocket(getenv("MYSQL_SOCKET"));
    cfg.setPort(std::atoi( getenv("MYSQL_PORT")));
    myconfig.resetNULLs(&cfg);
    //fix hardwired config defaults
    cfg.setHost("");
    cfg.setUser("CTAreadonly");
    cfg.setPassword("");
    cfg.setDatabase("CTAonline");
    cfg.setSocket("");
    cfg.setPort(0);
    cfg.setFlags(0);
    myconfig.resetNULLs(&cfg);
    return myconfig;
}


DBConfig *DBPubConfig::getCurrentConfigPtr()
{
    return currentconfig;
}


void DBPubConfig::setCurrentConfigPtr(DBConfig *config)
{
    currentconfig=config;
}


//the static variables below are automatically initialized to zero
DBConfig *DBPubConfig::currentconfig;
DBConfig *DBPubConfig::globalconfig;

