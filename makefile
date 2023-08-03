.PHONY:all
all: parser 

parser: parser.cc
	g++ -o $@ $^ -std=c++11 -lboost_system -lboost_filesystem


.PHONY:clean
clean:
	rm -rf parser
