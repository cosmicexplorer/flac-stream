.PHONY: all testc test clean distclean

CXX := g++
CXX_FLAGS := -Wall -Wextra -Werror -std=c++17

RELEASE ?= 0
ifeq ($(RELEASE),0)
CXX_FLAGS += -g -Og
NODE_GYP_FLAGS := --debug
NODE_GYP_OUT_DIR := Debug
else
CXX_FLAGS += -Ofast
NODE_GYP_OUT_DIR := Release
endif

NPM_DIR := node_modules
NPM_BIN := $(NPM_DIR)/.bin
COFFEE_CC := $(NPM_BIN)/coffee
NAN_DIR := $(NPM_DIR)/nan

DEPS_DIR := deps
FLAC_DIR := $(DEPS_DIR)/flac
FLAC_SO_DIR := $(FLAC_DIR)/src/libFLAC++/.libs
FLAC_SO := $(FLAC_SO_DIR)/libFLAC++.so

GRIN_DIR := $(DEPS_DIR)/gRin
GRIN_FILES := $(wildcard $(GRIN_DIR)/*.tpp) $(wildcard $(GRIN_DIR)/*.hpp)

NODE := /home/cosmicexplorer/tools/node/out/Debug/node
NODE_BINDINGS := binding.gyp
NODE_BUILD_DIR := build
NODE_CXX_MAKEFILE := $(NODE_BUILD_DIR)/Makefile
NODE_CXX_MODULE := $(NODE_BUILD_DIR)/$(NODE_GYP_OUT_DIR)/addon.node

DEPS := $(COFFEE_CC) $(FLAC_SO)

CPP_IN := $(wildcard *.cpp) $(wildcard *.h)
COFFEE_OUT := main.js
CPP_OUT := $(NODE_CXX_MODULE)

LIBS := FLAC++

TEST_DIR := test
TEST_COFFEE_OUT := $(addprefix $(TEST_DIR)/,test.js)
TEST_CPP_OUT := $(TEST_CPP_IN:.cpp=.o)
TEST_COFFEE_BIN := $(TEST_DIR)/test.coffee

all: $(COFFEE_OUT) $(CPP_OUT)

test: all $(TEST_COFFEE_OUT) $(COFFEE_CC)
	$(NODE) $(TEST_COFFEE_OUT)

%.js: %.coffee $(COFFEE_CC)
	$(COFFEE_CC) -bc --no-header $<

%.o: %.cpp
	$(CXX) -c $< -o $@ $(CXX_FLAGS)

$(NODE_CXX_MAKEFILE): $(NODE_BINDINGS) $(NAN_DIR)
	node-gyp configure $(NODE_GYP_FLAGS)

$(NODE_CXX_MODULE): $(NODE_CXX_MAKEFILE) $(CPP_IN) $(GRIN_FILES)
	node-gyp build $(NODE_GYP_FLAGS)

# TODO: do we link to this one, or just require libflac to be installed?
$(TEST_CPP_BIN): $(TEST_CPP_OUT) $(FLAC_SO)
	$(CXX) $^ -o $@ $(addprefix -l,$(LIBS)) $(CXX_FLAGS)

$(COFFEE_CC) $(NAN_DIR):
	npm install

$(FLAC_SO):
	./build-flac.sh "$(FLAC_DIR)"

clean:
	rm -f $(COFFEE_OUT) $(CPP_OUT) $(TEST_COFFEE_OUT) $(TEST_CPP_OUT) \
		$(TEST_CPP_BIN)
	node-gyp clean --debug
	node-gyp clean

distclean: clean
	rm -rf $(NPM_DIR)
	rm -rf $(NODE_BUILD_DIR)
