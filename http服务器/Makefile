
BIN=http_server #生成的可执行文件
SRC=main.cc
CC=g++
FLAGS=std=c++11 -pthread 

$(BIN):$(SRC)
	$(CC) $^ -o $@ -$(FLAGS) 

.PHONY:clean
clean:
	rm -f $(BIN)
