//
// Created by Deivid Ribeiro on 2019-03-14.
//

#ifndef ALIGNMENT_DBCONFIG_H
#define ALIGNMENT_DBCONFIG_H
#include <string>
#include <cstdlib>

using namespace std;

class DBConfig {

public:

    DBConfig(){
        host_name.clear();
        user_name.clear();
        password.clear();
        db_name.clear();
        port_num.clear();
        socket_name.clear();
        flags.clear();
    };

    void overwriteDefaultValues(DBConfig *configptr);

    std::string getHost();

    void setHost(std::string const &host);

    std::string getUser();

    void setUser(std::string const &user);

    std::string getPassword();

    void setPassword(std::string const &pw);

    std::string getDatabase();

    void setDatabase(std::string const &db);

    std::string getPort();

    void setPort(std::string const &port);

    std::string getSocket();

    void setSocket(std::string const &socket);

    std::string getFlags();

    void setFlags(std::string const &flg);

    static DBConfig getDefaultConfig();

    static DBConfig *getCurrentConfigPtr();

    static void setCurrentConfigPtr(DBConfig *config);

private:

    std::string host_name;
    std::string user_name;
    std::string password;
    std::string db_name;
    std::string port_num;
    std::string socket_name;
    std::string flags;
    static DBConfig *currentconfig;
};


#endif //ALIGNMENT_DBCONFIG_H
