/**
 * @file pascontroller.hpp
 * @brief Header file for generic device controller class
 */
#ifndef __PASCONTROLLER_H__
#define __PASCONTROLLER_H__

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"
#include "uabase/uavariant.h"

#include "common/opcua/pascominterfacecommon.h"

#include <chrono>
#include <memory>
#include <map>

class Platform;

/// @brief Generic controller class for PAS devices
class PasController
{
    UA_DISABLE_COPY(PasController); // Disables copy construction and copy assignment
    public:
        /// @brief Instantiate a PasController object with a Platform object.
        /// @param ID Device index within type.
        /// @param pPlatform Platform object.
        /// @param updateInterval Update interval in milliseconds.
        PasController(int ID, std::shared_ptr<Platform> pPlatform = std::shared_ptr<Platform>(nullptr), int updateInterval = 0) : m_ID(ID), m_pPlatform(pPlatform), m_kUpdateInterval_ms(updateInterval) {}
        /// @brief Instantiate a PasController object without a Platform object.
        /// @param ID ID Device index within type.
        /// @param updateInterval Update interval in milliseconds.
        PasController(int ID, int updateInterval) : PasController(ID, std::shared_ptr<Platform>(nullptr), updateInterval) {}
        /// @brief Initialize the controller object.
        /// @return 0 on success, -1 on failure.
        virtual int Initialize() {return 0;} // only MPES overload this for now
        /// @brief Getter for the device ID.
        /// @return The device ID.
        int getId() { return m_ID; }
        /// @brief Destroy a PasController object.
        virtual ~PasController(){}

        /// @brief Get the device's state.
        /// @return OPC UA status code indicating success or failure.
        virtual UaStatus getState(PASState& state);
        /// @brief Get data from a device variable.
        /// @return OPC UA status code indicating success or failure.
        virtual UaStatus getData(OpcUa_UInt32 offset, UaVariant& value) = 0; // Pure virtual function
        
        /// @brief Set the device's state.
        /// @return OPC UA status code indicating success or failure.
        virtual UaStatus setState(PASState state);
        /// @brief Set a device data variable.
        /// @return OPC UA status code indicating success or failure.
        virtual UaStatus setData(OpcUa_UInt32 offset, UaVariant value) = 0; // Pure virtual function
        /// @brief Call a device method.
        /// @return OPC UA status code indicating success or failure.
        virtual UaStatus Operate(OpcUa_UInt32 offset, const UaVariantArray& args = UaVariantArray()) = 0; // Pure virtual function

    protected:
        /// @brief Shared mutex used to lock parallel access to controller variables.
        UaMutex m_mutex;
        /// @brief Integer index of the device within its type.
        int m_ID;
        /// @brief Device state.
        PASState m_state;
        /// @brief Pointer to Platform object used to interface with hardware.
        std::shared_ptr<Platform> m_pPlatform;

        /// @brief Time interval between updates.
        const int m_kUpdateInterval_ms;
        /// @brief Whether a time interval longer than m_kUpdateInterval_ms has elapsed since the last update.
        // @return A bool indicating whether the update interval has elapsed since the last update.
        bool _expired() const {return (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - m_lastUpdateTime).count() > m_kUpdateInterval_ms); }
        /// @brief Time of last variable update.
        std::chrono::time_point<std::chrono::system_clock> m_lastUpdateTime;
};

#endif
