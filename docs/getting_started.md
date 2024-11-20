# Getting Started

## Prerequisites

These are all provided/handled for ease of use:

User can find the specification for gNMI here:\
<https://github.com/openconfig/reference/blob/master/rpc/gnmi/gnmi-specification.md>

User can find information regarding open-config here:\
<https://github.com/openconfig/public/tree/master>

User can find the protos used here (Updated protos on April 22 2024):
<https://github.com/openconfig/gnmi/tree/master/proto>
Things to do: cleanup and fetch during build
The commands used to get the protos were:
git clone <> followed by:
git checkout SHA:

```sh
git clone https://github.com/openconfig/gnmi.git
git checkout 5588964b559c9afee319909dd022b6706fe4a162
```

User might need to add more protos in the future as dependencies for these proto files can change.
The gnmi.proto file has one change made on it:\
`import "github.com/openconfig/gnmi/proto/gnmi_ext/gnmi_ext.proto"; -> import "gnmi_ext.proto";`

>[!NOTE] We use docker containers, which handles the installation of the required packages and libraries. If the user wants to run this in their own environment, then please refer to the Dockerfile for the necessary installation packages.

## Installation

1. Download last release

   ```sh
   curl -LO https://github.com/cisco-open/gnmi-client-examples/<latest_release>
   ```

2. Include `mgbl_api` into your project.

## Build from source

1. Clone the repo.

   ```sh
   git clone https://github.com/cisco-open/gnmi-client-examples/releases/tag/<latest_release>
   ```

2. Build Docker environment.

   ```sh
   make build
   ```

3. To go into docker environment.

   ```sh
   make sdk-bash
   ```

4. To build the library.\

> By default Cmake will not build the tests
> To tell Cmake to build extra targets, do
> `cmake -DENABLE_UNIT_TESTS=ON -DENABLE_FUNC_TESTS=ON -DENABLE_SPHINX_DOC=ON ..`

   ```sh
   mkdir build && cd build
   cmake ..
   make
   ```

5. To install the lib

   ```sh
   make install
   ```

6. To build the examples.

   ```sh
    make examples
   ```

7. To run unit tests.

   ```sh
   make unit_tests
   ```

7. To create the documentation

   ```sh
   make documentation
   ```