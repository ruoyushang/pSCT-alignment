#pragma once
#include <vector>
#include <iostream>
#include <ctime>
#include "Image.h"
#include "ImageStar.h"
//#include "ImageFilter.h"
#include "LEDoutputs.h"

class StarDetect {
public:
    // Constructor
    StarDetect(Image& i, std::vector<ImageStar>& s, LEDoutputs* lo) : image(i), StarList(s), ledsin(lo->inleds), ledsout(lo) {
        find_threshold();
        ed_filter();
        detect_stars();
    }

    // Accessors
    std::vector<ImageStar>& DetectedStars() { return StarList; }

private:
    Image& image;
    std::vector<ImageStar>& StarList;
    void find_threshold();
    void ed_filter();
    void detect_stars();
    //int calculate_noise(int nPairs);
    void add_star_pixels(ImageStar* CurrentStar, int pixnum);
    // two helpers for the above
    void __get_neighbors(int pixnum, int neighbors[8]);
    void __add_pixel(ImageStar* CurrentStar, int pixel);
    int threshold;
    const LEDinputs* ledsin;
    LEDoutputs* ledsout;
};
