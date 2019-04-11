#ifndef __MATHTOOLS_H__
#define __MATHTOOLS_H__

#include <string>
#include <vector>

namespace SCTMath {

    // get mirror, quadrant, ring and panel from position
    inline unsigned Mirror(unsigned pos) { return pos / 1000; };
    inline unsigned Quadrant(unsigned pos) {return (pos % 1000) / 100; };
    inline unsigned Ring(unsigned pos) { return (pos % 100) / 10; };
    inline unsigned Panel(unsigned pos) { return pos % 10; };

    // get position from mirror, quadrant, ring and panel
    unsigned Position(unsigned mirror, unsigned quadrant, unsigned ring, unsigned panel);

    // add inc to a panel position along the ring -- essentially
    // addition/subtraction on a cyclic group of size, represented in base digits;
    unsigned GroupAdd(unsigned position, unsigned size, unsigned base, int inc);
    bool IsLess(unsigned pos1, unsigned pos2, int dir);
    unsigned GetPanelNeighbor(unsigned position, int dir);
    std::string GetEdgeNeighbor(std::string address, int dir);
    std::string GetEdgeFromPanels(const std::vector<unsigned>& panelPos);
    // return the panels of this edge, smaller->greater in the sense of dir
    std::vector<unsigned> GetPanelsFromEdge(const std::string& address, int dir);
};

#endif // #define __MATHTOOLS_H__
