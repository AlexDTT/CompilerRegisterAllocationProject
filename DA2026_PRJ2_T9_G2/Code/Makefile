CXX      := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -I. -MMD -MP

TARGET := register_alloc
TARGET_SOURCES := \
	main.cpp \
	ui/RegisterAllocApp.cpp \
	io/FileParser.cpp \
	io/OutputWriter.cpp \
	data_structures/InterferenceGraph.cpp \
	algorithms/GraphColoring.cpp \
	services/AllocationLogic.cpp
TARGET_OBJECTS := $(TARGET_SOURCES:.cpp=.o)

UNIT_TEST_TARGET := register_alloc_tests
UNIT_TEST_SOURCES := \
	tests/register_alloc_tests.cpp \
	io/FileParser.cpp \
	io/OutputWriter.cpp \
	data_structures/InterferenceGraph.cpp \
	algorithms/GraphColoring.cpp \
	services/AllocationLogic.cpp
UNIT_TEST_OBJECTS := $(UNIT_TEST_SOURCES:.cpp=.o)

ALL_DEPS := $(TARGET_OBJECTS:.o=.d) $(UNIT_TEST_OBJECTS:.o=.d)

.PHONY: all unit-test integration-test test run clean docs

all: $(TARGET)

$(TARGET): $(TARGET_OBJECTS)
	$(CXX) $(CXXFLAGS) $(TARGET_OBJECTS) -o $(TARGET)

unit-test: $(UNIT_TEST_TARGET)
	./$(UNIT_TEST_TARGET)

$(UNIT_TEST_TARGET): $(UNIT_TEST_OBJECTS)
	$(CXX) $(CXXFLAGS) $(UNIT_TEST_OBJECTS) -o $(UNIT_TEST_TARGET)

integration-test: $(TARGET)
	bash tests/run_integration_tests.sh

test: unit-test integration-test

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

batch: $(TARGET)
	./$(TARGET) -b inputs/example_ranges.txt inputs/example_config.txt allocation.txt

docs:
	doxygen Doxyfile

clean:
	rm -f $(TARGET_OBJECTS) $(UNIT_TEST_OBJECTS) $(TARGET) $(UNIT_TEST_TARGET)
	rm -f $(ALL_DEPS)
	rm -f allocation.txt interference.dot
	rm -rf docs/html
	rm -rf tests/output/generated

-include $(ALL_DEPS)
