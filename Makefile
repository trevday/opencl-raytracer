TARGET := raytracer
SRCS := ./src/*.cpp
INCLUDES := ./include/*.hpp ./include/*.h
LIBRARIES := -lGLEW -lGLFW -framework OpenCL -framework OpenGL

target_location = objects/$(TARGET)

$(target_location): $(SRCS) $(INCLUDES)
	clang++ -std=c++14 $(LIBRARIES) $(SRCS) -I./include -o $(TARGET)
