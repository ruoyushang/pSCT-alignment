//
// Created by Deivid Ribeiro on 2019-03-14.
//

#ifndef ALIGNMENT_DBCONFIG_H
#define ALIGNMENT_DBCONFIG_H
#include <string>

using namespace std;

class DBConfig {

public:

    DBConfig();

    ~DBConfig();

    void resetNULLs(DBConfig *configptr);

    std::string getHost();
    void setHost(const char *host);

    std::string getUser();
    void setUser(const char *user);

    std::string getPassword();
    void setPassword(const char *pw);

    std::string getDatabase();
    void setDatabase(const char *db);

    unsigned int getPort();
    void setPort(unsigned int port);

    std::string getSocket();
    void setSocket(const char *socket);

    unsigned int getFlags();
    void setFlags(unsigned int flg);

private:

    std::string host_name;
    std::string user_name;
    std::string password;
    std::string db_name;
    unsigned int port_num;
    std::string socket_name;
    unsigned int flags;

};



class DBPubConfig {

public:

    DBPubConfig(DBConfig &config);

    ~DBPubConfig();

//    static void setGlobalConfig(DBConfig &config) throw(DBGlobalConfigException);

    static DBConfig getDefaultConfig();

    static DBConfig *getCurrentConfigPtr();

    static void setCurrentConfigPtr(DBConfig *config);

private:


    static DBConfig *currentconfig;
    static DBConfig *globalconfig;

};




#endif //ALIGNMENT_DBCONFIG_H
