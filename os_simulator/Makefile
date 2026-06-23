CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude
LDFLAGS  := -lsfml-graphics -lsfml-window -lsfml-system

TARGET  := simulador_so
SRCDIR  := src
SRCS    := $(wildcard $(SRCDIR)/*.cpp)
OBJS    := $(SRCS:.cpp=.o)

.PHONY: all clean run csv

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

csv:
	@echo "0,8,2,64"   >  processos.csv
	@echo "1,4,1,32"   >> processos.csv
	@echo "2,9,3,128"  >> processos.csv
	@echo "4,5,2,48"   >> processos.csv
	@echo "6,3,1,16"   >> processos.csv
	@echo "7,6,4,80"   >> processos.csv
	@echo "CSV gerado em processos.csv"

run: all
	./$(TARGET)

clean:
	rm -f $(SRCDIR)/*.o $(TARGET)
