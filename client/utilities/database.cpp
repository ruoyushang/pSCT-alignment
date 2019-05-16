/**
 * @file database.cpp
 * @brief Source file for Database wrapper class.
 */

#include <memory>
#include <string>
#include "database.hpp"
#include "configuration.hpp"

// MySQL C++ Connector includes
#include "cppconn/driver.h"
#include "cppconn/resultset.h"
#include "cppconn/prepared_statement.h"

/// @details Sets internal Configuration pointer to NULL.
Database::Database()
    : m_pConfiguration(nullptr)
{
}

/// @details Clears Connection vector and Statement vector.
Database::~Database()
{
    m_pConnection.clear();
    m_pStmt.clear();
 }

/// @details Assigns provided Configuration pointer to object.
void Database::setConfiguration(std::shared_ptr<Configuration> pConfiguration)
{
    m_pConfiguration = std::move(pConfiguration);
}

/// @details Gets list of databases from the Configuration, creates drivers,
/// and then reads and prints database information (host, user, name, password)
/// and entries saved in Configuration object.
/// @warning Functionality to setup actual database connections and statements
/// is commented out.
void Database::connectAndPrepare()
{
    std::cout << "reading database config details:" << std::endl;
    OpcUa_UInt32 i, j;
    std::cout << "found "<< m_pConfiguration->getDatabaseHost().length() << " databases" << std::endl;
    for ( i = 0; i < m_pConfiguration->getDatabaseHost().length(); i++ )
    {
        m_pDriver.push_back(std::unique_ptr<sql::Driver>(get_driver_instance()));

        //m_pConnection.push_back(m_pDriver.back()->connect(UaString(m_pConfiguration->getDatabaseHost()[i]).toUtf8(),
          //  UaString(m_pConfiguration->getDatabaseUser()[i]).toUtf8(),
          // UaString(m_pConfiguration->getDatabasePassword()[i]).toUtf8()));

       // m_pConnection.back()->setSchema(UaString(m_pConfiguration->getDatabaseName()[i]).toUtf8());

        OpcUa_UInt32 size;
        size = m_pConfiguration->getDatabaseEntries().at(i).length();

        std::cout << UaString(m_pConfiguration->getDatabaseHost()[i]).toUtf8() << std::endl;
        std::cout << UaString(m_pConfiguration->getDatabaseUser()[i]).toUtf8() << std::endl;
        std::cout << UaString(m_pConfiguration->getDatabaseName()[i]).toUtf8() << std::endl;
        std::cout << UaString(m_pConfiguration->getDatabasePassword()[i]).toUtf8() << std::endl;
        std::cout << "database " << i << " has " << size << " entries:" << std::endl;
        for (j = 0; j < size; j++)
        {
            std::cout << "      ";
            std::cout << UaString(m_pConfiguration->getDatabaseEntries().at(i)[j]).toUtf8() << std::endl;
        }
    }
    // the values to read/write are in the format
    // VarNameOnTheServer;ReadOrWrite;Type;VarNameInTheDatabase
//    OpcUa_Int32 sPos=0, ePos=0;
//    ePos = UaString(m_pConfiguration->getDatabaseEntries()[i]).find(';', sPos);

//    nameOnTheServer = UaStringk

    /* prepare the statement
    std::string stmt = "INSERT INTO mpes(";
    for ( i = 0; i < size - 1; i++ )
        stmt += std::string(UaString(m_pConfiguration->getDatabaseEntries()[i]).toUtf8())
             + std::string(", ");
    stmt +=  std::string(UaString(m_pConfiguration->getDatabaseEntries()[size-1]).toUtf8())
        + std::string(") VALUES (");
    for ( i = 0; i < size - 1; i++ )
        stmt += "?, ";
    stmt += "?)";

    m_pStmt = m_pConnection->prepareStatement(stmt.c_str()); */
}

/// @details Adds timestamped MPES reading to database.
/// @warning Currently commented out and unfinished.
void Database::write(const char *timestamp, double x, double y)
{
    /*
    OpcUa_UInt32 i;
    for ( i = 0; i < m_pConfiguration->getDatabaseEntries().length(); i++ )
        if (m_pConfiguration->getDatabaseEntries
    m_pStmt->setString(1, timestamp);
    m_pStmt->setDouble(2, x);
    m_pStmt->setDouble(3, y);

    if ( m_pStmt->executeUpdate() != 1) printf("failed to update DB;\n");
    */
}
