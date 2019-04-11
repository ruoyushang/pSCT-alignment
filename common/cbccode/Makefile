CXX = g++
SOURCES = $(shell echo *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

CXXFLAGS = -std=c++11 -fPIC -g -Wall -O3 -I.
LDFLAGS  = -shared

TARGET = libcbc.so

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

.PHONY: clean tar

clean:
	$(RM) *.o *.so

install: 
	cp $(TARGET) /a/home/tehanu/ap3115/mpes/lib/$(TARGET)
	chmod 755 /a/home/tehanu/ap3115/mpes/lib/$(TARGET)
	cp m_pCBC.hpp  /a/home/tehanu/ap3115/mpes/include/m_pCBC.hpp
	chmod 644 /a/home/tehanu/ap3115/mpes/include/m_pCBC.hpp

tar: 
	tar czvf libcbc.tar.gz ../libcbc/*.cpp ../libcbc/*.hpp ../libcbc/Makefile

update: 
	rm master.zip ; wget --no-check-certificate https://github.com/andrewpeck/CTA-Mirror-Control/archive/master.zip && unzip -o master.zip && mv CTA-Mirror-Control-master/* .; mv CTA-Mirror-Control-master/.gitignore .; rmdir CTA-Mirror-Control-master ; rm master.zip
