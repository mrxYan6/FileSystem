# 变量定义
target = main
objs = fs.o table.o disk.o main.o
CC = gcc  # C编译器
CXX = g++ # C++编译器
CFLAGS =  # C文件的编译选项
CXXFLAGS = # C++文件的编译选项

# 最终目标
main: $(objs)
		$(CXX) -o $(target) $(objs)

# C文件的编译规则
fs.o: fs.c
		$(CC) -c $(CFLAGS) fs.c

table.o: table.c
		$(CC) -c $(CFLAGS) table.c

disk.o: disk.c
		$(CC) -c $(CFLAGS) disk.c

# C++文件的编译规则
main.o: main.cpp
		$(CXX) -c $(CXXFLAGS) main.cpp

# 清理编译生成的文件
clean:
		rm -f $(target) $(objs)
