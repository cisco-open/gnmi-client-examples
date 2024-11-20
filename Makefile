SHELL = /bin/bash
USER := $(shell id -un)
PWD := $(shell pwd)

IMAGE_NAME = sdk
IMAGE_TAG = latest
CONTAINER_NAME = sdk-container

DOCKER_BUILD = docker build -t $(IMAGE_NAME) .

ifeq ($(DOCKER_VOLUME),)
override DOCKER_VOLUME := "$(PWD):/sdk"
endif

ifeq ($(DOCKER_WORKDIR),)
override DOCKER_WORKDIR := "/sdk"
endif

DOCKER_RUN := docker run --name $(CONTAINER_NAME) --rm=true --privileged \
    -v $(DOCKER_VOLUME) \
    -w $(DOCKER_WORKDIR) \
    -e "http_proxy=$(http_proxy)" \
    -e "https_proxy=$(https_proxy)" \
    -i$(if $(TERM),t,)

# New clean target to remove the container and image
clean:
	-docker rm -f $(CONTAINER_NAME)
	-docker rmi -f $(IMAGE_NAME):$(IMAGE_TAG)

.PHONY: sdk-bash
.DEFAULT_GOAL := sdk-bash

build:
	$(DOCKER_BUILD) ;

sdk-bash:
	$(DOCKER_RUN) -t $(IMAGE_NAME):$(IMAGE_TAG) bash

build-mgbl-api-library: build
	$(DOCKER_RUN) -t $(IMAGE_NAME):$(IMAGE_TAG) \
	bash -c "make -C build/"

tutorial: build-mgbl-api-library
	$(DOCKER_RUN) -t $(IMAGE_NAME):$(IMAGE_TAG) \
	bash -c "make -C build/ mgbl_api_tutorial"

build-mgbl-api-in-container:
	make -C build/ && \
	make -C build/ mgbl_api_tutorial

