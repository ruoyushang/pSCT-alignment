#pragma once

#include <vector>
#include <iostream>
#include <vector>
#include <algorithm>

#define _USE_MATH_DEFINES

#include <math.h>            // needed for fitting bounding ellipse
#include "common/globalalignment/ccd/Image.h"
#include "common/globalalignment/ccd/LEDinputs.h"

using std::vector;


class ImageStar {
public:
    // Constructor
    ImageStar(Image &image, const LEDinputs *li);

    // Compute angular separation between two stars
    long double angular_sep(ImageStar *other);

    // Getters
    double pX() { return PixelX; }

    double pY() { return PixelY; }

    double ra() { return right_ascension; }

    double dec() { return declination; }

    void image_unit_vec(double U[3]);

    void catalog_unit_vec(double U[3]);

    int votes() { return VoteTally; }

    int get_index() { return HIP_Index; }

    int tally_votes();

    // Setters
    void reset_index() { HIP_Index = -1; }

    void set_radec(double RA, double DEC);

    void add_vote(int vote) { Votes.push_back(vote); }

    void clear_votes() { Votes.clear(); }

protected:
    // Methods used by StarDetect
    void add_pixel(int px, int py, int pmag);

    void centroid();

    double ellipticity();

    double total_flux();

    // Method used by GUI
    vector<vector<int> > StarPixels;        // container with all pixels associated with star

private:
    double PixelX, PixelY;                    // image coordinates
    double right_ascension, declination;    // star coordinates
    double xImageCenter, yImageCenter;
    double CatalogUnitVector[3];
    long double ImageUnitVector[3];
    const LEDinputs *ledsin;

    vector<int> Votes;
    int HIP_Index, VoteTally;

    friend class StarDetect;

    friend class GUI;
};

