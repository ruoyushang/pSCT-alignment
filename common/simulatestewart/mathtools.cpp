#include "mathtools.h"
#include "mirrordefinitions.h"
#include <algorithm>
#include <numeric> // accumulate
#include <string>

using namespace std;

namespace SCTMath {
    unsigned Position(unsigned mirror, unsigned quadrant, unsigned ring, unsigned panel)
    {
        return mirror * 1000 + quadrant * 100 + ring * 10 + panel;
    }

    unsigned GetPanelNeighbor(unsigned position, int dir)
    {
        int mirror = Mirror(position);
        int ring = Ring(position);
        // define our cyclic group:
        // total panels in the ring
        int size = (mirror == 1) ? SCT::Primary::kPanels[ring - 1] : SCT::Secondary::kPanels[ring - 1];
        int base = size / 4; // panels in a quadrant, always 4 quadrants in total

        return GroupAdd(position, size, base, dir*2 - 1);
    }

    string GetEdgeNeighbor(string address, int dir)
    {
        // get the greater panel and the neighbor of the greater panel in the sense of dir
        unsigned panel1 = GetPanelsFromEdge(address, dir).at(1);
        unsigned panel2 = GetPanelNeighbor(panel1, dir);

        return GetEdgeFromPanels({panel1, panel2});
    }

    // get the address of the edge given panels
    string GetEdgeFromPanels(const vector<unsigned>& panelPos)
    {
        auto panel_positions = panelPos;
        // order the panels in _regular_ ascending order to name the edge
        std::sort(panel_positions.begin(), panel_positions.end());

        // join this vector into a string: "panel1+panel2+panel3"
        return std::accumulate(panel_positions.begin() + 1, panel_positions.end(),
                std::to_string(panel_positions.front()),
                [](std::string s, int i){return s + "+" + std::to_string(i);});
    }

    // return the panels of this edge, smaller->greater in the sense of dir
    vector<unsigned> GetPanelsFromEdge(const string& address, int dir)
    {
        vector<unsigned> curPanels;

        // split the edge address on "+"
        string delim = "+";
        string in_str = address + delim;
        size_t prev = 0, cur;
        while ( (cur = in_str.find_first_of(delim, prev)) != string::npos ) {
            if (cur > prev)
                curPanels.push_back(stoi(in_str.substr(prev, cur-prev)));
            prev = cur + 1;
        }

        // assume two panels though
        unsigned panel1 = min(curPanels.at(0), curPanels.at(1), [dir](unsigned a, unsigned b) {return IsLess(a, b, dir); } );
        unsigned panel2 = max(curPanels.at(0), curPanels.at(1), [dir](unsigned a, unsigned b) {return IsLess(a, b, dir); } );

        return {panel1, panel2};
    }

    bool IsLess(unsigned pos1, unsigned pos2, int dir)
    {
        return GetPanelNeighbor(pos1, dir) == pos2;
    }

    unsigned GroupAdd(unsigned position, unsigned size, unsigned base, int inc)
    {
        if (inc == 0) return position;

        // first, we gather all the definitions
        // we subtract the 1 here to have 0 as the true identity element of the group
        int quadrant = Quadrant(position) - 1;
        int panel = Panel(position) - 1;

        // we need to shift within the group by this much:
        if (inc < 0) {
            int shift = size +  inc % size;
        }
        else {
            int shift = inc % size;
        }


        // this is the position within the group written in our base
        int group_pos = quadrant*base + panel;
        // shift it
        group_pos += shift;
        group_pos %= size;
        // and convert back to decimals
        panel = group_pos % base;
        quadrant = (group_pos - panel)/base;

        // remember to add back the 1 we subtracted before:
        return Position(Mirror(position), quadrant + 1, Ring(position), panel + 1);
    }

}
