#!/usr/bin/env python3
import sys
import os
import grpc
import time
from concurrent import futures
import unittest

sys.path.append(os.path.join(os.path.dirname(__file__), "generated"))
from generated import gnmi_pb2
from generated import gnmi_pb2_grpc
from generated import gnmi_ext_pb2
from generated import gnmi_ext_pb2_grpc


# TODO: Add way to synchronize execution of the server and the client
class TestGnmi(unittest.TestCase):
    def connectivity_test(self):
        print("Testing connectivity")
        server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        server.add_insecure_port("[::]:50051")
        server.start()
        time.sleep(10)
        server.stop(0)

    def stream_initialization_test(self):
        print("Testing connectivity")
        server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        server.add_insecure_port("[::]:50051")
        server.start()
        time.sleep(10)
        server.stop(0)

    def stream_tls_initialization_test(self):
        server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        with open("example_tls/localhost.key", "rb") as f:
            server_key = f.read()
        with open("example_tls/localhost.crt", "rb") as f:
            server_crt = f.read()
        server_credentials = grpc.ssl_server_credentials(((server_key, server_crt),))
        server.add_secure_port("0.0.0.0:50051", server_credentials)
        server.start()
        time.sleep(30)
        server.stop(0)

    def stream_once_basic_test(self):
        class GnmiServicer(gnmi_pb2_grpc.gNMIServicer):
            def Subscribe(self, request, context):
                print("Received request")
                yield gnmi_pb2.SubscribeResponse()

        print("Testing connectivity")
        server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        gnmi_pb2_grpc.add_gNMIServicer_to_server(GnmiServicer(), server)
        server.add_insecure_port("[::]:50051")
        server.start()
        time.sleep(20)
        server.stop(0)

    def stream_basic_test(self):
        class GnmiServicer(gnmi_pb2_grpc.gNMIServicer):
            def Subscribe(self, request, context):
                print("Received request")
                yield gnmi_pb2.SubscribeResponse()

        print("Testing connectivity")
        server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        gnmi_pb2_grpc.add_gNMIServicer_to_server(GnmiServicer(), server)
        server.add_insecure_port("[::]:50051")
        server.start()
        time.sleep(10)

    def stream_once_custom_comm_internals_test(self):
        class GnmiServicer(gnmi_pb2_grpc.gNMIServicer):
            def Subscribe(self, request, context):
                print("Received request")
                yield gnmi_pb2.SubscribeResponse()

        print("Testing connectivity")
        server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        gnmi_pb2_grpc.add_gNMIServicer_to_server(GnmiServicer(), server)
        server.add_insecure_port("[::]:50051")
        server.start()
        time.sleep(10)

    def stream_once_wrong_key_and_policy_test(self):
        class GnmiServicer(gnmi_pb2_grpc.gNMIServicer):
            def Subscribe(self, request, context):
                print("Received request")
                yield gnmi_pb2.SubscribeResponse()

        print("Testing connectivity")
        server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        gnmi_pb2_grpc.add_gNMIServicer_to_server(GnmiServicer(), server)
        server.add_insecure_port("[::]:50051")
        server.start()
        time.sleep(20)
        server.stop(0)


if __name__ == "__main__":
    unittest.main()
