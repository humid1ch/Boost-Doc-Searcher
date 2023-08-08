.PHONY:all
all: parser searcherServerd

parser: parser.cc
	g++ -o $@ $^ -std=c++11 -lboost_system -lboost_filesystem
searcherServerd: httpServer.cc
	g++ -o $@ $^ -std=c++11 -lpthread -ljsoncpp

.PHONY:clean
clean:
	rm -rf parser searcherServerd
