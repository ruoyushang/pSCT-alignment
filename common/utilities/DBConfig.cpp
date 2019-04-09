//
// Created by Deivid Ribeiro on 2019-03-14.
//

#include "DBConfig.hpp"
#include <string>

void DBConfig::overwriteDefaultValues(DBConfig *configptr)
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
    if (port_num.empty())
        port_num = configptr->getPort();
    if (socket_name.empty())
        socket_name = configptr->getSocket();
    if (flags.empty())
        flags = configptr->getFlags();
}


std::string DBConfig::getHost()
{
    return host_name;
}
void DBConfig::setHost(std::string const& host)
{
    if (!host.empty())
        host_name = host;
}


std::string DBConfig::getUser()
{
    return user_name;
}
void DBConfig::setUser(std::string const& user)
{
    if (!user.empty())
        user_name = user;
}


std::string DBConfig::getPassword()
{
    return password;
}
void DBConfig::setPassword(std::string const& pw)
{
    if (!pw.empty())
        password = pw;
}


std::string DBConfig::getDatabase()
{
    return db_name;
}
void DBConfig::setDatabase(std::string const& db)
{
    if (!db.empty())
        db_name = db;
}


std::string DBConfig::getPort()
{
    return port_num;
}
void DBConfig::setPort(std::string const& port)
{
    port_num = port;
}


std::string DBConfig::getSocket()
{
    return socket_name;
}
void DBConfig::setSocket(std::string const& socket)
{
    if (!socket.empty())
        socket_name = socket;
}


std::string DBConfig::getFlags()
{
    return flags;
}
void DBConfig::setFlags(std::string const& flg)
{
    flags = flg;
}



DBConfig DBConfig::getDefaultConfig()
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
    myconfig.overwriteDefaultValues(getCurrentConfigPtr());
    //fix config defaults from UNIX environment variables
    DBConfig cfg;
    cfg.setHost(getenv("MYSQL_HOST"));
    cfg.setUser(getenv("MYSQL_USER"));
    cfg.setPassword(getenv("MYSQL_PASSWORD"));
    cfg.setDatabase(getenv("MYSQL_DATABASE"));
    cfg.setSocket(getenv("MYSQL_SOCKET"));
    cfg.setPort(getenv("MYSQL_PORT"));
    myconfig.overwriteDefaultValues(&cfg);
    //fix hardwired config defaults
    cfg.setHost("");
    cfg.setUser("");
    cfg.setPassword("");
    cfg.setDatabase("");
    cfg.setSocket("");
    cfg.setPort("");
    cfg.setFlags("");
    myconfig.overwriteDefaultValues(&cfg);
    return myconfig;
}


DBConfig *DBConfig::getCurrentConfigPtr()
{
    return currentconfig;
}


void DBConfig::setCurrentConfigPtr(DBConfig *config)
{
    currentconfig=config;
}


//the static variables below are automatically initialized to zero
DBConfig *DBConfig::currentconfig;

