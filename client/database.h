#ifndef DATABASE_H
#define DATABASE_H

#include "uabase.h"
#include "mysql_connection.h"
#include "mysql_driver.h"
#include <vector>

class Configuration;

class Database
{
    UA_DISABLE_COPY(Database);
public:
    Database();
    virtual ~Database();

    // Set Configuration of the database connection/what to write
    void setConfiguration(const Configuration *pConfiguration);
    
    // connect to the DB and prepare the statement
    void connectAndPrepare(); 

    // write to the DB
    void write(const char *timestamp, double x, double y);

private:
    const Configuration *m_pConfiguration;
    std::vector<sql::Driver*> m_pDriver;
    std::vector<sql::Connection*> m_pConnection;
    std::vector<sql::PreparedStatement*> m_pStmt;
};

#endif
