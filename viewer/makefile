CXX = g++
INCLUDE = -I./ -I./external `pkg-config --cflags glfw3`
LDFLAGS = -lGL `pkg-config --static --libs glfw3` `python3 -m pybind11 --includes`
FLAGS = -O3 -DNDEBUG -Wall -Wformat -march=native -shared -std=c++17 -fPIC -pthread
SOURCES = 	./external/gl3w/*.c \
			./core/base/*.cpp \
			./core/view/*.cpp \
			./core/data/*.cpp \
			./core/renderer/*.cpp \
			./core/*.cpp \
			./pybind/*.cpp \

make: 
	$(CXX) $(FLAGS) $(INCLUDE) $(SOURCES) $(LDFLAGS) -o ../gradient_based_editing/python/gradient_based_editing/viewer.so