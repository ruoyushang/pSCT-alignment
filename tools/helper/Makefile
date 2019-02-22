CXX=g++
CXXFLAGS=-std=c++11
OBJ=test.o actuator.o platform.o

%.o: %.cpp %.h
	$(CXX) -c -o $@ $< $(CXXFLAGS)

test: actuator.o platform.o test.o

test: $(OBJ)
	$(CXX) -o $@ $^ -lcbc -lmysqlcppconn
