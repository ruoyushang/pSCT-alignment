
#include "common/globalalignment/ccd/ImageStar.h"
//#include "common/globalalignment/ccd/CameraParams.h"

using namespace std;


ImageStar::ImageStar(Image &image, const LEDinputs *li) {
    xImageCenter = (image.width() - 1.0) / 2.0;
    yImageCenter = (image.height() - 1.0) / 2.0;
    PixelX = PixelY = 0.0;
    HIP_Index = -1;
    VoteTally = 0;
    right_ascension = 0.0;
    declination = 0.0;
    ledsin = li;
}

void ImageStar::image_unit_vec(double U[3]) {
    U[0] = ImageUnitVector[0];
    U[1] = ImageUnitVector[1];
    U[2] = ImageUnitVector[2];
}

void ImageStar::catalog_unit_vec(double U[3]) {
    U[0] = CatalogUnitVector[0];
    U[1] = CatalogUnitVector[1];
    U[2] = CatalogUnitVector[2];
}

void ImageStar::set_radec(double RA, double DEC) {
    right_ascension = RA;
    declination = DEC;
    CatalogUnitVector[0] = cos(RA) * cos(DEC);
    CatalogUnitVector[1] = sin(RA) * cos(DEC);
    CatalogUnitVector[2] = sin(DEC);
}

long double ImageStar::angular_sep(ImageStar *other) {
    return acosl(this->ImageUnitVector[0] * other->ImageUnitVector[0] +
                 this->ImageUnitVector[1] * other->ImageUnitVector[1] +
                 this->ImageUnitVector[2] * other->ImageUnitVector[2]);
}

void ImageStar::add_pixel(int px, int py, int pmag) {
    StarPixels.push_back({px, -py, pmag});
}

double ImageStar::total_flux() {
    int flux = 0;
    for (unsigned int i = 0; i < StarPixels.size(); i++)
        flux += StarPixels[i][2];
    return flux;
}

void ImageStar::centroid() {
    double weight, flux_norm = 0.0;
    for (unsigned int i = 0; i < StarPixels.size(); i++)
        flux_norm += StarPixels[i][2];

    for (unsigned int i = 0; i < StarPixels.size(); i++) {
        weight = StarPixels[i][2] / flux_norm;
        PixelX += weight * StarPixels[i][0];
        PixelY += weight * StarPixels[i][1];
    }

    double x = PixelX - xImageCenter;
    double y = PixelY + yImageCenter;

    ImageUnitVector[0] = ledsin->PIXSIZE * 1E-6 * x;
    ImageUnitVector[1] = ledsin->PIXSIZE * 1E-6 * y;
    ImageUnitVector[2] = ledsin->LENSFL * 1E-6;
    double norm = sqrt(ImageUnitVector[0] * ImageUnitVector[0] +
                       ImageUnitVector[1] * ImageUnitVector[1] +
                       ImageUnitVector[2] * ImageUnitVector[2]);
    ImageUnitVector[0] /= norm;
    ImageUnitVector[1] /= norm;
    ImageUnitVector[2] /= norm;
}

double ImageStar::ellipticity() {
    // compute moment of inertia tensor [[yy, -xy], [-xy, xx]]
    double xx = 0.0, yy = 0.0, xy = 0.0;
    for (unsigned int i = 0; i < StarPixels.size(); i++) {
        double dx = StarPixels[i][0] - PixelX;
        double dy = StarPixels[i][1] - PixelY;
        xx += dx * dx;
        yy += dy * dy;
        xy += dx * dy;
    }
    xx /= (double) StarPixels.size();
    yy /= (double) StarPixels.size();
    xy /= -(double) StarPixels.size();

    // Find eigenvalues
    double trace = xx + yy;
    double det = xx * yy - xy * xy;
    double eigenval1 = trace / 2.0 + sqrt(trace * trace / 4.0 - det);
    double eigenval2 = trace / 2.0 - sqrt(trace * trace / 4.0 - det);

    // Ellipse parameters
    double semimajor = 2.0 * sqrt(eigenval1);
    double semiminor = 2.0 * sqrt(eigenval2);
    double ellipt = semimajor / semiminor;

    if (ellipt != ellipt)    // check for infinite values
        ellipt = 1000.0;
    return ellipt;
}

int ImageStar::tally_votes() {
    if (Votes.empty()) return 0;

    // Calculate mode
    sort(Votes.begin(), Votes.end());

    VoteTally = 1;
    int count = 1, mode = 0, current = Votes[0];
    for (unsigned int i = 1; i < Votes.size(); i++) {
        if (Votes[i] == current) count++;
        else {
            if (count > VoteTally) {
                VoteTally = count;
                mode = current;
            }
            current = Votes[i];
            count = 1;
        }
    }

    if (VoteTally == 1) return 0;
    HIP_Index = mode;
    return VoteTally;
}
