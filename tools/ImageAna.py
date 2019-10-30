import numpy as np
import matplotlib.pyplot as plt
from astropy.io import fits
from astropy.convolution import Gaussian2DKernel
from astropy.convolution import convolve


def ImageSmearing(array):

    # example taken from: https://docs.astropy.org/en/stable/convolution/
    kernel = Gaussian2DKernel(2) # smearing size = 2 pixels
    astropy_conv = convolve(array, kernel)
    return astropy_conv

def ImageCleaning(array):

    array_clean = array
    n_rows = len(array)
    n_cols = len(array[0])
    threshold = avg_size+10.*std_size
    for row in range(0,n_rows):
        for col in range(0,n_cols):
            if array[row,col]<threshold: array_clean[row,col] = 0.
    return array_clean

def ImageClustering(array):

    pix_isolate_x = []
    pix_isolate_y = []
    pix_isolate_z = []
    n_rows = len(array)
    n_cols = len(array[0])
    for row in range(0,n_rows):
        for col in range(0,n_cols):
            if array[row,col]>0:
                pix_isolate_x += [row]
                pix_isolate_y += [col]
                pix_isolate_z += [array[row,col]]

    clusters = []
    n_pix_isolate = len(pix_isolate_x)
    print 'n_pix_isolate = %s'%(n_pix_isolate)
    while n_pix_isolate>0:
        new_cluster = []
        for pix in range(0,len(pix_isolate_x)): # find a seed
            if pix_isolate_z[pix]>0:
                if len(new_cluster)==0:
                    new_cluster += [[pix_isolate_x[pix],pix_isolate_y[pix],pix_isolate_z[pix]]]
                    pix_isolate_x[pix] = 0
                    pix_isolate_y[pix] = 0
                    pix_isolate_z[pix] = 0
                    n_pix_isolate += -1
                    print 'n_pix_isolate = %s'%(n_pix_isolate)
                    break
        found_a_new_neighbor = True
        while found_a_new_neighbor:
            found_a_new_neighbor = False
            for pix_cluster in range(0,len(new_cluster)):
                for pix in range(0,len(pix_isolate_x)):
                    if pix_isolate_z[pix]>0:
                        dr = pow(pix_isolate_x[pix]-new_cluster[pix_cluster][0],2)
                        dr += pow(pix_isolate_y[pix]-new_cluster[pix_cluster][1],2)
                        if dr<2:
                            new_cluster += [[pix_isolate_x[pix],pix_isolate_y[pix],pix_isolate_z[pix]]]
                            pix_isolate_x[pix] = 0
                            pix_isolate_y[pix] = 0
                            pix_isolate_z[pix] = 0
                            n_pix_isolate += -1
                            print 'n_pix_isolate = %s'%(n_pix_isolate)
                            found_a_new_neighbor = True
        clusters += [new_cluster]

    clusters_centroid = []
    for cls in range(0,len(clusters)):
        z_total = 0
        x_avg = 0
        y_avg = 0
        for pix in range(0,len(clusters[cls])):
            z_total += clusters[cls][pix][2]
            x_avg += clusters[cls][pix][0]*clusters[cls][pix][2]
            y_avg += clusters[cls][pix][1]*clusters[cls][pix][2]
        x_avg = x_avg/z_total
        y_avg = y_avg/z_total
        clusters_centroid += [[x_avg,y_avg,z_total]]

    return clusters_centroid


def GetMeanAndRMS(array):

    n_bins = 0
    total_size = 0.
    total_dev = 0.
    avg_size = 0.
    std_size = 0.
    n_rows = len(array)
    n_cols = len(array[0])

    for row in range(0,n_rows):
        for col in range(0,n_cols):
            n_bins += 1
            total_size += array[row,col]
    avg_size = total_size/n_bins
    
    for row in range(0,n_rows):
        for col in range(0,n_cols):
            total_dev += pow(array[row,col]-avg_size,2)
    std_size = pow(total_dev/n_bins,0.5)

    return avg_size, std_size


fits_image_filename = '/home/vvv/Downloads/GmbH-37514083-2592-1944-Mono8-20190726204551.fits'

with fits.open(fits_image_filename) as hdul:
    data = hdul[0].data

print 'Image dimension:'
print '%s rows x %s cols'%(len(data),len(data[0]))

n_rows = len(data)
n_cols = len(data[0])

n_bins = 0
total_size = 0.
total_dev = 0.
avg_size = 0.
std_size = 0.

data_smear = ImageSmearing(data)
avg_size, std_size = GetMeanAndRMS(data_smear)
data_clean = ImageCleaning(data_smear)
image_cluster_centroid = ImageClustering(data_clean)

print 'avg size = %s +/- %s'%(avg_size,std_size)
print 'found %s clusters'%(len(image_cluster_centroid))
print 'image_cluster_centroid = %s'%(image_cluster_centroid)

plt.imshow(data_clean)
for cls in range(0,len(image_cluster_centroid)):
    plt.plot(image_cluster_centroid[cls][1],image_cluster_centroid[cls][0],'+',color='red',markersize=2)
plt.savefig('plot.png')
