## Preparation

* Create a virtual environment and install the requirements `pip install -r test/integration/requirements.txt`
* From the top level directory run `python -m grpc_tools.protoc --python_out=test/integration/generated/ --grpc_python_out=test/integration/generated third_party/gnmi/protos/gnmi.proto` and `python -m grpc_tools.protoc -Ithird_party/gnmi/protos --python_out=test/integration/generated/ --grpc_python_out=test/integration/generated third_party/gnmi/protos/gnmi_ext.proto`

## Running the tests

Right now test execution has to be done manually, that is by running test case one by one, for either the client and server side. (To be synchronized, or at least automatize it in bash script in further PRs).
Example: to run ConnvectivityTest from gnmi_integration_tests.cpp, run the following command:

* Setup the server: `python3 -m unittest test_gnmi.TestGnmi.connectivity_test`
* Run the test: `gnmi_integration_test --gtest_filter=*ConnectivityTest`
