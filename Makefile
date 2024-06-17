CXX = g++
SHARED_CXXFLAGS = -Wall -std=c++20 -Werror
DEBUG_CXXFLAGS = $(SHARED_CXXFLAGS) -g
RELEASE_CXXFLAGS = $(SHARED_CXXFLAGS) -O3 -DNDEBUG
CXXFLAGS ?= $(DEBUG_CXXFLAGS)

BUILD_TYPE ?= debug

.PHONY: all clean test

ifeq ($(BUILD_TYPE),release)
      CXXFLAGS = $(RELEASE_CXXFLAGS)
else
ifeq ($(BUILD_TYPE),test)
      CXXFLAGS = $(TEST_CXXFLAGS)
else
      CXXFLAGS = $(DEBUG_CXXFLAGS)
endif
endif

OBJ = build/$(BUILD_TYPE)/dotenv.o \
      build/$(BUILD_TYPE)/main.o
BIN = build/$(BUILD_TYPE)/dotenv

all: $(BIN)

test: $(BIN)
	@cd test; npm install --silent; npm run --silent test

build/$(BUILD_TYPE)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@

clean:
	rm -rv $(OBJ) $(BIN) test/cpp.txt test/original.txt
