/**
 * @file positioner.h
 * @brief Header file for PositionerObject class inheriting from the PasObject class.
 */

#ifndef _COMPONENTS_H_
#define _COMPONENTS_H_
#include <string>
#include <iostream>

struct Identity
{
    int serialNumber = -1; // engraved serial number
    std::string eAddress = "";     // electronic address: IP, USB port, serial port etc.
    std::string name = ""; // name of the device
    int position = -1;     // location within the immediate parent (see positioning schematic)
    // comparison operator to be able to construct associative maps with this struct as key
    bool operator<(const Identity &r) const;
    bool operator==(const Identity &r) const;
    bool operator!=(const Identity &r) const;
};

inline std::ostream& operator<<(std::ostream& out, const Identity& id)
{
    out << "("
        << id.serialNumber << ", " << id.eAddress << ", " << id.name << ", " << id.position
        << ")";
    return out;
}
#endif
