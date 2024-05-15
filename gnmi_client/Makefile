SHELL = /bin/bash
USER := $(shell id -un)
PWD := $(shell pwd)

IMAGE_NAME = app
IMAGE_TAG = latest
CONTAINER_NAME = app-container

DOCKER_BUILD = docker build -t $(IMAGE_NAME) .

ifeq ($(DOCKER_VOLUME),)
override DOCKER_VOLUME := "$(PWD):/app"
endif

ifeq ($(DOCKER_WORKDIR),)
override DOCKER_WORKDIR := "/app"
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

.PHONY: app-bash
.DEFAULT_GOAL := app-bash

build:
	$(DOCKER_BUILD) ;

app-bash:
	$(DOCKER_RUN) -t $(IMAGE_NAME):$(IMAGE_TAG) bash

gnmi-build: build
	$(DOCKER_RUN) -t $(IMAGE_NAME):$(IMAGE_TAG) \
	bash -c "make -C src/ all"

