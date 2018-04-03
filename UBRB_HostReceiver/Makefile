FLAGS=-O2 -Wall -fPIC

CXX=g++
CXXFLAGS=$(FLAGS)

OBJ=main.o


all: ubrb

clean:
	rm -f *.o ubrb

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $<

ubrb: $(OBJ)
	$(CXX) -o ubrb $(OBJ)
