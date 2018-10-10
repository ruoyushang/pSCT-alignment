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
/*
class Component
{
    public:
        Identity GetIdentity() {return id;};
        void SetIdentity(Identity input_id);
    
    private:
        Identity id;
};


class Panel : Component
{
    public:
        Panel();
        ~Panel();

        Device& GetDevice(DeviceType device_to_id, Identity partial_device_id);
    
    private:
        CBC *cbc;

};

class Device : Component
{
    public:
        Identity GetPanel() {return parent->GetIdentity();};

    private:
        Panel *parent;
}

enum DeviceType {ACT, MPES};

class ACTDevice : Device
{
    public:
        ACTDevice();
        ~ACTDevice();
};

class MPESDevice : Device
{
    public:
        MPESDevice();
        ~MPESDevice();
}; */

#endif
