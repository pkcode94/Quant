# ============================================================
# Quant - Linux / Termux Makefile
#
# Build:
#   make              # CPU-only desktop app
#   make CUDA=1       # GPU build (requires nvcc + CUDA toolkit)
#   make engine       # Build libquant-engine only
#   make clean
#
# Termux:
#   pkg install clang make
#   make CXX=clang++
#
# Dependencies:
#   C++17 compiler, pthreads, filesystem
#   (cpp-httplib, nanojson3 are header-only, vendored)
# ============================================================

CXX       ?= g++
NVCC      ?= nvcc
CXXFLAGS  ?= -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter
LDFLAGS   ?= -lpthread

# Termux / Android does not need -lstdc++fs; modern GCC/Clang
# have std::filesystem in libstdc++ by default.  Uncomment if
# your toolchain requires it:
# LDFLAGS += -lstdc++fs

SRC_DIR    := Quant
ENGINE_DIR := libquant-engine
UI_DIR     := quant-ui
BUILD_DIR  := build
TARGET     := $(BUILD_DIR)/quant
ENGINE_LIB := $(BUILD_DIR)/libquant-engine.a
UI_LIB     := $(BUILD_DIR)/libquant-ui.a

INCLUDES   := -I$(SRC_DIR) -I$(SRC_DIR)/cpp-httplib-master -Ilibquant/include
ENGINE_INC := -I$(ENGINE_DIR)/include -I$(SRC_DIR) -I$(SRC_DIR)/cpp-httplib-master -Ilibquant/include
UI_INC     := -I$(UI_DIR)/include -I$(ENGINE_DIR)/include -I$(SRC_DIR) -I$(SRC_DIR)/cpp-httplib-master -Ilibquant/include

# ---- Source files ----
SRCS       := $(SRC_DIR)/Quant.cpp
OBJS       := $(BUILD_DIR)/Quant.o
ENGINE_OBJ := $(BUILD_DIR)/quant_engine.o
UI_OBJ     := $(BUILD_DIR)/quant_ui.o

# ---- CUDA support ----
ifdef CUDA
  CXXFLAGS  += -DQUANT_CUDA
  NVCCFLAGS := -std=c++17 -O2 -DQUANT_CUDA $(INCLUDES)
  CUDA_SRCS := $(SRC_DIR)/CudaKernels.cu $(SRC_DIR)/GpuLLM.cu
  CUDA_OBJS := $(BUILD_DIR)/CudaKernels.o $(BUILD_DIR)/GpuLLM.o
  LDFLAGS   += -lcudart
else
  CUDA_OBJS :=
endif

# ============================================================
# Targets
# ============================================================

.PHONY: all engine ui clean

all: $(TARGET)
	@echo ""
	@echo "  Build complete: $(TARGET)"
ifdef CUDA
	@echo "  CUDA:  enabled"
else
	@echo "  CUDA:  disabled (use 'make CUDA=1' for GPU support)"
endif
	@echo ""

engine: $(ENGINE_LIB)
	@echo "  Engine library: $(ENGINE_LIB)"

ui: $(UI_LIB)
	@echo "  UI library: $(UI_LIB)"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ---- Engine library ----
$(BUILD_DIR)/quant_engine.o: $(ENGINE_DIR)/src/quant_engine.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ENGINE_INC) -c $< -o $@

$(ENGINE_LIB): $(ENGINE_OBJ) | $(BUILD_DIR)
	ar rcs $@ $^

# ---- UI library ----
$(BUILD_DIR)/quant_ui.o: $(UI_DIR)/src/quant_ui.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(UI_INC) -c $< -o $@

$(UI_LIB): $(UI_OBJ) | $(BUILD_DIR)
	ar rcs $@ $^

# ---- C++ compilation ----
$(BUILD_DIR)/Quant.o: $(SRC_DIR)/Quant.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# ---- CUDA compilation ----
ifdef CUDA
$(BUILD_DIR)/CudaKernels.o: $(SRC_DIR)/CudaKernels.cu | $(BUILD_DIR)
	$(NVCC) $(NVCCFLAGS) -c $< -o $@

$(BUILD_DIR)/GpuLLM.o: $(SRC_DIR)/GpuLLM.cu | $(BUILD_DIR)
	$(NVCC) $(NVCCFLAGS) -c $< -o $@
endif

# ---- Link ----
$(TARGET): $(OBJS) $(CUDA_OBJS) | $(BUILD_DIR)
	$(CXX) $(OBJS) $(CUDA_OBJS) -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)
