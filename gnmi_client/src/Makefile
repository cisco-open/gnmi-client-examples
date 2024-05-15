PROTOS_PATH = ../protos/gnmi/generated

CC = g++
CFLAGS = -std=c++11 -I$(PROTOS_PATH)
LDFLAGS = -L/usr/local/lib `pkg-config --libs grpc++ grpc nlohmann_json` -lprotobuf -lpthread

SRCS = gnmi_client.cpp $(PROTOS_PATH)/gnmi_ext.pb.cc $(PROTOS_PATH)/gnmi_ext.grpc.pb.cc $(PROTOS_PATH)/gnmi.pb.cc $(PROTOS_PATH)/gnmi.grpc.pb.cc
OBJS = $(SRCS:.cpp=.o)
EXEC = gnmi_client
all: buildproto $(EXEC)

buildproto:
	cd ../ && \
	protoc -I protos/gnmi --cpp_out=protos/gnmi/generated --grpc_out=protos/gnmi/generated --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` protos/gnmi/gnmi_ext.proto && \
	protoc -I protos/gnmi --cpp_out=protos/gnmi/generated --grpc_out=protos/gnmi/generated --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` protos/gnmi/gnmi.proto

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXEC)
