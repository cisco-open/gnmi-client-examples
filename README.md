# GNMI client

[![Release](https://img.shields.io/github/v/release/cisco-ospo/oss-template?display_name=tag)](CHANGELOG.md)
[![Lint](https://github.com/cisco-ospo/oss-template/actions/workflows/lint.yml/badge.svg?branch=main)](https://github.com/cisco-open/gnmi-client-examples/actions/workflows/lint.yml)
[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-2.1-fbab2c.svg)](CODE_OF_CONDUCT.md)
[![Maintainer](https://img.shields.io/badge/Maintainer-Cisco-00bceb.svg)](MAINTAINERS.md)

## About The Project

This repo demonstrates how to use a subscribe request through gRPC Network Management Interface (gNMI) in order to retrieve interface counter statistics data with open-config models. A function is provided for this endeavour. This project will expand to other data in future releases. Support for C++ is currently provided. Potential for other language support in future releases.

## Getting Started

To get a local copy up and running follow these simple steps.

### Prerequisites and Information

These are all provided/handled for ease of use:

User can find the specification for gNMI here:  
https://github.com/openconfig/reference/blob/master/rpc/gnmi/gnmi-specification.md

User can find information regarding open-config here:  
https://github.com/openconfig/public/tree/master

User can find the protos used here (Updated protos on April 22 2024):
https://github.com/openconfig/gnmi/tree/master/proto
Things to do: cleanup and fetch during build
The commands used to get the protos were:
git clone <> followed by:
git checkout SHA:
```
git clone https://github.com/openconfig/gnmi.git
git checkout 5588964b559c9afee319909dd022b6706fe4a162
```

User might need to add more protos in the future as dependecies for these proto files can change.
The gnmi.proto file has one change made on it:
import "github.com/openconfig/gnmi/proto/gnmi_ext/gnmi_ext.proto"; -> import "gnmi_ext.proto";

We use docker containers, which handles the installation of the required packages and libraries. If the user wants to run this in their own environment, then please refer to the [Dockerfile](gnmi_client/Dockerfile) for the necessary installation packages.

### Installation

1. Clone the repo

   ```sh
   git clone https://github.com/cisco-open/gnmi-client-examples.git
   ```

## Build Docker environment

1. Run the docker build
   ```sh
   cd gnmi_client
   make gnmi-build
   ```
2. To go into docker enviornment
   ```sh
   make app-bash
   ```
3. To remake the build with any new changes, in the docker environment
   ```sh
    $ cd src
    $ make
   ```

## Usage: Gnmi Client Examples:
Following the Steps mentioned in the C++ [README](gnmi_client/src/README.md)

## Roadmap

See the [open issues](https://github.com/cisco-open/gnmi-client-examples/issues) for a list of proposed features (and known issues).

## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions Users make are **greatly appreciated**. For detailed contributing guidelines, please see [CONTRIBUTING.md](CONTRIBUTING.md)

## License

Distributed under the `Apache License, Version 2.0` License. See [LICENSE](LICENSE) for more information.

## Contact

Project Link: [https://github.com/cisco-open/gnmi-client-examples](https://github.com/cisco-open/gnmi-client-examples)

## Acknowledgements

This template was adapted from
[https://github.com/othneildrew/Best-README-Template](https://github.com/othneildrew/Best-README-Template).
