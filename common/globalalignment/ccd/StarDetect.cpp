#include "common/globalalignment/ccd/StarDetect.h"
#include <set>

using namespace std;

void StarDetect::ed_filter() {
    // find sources in the image by finding isolated regions of pixels above
    // threshold via 'morphological opening', http://en.wikipedia.org/wiki/Opening_%28morphology%29
    // use a 3x3 square as the 'structuring element'

//Save some local variables
    int nxpix = image.width();
    int nypix = image.height();
    int npix = nxpix*nypix;
    int t = threshold; // just to save on typing

// perform morphological erosion on pixels above threshold
// this removes small groups (less than 3x3) of hit pixels
    vector<unsigned char> eimg(npix, 0); // eroded image, initial to False
    for (int x=1; x<nxpix; x++)
        { // loop over x coordinate, not including edges
        for (int y=1; y<nypix; y++)
            { // loop over y coordinate, not including edges
            int i = x+y*nxpix; // location of element x, y of image
            if ((image.pixels()[i] >= t) && (image.pixels()[i+1] >= t) && (image.pixels()[i-1] >= t) &&
                (image.pixels()[i+nxpix] >= t) && (image.pixels()[i+1+nxpix] >= t) && (image.pixels()[i-1+nxpix] >= t) &&
                (image.pixels()[i-nxpix] >= t) && (image.pixels()[i+1-nxpix] >= t) && (image.pixels()[i-1-nxpix] >= t))
                eimg[i] = 1; // set to True
            } // end y for loop
        } // end x for loop

// perform morphological dilation on eroded map
// this puts back the pixels on the edges of surviving groups
    vector<unsigned char> dimg(npix, 0); // eroded image, initial to False
    for (int x=1; x<nxpix; x++)
        { // loop over x coordinate, not including edges
        for (int y=1; y<nypix; y++)
            { // loop over y coordinate, not including edges
              int i = x+y*nxpix; // location of element x, y of image
              if(eimg[i] || eimg[i+1] || eimg[i-1] ||
                  eimg[i+nxpix] || eimg[i+1+nxpix] || eimg[i-1+nxpix] ||
                  eimg[i-nxpix] || eimg[i+1-nxpix] || eimg[i-1-nxpix])
                 dimg[i] = 1; // mark pixel as in a group
            }
    }

//save just stars from the original image (set all non-saved pixels to 0)
    for (int x=0; x<=nxpix; x++)
        { // loop over x coordinate, including edges
        for (int y=0; y<=nypix; y++)
            { // loop over y coordinate, including edges
              int i = x+y*nxpix; // location of element x, y of image
              if (dimg[i]==0)
                 image.pixels()[i] = 0; // erase background pixels
            }
    }
} // end ed_filter()

void StarDetect::__get_neighbors(int pixnum, int neighbors[8])
{
    int tmp[8] = {pixnum-image.width()-1, pixnum-image.width(), pixnum-image.width()+1,
                  pixnum-1,                                     pixnum+1,
                  pixnum+image.width()-1, pixnum+image.width(), pixnum+image.width()+1};
    for (int i = 0; i < 8; i++)
        neighbors[i] = tmp[i];
}

void StarDetect::__add_pixel(ImageStar* CurrentStar, int pixel)
{
    CurrentStar->add_pixel(pixel % image.width(), pixel / image.width(), (int)image.pixels()[pixel]);
    image.pixels()[pixel] = 0;
}

// Add clustered pixels to a star
void StarDetect::add_star_pixels(ImageStar* CurrentStar, int pixnum)
{
    vector<int> pix_to_add;
    pix_to_add.push_back(pixnum);

    while (!pix_to_add.empty()) {
        // add all the currently stored pixels
        for (int pixel : pix_to_add)
            __add_pixel(CurrentStar, pixel);

        // don't want to pass twice over the same pixel -- use set
        set<int> new_neighbors;
        // find all the new neighbors of the currently stored pixels, while
        // removing the current pixels from the vector
        while (!pix_to_add.empty()) {
            int pixel = pix_to_add.back();
            pix_to_add.pop_back();

            // get neighbors of the pixel we just removed
            int neighbors[8];
            __get_neighbors(pixel, neighbors);

            // put all of the non-zero ones in the non-added pixels set
            for (int val : neighbors) {
                if (image.pixels()[val] > 0)
                    new_neighbors.insert(val);
            }
        }

        // repopulate the vector of pixels to add with new neighbors
        for (int pixel : new_neighbors)
            pix_to_add.push_back(pixel);
    }
}

void StarDetect::find_threshold() {
//use a multiplicity of mean to find hotspots
    threshold = 0;
    const int npix = image.width()*image.height();
    for(int i = 0; i < npix; i++)
        threshold += image.pixels()[i];
    // ledsin->THRESHOLD indicates how many times the mean to look for px. THRESHOLD=1 means that (almost) all pixels would be considered part of an led.
    // nominally this is 3-5ish
    threshold *= ledsin->THRESHOLD/(1.0*npix);
}

// Method to automatically detect stars in an image
void StarDetect::detect_stars() {
    // add stars using known nearby positions if there are known LEDs
    if(ledsin->NLED > 0){
        for(int i = 0; i<ledsin->NLED; i++){
            //make cursor the known point
            int cx = abs(ledsin->LEDCCD[i][0]);
            int cy = abs(ledsin->LEDCCD[i][1]); // remember origin is upper-left
            bool foundit = false;
            int dist = 1; //current count for distance travelled this spiral leg
            int length = 1; //go this far in each 'spiral' leg
            bool dir = true; // true = horizontal shift, false = vertical shift

            if(ledsin->VERBOSE ==true){
                cout << "StarDetect: Starting search for LED#" << i << endl;
            }
            while(!foundit){
                if(ledsin->VERBOSE ==true)
                    {
                    cout << "StarDetect: Searching for LED#" << i << " at (" << cx << "," << cy << ")" << endl;
                    }
                if(image.pixels()[image.width()*cy+cx] > threshold){
                    StarList.push_back(ImageStar(image,ledsin));
                    add_star_pixels(&StarList.back(), image.width()*cy+cx);
                    ledsout->LEDSPRESENT[i]=true;
                    ledsout->LEDSCOUNT++;
                    foundit = true;
                }else{
                    //shift position to clockwise spiral until edge found or 500px square surveyed
                    if(dir==true){// shift horizontal
                        cx+=pow(-1.0,length);
                    }else if(dir==false){//shift vertical
                        cy+=pow(-1.0,length);
                    }
                    dist++;
                    if(dist>length){ // turn the corner
                        dist = 1;
                        if(dir==false) // increase the leg length by one when going from veritcal -> horizontal
                            length++;
                        dir = !dir;
                    }

                    if(dist>500 || cx == 0 || cy == 0 || cx == image.width() || cy == image.height() ){
                        string errmsg;
                        if(dist>500) errmsg = "Distance > 500px";
                        if(cx == 0) errmsg = "cx == 0";
                        if(cy == 0) errmsg = "cy == 0";
                        if(cx == image.width()) errmsg = "cx == image.width()";
                        if(cy == image.height()) errmsg = "cy == image.height()";
                        errmsg += ", CP: (" +to_string(cx)+","+to_string(cy)+"), length = "+to_string(length)+".";
                        cout << "StarDetect: FINDING FAILURE (Reason: LED#=" << i+1 << ", " << errmsg << ")" << endl;
                        ledsout->LEDSPRESENT[i]=false;
                        break;
                    }
                }
            } // end foundit while loop
        } // end per LED for loop
    }else{
        // add stars using the add_star_pixels clustered pixels method across whole image if no known stars
        for(int i = 0; i < image.width()*image.height(); i++) {
            if(image.pixels()[i] > threshold) {
                StarList.push_back(ImageStar(image,ledsin));
                add_star_pixels(&StarList.back(), i);
            }
        }
    }

    // centroid stars and apply cuts
    for(unsigned int i = 0; i < StarList.size(); i++) {
        StarList[i].centroid();
        //if(StarList[i].ellipticity() > 1.4)
        //    StarList.erase(StarList.begin() + i--);
    }

    int spacer = 0;
    for(int i=0; i< ledsin->NLED; i++){
        if(ledsout->LEDSPRESENT[i]==false) {
            spacer++;
            ledsout->LEDPOS[i][0]=0;
            ledsout->LEDPOS[i][1]=0;
            }
                try {
                    ledsout->LEDPOS[i][0]=StarList.at(i-spacer).pX();
                    ledsout->LEDPOS[i][1]=StarList.at(i-spacer).pY();
                }
                catch (const std::out_of_range)
                {
                    ledsout->LEDPOS[i][0]=0;
                    ledsout->LEDPOS[i][1]=0;
                }
    }

    if (static_cast<int>(StarList.size()) == ledsin->NLED) ledsout->LEDSMATCH = true;
    else { cout << "StarDetect: MATCHING WARNING. Number of LEDs is different than expected." << endl; }
}
