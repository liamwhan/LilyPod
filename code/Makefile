#
# You will need GLFW (http://www.glfw.org):
#   brew install glfw
#

CXX = g++
#CXX = clang++

OUTDIR = ../build
EXE = $(OUTDIR)/podcast_utility
IMGUI_DIR = ./imgui
SOURCES = ./main.mm 
SOURCES += ./wav.cpp ./wav_gui.cpp $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_metal.mm
_OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
OBJS = $(patsubst %,$(OUTDIR)/%,$(_OBJS))

LIBS = -framework Metal -framework MetalKit -framework Cocoa -framework IOKit -framework CoreVideo -framework QuartzCore
LIBS += -L/usr/local/lib -lglfw

CXXFLAGS = -I. -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I/usr/local/include
CXXFLAGS += -Wall -Wformat -Wc++11-extensions
CFLAGS = $(CXXFLAGS)

$(OUTDIR)/%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUTDIR)/%.o:$(CODE_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUTDIR)/%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUTDIR)/%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUTDIR)/%.o:%.mm
	$(CXX) $(CXXFLAGS) -ObjC++ -fobjc-weak -fobjc-arc -c -o $@ $<

$(OUTDIR)/%.o:$(IMGUI_DIR)/backends/%.mm
	$(CXX) $(CXXFLAGS) -ObjC++ -fobjc-weak -fobjc-arc -c -o $@ $<

all: $(EXE)
	@echo Build complete

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
