CFLAGS=-g -std=c++11 `pkg-config --cflags-only-I aravis-0.6 nlopt cfitsio CCfits`
CLIBS=`pkg-config --libs aravis-0.6 nlopt cfitsio CCfits`

DEPS=AravisCamera.o StarDetect.o Image.o FitLED.o CamOutThread.o ImageStar.o

TARGETS=runGAcalfromfile runGAcalfromfits runGAcalfromRaw

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	rm *.o $(TARGETS)

runGAcalfromfile: runGAcalfromfile.C $(DEPS)
	g++ -g -o $@ $(CFLAGS) -fPIC $^ $(CLIBS)

runGAcalfromfits: runGAcalfromfits.C $(DEPS)
	g++ -g -o $@ $(CFLAGS) -fPIC $^ $(CLIBS)

runGAcalfromRaw: runGAcalfromRaw.C $(DEPS)
	g++ -g -o $@ $(CFLAGS) -fPIC $^ $(CLIBS)

%.o: %.cpp %.h
	g++ -g -c -Wpointer-arith -fpermissive -o $@ $(CFLAGS) -fPIC $< $(CLIBS)
