all: compile

compile: integrate

integrate: main.cpp
	$(CXX) -std=c++2a -I../../.. $(if $(EXPR),-DEXPR='$(EXPR)',-lmatheval) $(CXXFLAGS) $< -o $@

clean:
	rm -f integrate

rebuild: clean compile
