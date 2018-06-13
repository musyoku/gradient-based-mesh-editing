CXX = g++
INCLUDE = `pkg-config --cflags glfw3`
LDFLAGS = `pkg-config --static --libs glfw3` `python3 -m pybind11 --includes`
FLAGS = -O3 -DNDEBUG -Wall -Wformat -march=native -shared -std=c++14 -fPIC
SOURCES = 	./cpp/core/*.cpp
EXTENSION = `python3-config --extension-suffix`

UNAME := $(shell uname -s)
ifeq ($(UNAME), Darwin)
	LDFLAGS += -undefined dynamic_lookup
endif

make: 
	$(CXX) $(FLAGS) $(INCLUDE) $(SOURCES) $(LDFLAGS) -o python/gradient_based_editing/rasterizer/rasterize_cpu$(EXTENSION)