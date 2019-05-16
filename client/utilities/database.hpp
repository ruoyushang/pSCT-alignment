/**
 * @file database.h
 * @brief Header file for Database wrapper class.
 */

#ifndef DATABASE_H
#define DATABASE_H

#include "uabase.h"
#include "mysql_connection.h"
#include "mysql_driver.h"
#include <memory>
#include <vector>

class Configuration;

/// @brief Database wrapper/access class for saving MPES readings.
class Database
{
    UA_DISABLE_COPY(Database);
public:
	/// @brief Constructor for Database class.
    Database();
	/// @brief Destructor for Database class.
    virtual ~Database();

	/// @brief Setter method to set internal Configuration object.
	/// @param Pointer to new Configuration object.
    void setConfiguration(std::shared_ptr<Configuration> pConfiguration);

	/// @warning Logic to setup statements and connections is
	/// currently commented out.
	/// @brief Setup method to connect to database(s) and print
	/// metadata/entries from the Configuration object.
    void connectAndPrepare();

    // write to the DB
	/// @warning Currently commented out.
	/// @brief Write an MPES measurement entry consisting of a timestamp and
	/// an x and y position to the database.
	/// @param timestamp Timestamp represented as a C string.
	/// @param x MPES laser x position.
	/// @param y MPES laser y position.
    void write(const char *timestamp, double x, double y);

private:
	/// @brief Pointer to Configuration object.
    std::shared_ptr<Configuration> m_pConfiguration;
	/// @brief Vector of MySQL Driver objects to create connections
	/// to the MySQL server.
    std::vector<sql::Driver*> m_pDriver;
	/// @brief Vector of MySQL Connection objects.
	/// @warning Currently unused.
    std::vector<std::unique_ptr<sql::Connection>> m_pConnection;
	/// @brief Vector of MySQL query statement objects.
	/// @warning Currently unused.
    std::vector<std::unique_ptr<sql::PreparedStatement>> m_pStmt;
};

#endif
