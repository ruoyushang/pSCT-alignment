import sys,ROOT
import array
import math
from array import *
from ROOT import *

ROOT.gStyle.SetOptStat(0)

def FindPanelPosition(panel_id):
    radius = 0.
    phase = 0.
    is_primary = True
    is_inner_ring = True
    if list(panel_id)[0]=="1":
        is_primary = True
    else:
        is_primary = False
    if list(panel_id)[2]=="1":
        is_inner_ring = True
    else:
        is_inner_ring = False
    quadrant = float(list(panel_id)[1])
    segment = float(list(panel_id)[3])
    total_segment = 0.
    if is_primary and is_inner_ring:
        total_segment = 4.
    elif is_primary and not is_inner_ring:
        total_segment = 8.
    elif not is_primary and is_inner_ring:
        total_segment = 2.
    elif not is_primary and not is_inner_ring:
        total_segment = 4.
    phase = (quadrant-1)*0.5*ROOT.TMath.Pi()+0.5*ROOT.TMath.Pi()*(segment-0.5)/total_segment
    if phase>ROOT.TMath.Pi():
        phase = phase - 2.*ROOT.TMath.Pi()
    if is_inner_ring:
        radius = 1.
    else:
        radius = 2.5
    return radius, phase

if __name__ == "__main__":

        outputfilename = sys.argv[1]
        outputfile = open('%s'%(outputfilename),"r")
        
        is_an_position_block = False
        panel_coordinates = []
        panel_x = 0.
        panel_y = 0.
        panel_z = 0.
        panel_xrot = 0.
        panel_yrot = 0.
        panel_zrot = 0.
        
        Hist_PanelPositionZ_outer = ROOT.TH2D("Hist_PanelPositionZ_outer","",32,-1.*ROOT.TMath.Pi(),1.*ROOT.TMath.Pi(),2,0,4)
        Hist_PanelPositionZ_inner = ROOT.TH2D("Hist_PanelPositionZ_inner","",16,-1.*ROOT.TMath.Pi(),1.*ROOT.TMath.Pi(),1,0,2)
        #Hist_PanelPositionZ_outer = ROOT.TH2D("Hist_PanelPositionZ_outer","",16,-1.*ROOT.TMath.Pi(),1.*ROOT.TMath.Pi(),2,0,4)
        #Hist_PanelPositionZ_inner = ROOT.TH2D("Hist_PanelPositionZ_inner","",8,-1.*ROOT.TMath.Pi(),1.*ROOT.TMath.Pi(),1,0,2)

        
        for line in outputfile:
            if "Current panel coordinates (x, y ,z xRot, yRot, zRot)" in line:
                is_an_position_block = True
                for string in range(0,len(line.split(" "))):
                    if "Panel_" in line.split(" ")[string]:
                        panel_id = line.split(" ")[string].split("_")[1].strip(",")
                continue
            if is_an_position_block:
                if not len(panel_coordinates)==6:
                    panel_coordinates += [float(line.strip(" "))]
                else:
                    is_an_position_block = False
                    panel_x = panel_coordinates[0]
                    panel_y = panel_coordinates[1]
                    panel_z = panel_coordinates[2]
                    panel_xrot = panel_coordinates[3]
                    panel_yrot = panel_coordinates[4]
                    panel_zrot = panel_coordinates[5]
                    panel_coordinates = []
                    print '%s %0.2f %0.2f %0.2f %0.2f %0.2f %0.2f'%(panel_id,panel_x,panel_y,panel_z,panel_xrot,panel_yrot,panel_zrot)
                    radius, phase = FindPanelPosition(panel_id)
                    print 'phase = %0.2f'%(phase)
                    if radius==1:
                        bin_x = Hist_PanelPositionZ_inner.GetXaxis().FindBin(phase)
                        bin_y = Hist_PanelPositionZ_inner.GetYaxis().FindBin(radius)
                        Hist_PanelPositionZ_inner.SetBinContent(bin_x,bin_y,panel_z)
                    else:
                        bin_x = Hist_PanelPositionZ_outer.GetXaxis().FindBin(phase)
                        bin_y = Hist_PanelPositionZ_outer.GetYaxis().FindBin(radius)
                        Hist_PanelPositionZ_outer.SetBinContent(bin_x,bin_y,panel_z)
        
        
        mycanvas = ROOT.TCanvas("mycanvas","mycanvas", 200, 10, 600, 600)
        ROOT.gPad.DrawFrame(-4,-4,4,4)
        Hist_PanelPositionZ_outer.SetMinimum(580)
        Hist_PanelPositionZ_outer.SetMaximum(620)
        Hist_PanelPositionZ_inner.SetMinimum(580)
        Hist_PanelPositionZ_inner.SetMaximum(620)
        #Hist_PanelPositionZ_outer.SetMinimum(600)
        #Hist_PanelPositionZ_outer.SetMaximum(640)
        #Hist_PanelPositionZ_inner.SetMinimum(600)
        #Hist_PanelPositionZ_inner.SetMaximum(640)
        Hist_PanelPositionZ_outer.Draw("COLZ POL same")
        Hist_PanelPositionZ_inner.Draw("COLZ POL same")
        mycanvas.SaveAs("position_plot.pdf")
