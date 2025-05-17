# $@ nome del target
# $^ lista delle dipendenze

CC = g++
CCFLAGS = -O3 -s -DNDEBUG

ifeq ($(OS),Windows_NT)
	BASEDIR = ../base
	INCLUDEDIRS += -I$(BASEDIR)/VulkanSDK/Include
	INCLUDEDIRS += -I$(BASEDIR)/glfw/include
	INCLUDEDIRS += -I$(BASEDIR)/glm
	INCLUDEDIRS += -I$(BASEDIR)/stbImage

	LIBDIRS += -L$(BASEDIR)/glfw/lib-mingw-w64
	LIBDIRS += -L$(BASEDIR)/VulkanSDK/Lib
	LIBDIRS += -L$(BASEDIR)/stbImage

	LIBS += -lvulkan-1
	LIBS += -lglfw3dll

else
	LIBS += -lGLFW
	LIBS += -lvulkan
endif

OBJS = main.o bufferUtils.o texture.o mesh.o shaderclass.o light.o

textures.exe : $(OBJS)
	$(CC) $(CCFLAGS) $^ $(LIBDIRS) $(LIBS) -o $@

main.o : main.cpp
	$(CC) -c $(CCFLAGS) $(INCLUDEDIRS) $? -o $@

bufferUtils.o : bufferUtils.cpp
	$(CC) -c $(CCFLAGS) $(INCLUDEDIRS) $? -o $@

shaderclass.o : shaderclass.cpp
	$(CC) -c $(CCFLAGS) $(INCLUDEDIRS) $? -o $@

light.o : light.cpp
	$(CC) -c $(CCFLAGS) $(INCLUDEDIRS) $? -o $@

mesh.o : mesh.cpp
	$(CC) -c $(CCFLAGS) $(INCLUDEDIRS) $? -o $@

texture.o : texture.cpp
	$(CC) -c $(CCFLAGS) $(INCLUDEDIRS) $? -o $@
.PHONY: clean
clean:
	rm -f *.o *.exe
