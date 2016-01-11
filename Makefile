.PHONY: all test clean distclean

CXX := g++

NPM_DIR := node_modules
NPM_BIN := $(NPM_DIR)/.bin
COFFEE_CC := $(NPM_BIN)/coffee

DEPS_DIR := deps
FLAC_DIR := $(DEPS_DIR)/flac
FLAC_SO_DIR := $(FLAC_DIR)/src/libFLAC++/.libs
FLAC_SO := $(FLAC_SO_DIR)/libFLAC++.so

DEPS := $(COFFEE_CC) $(FLAC_SO)

COFFEE_OUT :=
CPP_OUT :=

LIBS := FLAC++

TEST_DIR := test
TEST_COFFEE_OUT := $(addprefix $(TEST_DIR)/,test.js)
TEST_CPP_IN := $(wildcard $(TEST_DIR)/*.cpp)
TEST_CPP_OUT := $(TEST_CPP_IN:.cpp=.o)
TEST_CPP_BIN := $(TEST_DIR)/test
TEST_COFFEE_BIN := $(TEST_DIR)/test.coffee

all: $(COFFEE_OUT) $(CPP_OUT)

test: all $(TEST_COFFEE_OUT) $(TEST_CPP_BIN)
	$(COFFEE_CC) $(TEST_COFFEE_BIN)

%.js: %.coffee $(COFFEE_CC)
	$(COFFEE_CC) -bc --no-header $<

%.o: %.cpp
	$(CXX) -c $< -o $@

$(TEST_CPP_BIN): $(TEST_CPP_OUT) $(FLAC_SO)
	$(CXX) $^ -o $@ -L$(FLAC_SO_DIR) $(addprefix -l,$(LIBS))


$(COFFEE_CC):
	npm install

$(FLAC_SO):
	./build-flac.sh "$(FLAC_DIR)"

clean:
	rm -f $(COFFEE_OUT) $(CPP_OUT) $(TEST_COFFEE_OUT) $(TEST_CPP_OUT) \
		$(TEST_CPP_BIN)

distclean: clean
	rm -rf $(NPM_DIR)
