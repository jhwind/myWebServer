all: demo

#指定链接器
LD      = g++
#指定编译器
CC      = g++

OUT     = ./debug/

#指定头文件搜索路径
INC     = -I ./buffer/ \
		  -I ./config/ \
		  -I ./http/ \
		  -I ./log/ \
		  -I ./pool/ \
		  -I ./server/ \
		  -I ./timer/ 

#添加要连接的库文件
LINK    = -lpthread -lmysqlclient

#添加要编译的.c文件
SRCS    = ./main.cc  \
		  ./buffer/buffer.cc  \
		  ./http/httpconn.cc  \
		  ./http/httprequest.cc  \
		  ./http/httpresponse.cc  \
		  ./log/log.cc \
		  ./pool/sqlconnpool.cc \
		  ./server/epoller.cc \
		  ./server/webserver.cc \
		  ./timer/heaptimer.cc \
		  #./MCU_BSP_Lib/src/BSP_Api_License/Lib_Update.c \

OBJS    = $(SRCS:.cc=.o)

CFLAGS  = -g -std=c++11


#$(info SRCS: $(SRCS))
#$(info OBJS: $(OBJS))

#连接，生产目标程序DemoApp
demo: $(OBJS)
	@echo link...
	$(LD)  -o $@  $^ $(LINK)
	@echo done.


#编译
%.o:%.cc                    
	$(CC) $(CFLAGS) $(INC)  -o $@ -c $<      

#clean
clean:
	rm $(OBJS)
	rm demo