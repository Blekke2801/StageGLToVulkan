# $@ nome del target
# $^ lista delle dipendenze

CC = g++
CCFLAGS = -O3 -s -DNDEBUG

ifeq ($(OS),Windows_NT)
	BASEDIR = ../base
	INCLUDEDIRS += -I$(BASEDIR)/VulkanSDK/Include
	INCLUDEDIRS += -I$(BASEDIR)/glfw/include
	INCLUDEDIRS += -I$(BASEDIR)/glm

	LIBDIRS += -L$(BASEDIR)/glfw/lib-mingw-w64
	LIBDIRS += -L$(BASEDIR)/VulkanSDK/Lib

	LIBS += -lvulkan-1
	LIBS += -lglfw3dll

else
	LIBS += -lGLFW
	LIBS += -lvulkan
endif

OBJS = main.o shaderclass.o light.o

diffusive-light.exe : $(OBJS)
	$(CC) $(CCFLAGS) $^ $(LIBDIRS) $(LIBS) -o $@

main.o : main.cpp
	$(CC) -c $(CCFLAGS) $(INCLUDEDIRS) $? -o $@

shaderclass.o : shaderclass.cpp
	$(CC) -c $(CCFLAGS) $(INCLUDEDIRS) $? -o $@

light.o : light.cpp
	$(CC) -c $(CCFLAGS) $(INCLUDEDIRS) $? -o $@
.PHONY: clean
clean:
	rm -f *.o *.exe
