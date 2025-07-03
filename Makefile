.PHONY: setup 

build-docker-image:
	./docker/build.sh

run-docker-container:
	./docker/run.sh

setup: build-docker-image
	sudo apt-get update -y && sudo apt-get install gdb-multiarch libusb-1.0.0-dev -y
	pip3 install --no-cache-dir platformio --user --break-system-packages
	ln -s ~/.platformio/penv/bin/platformio ~/.local/bin/pio
	pip3 install -U pyocd --user --break-system-packages
	sudo cp docker/udev/*.rules /etc/udev/rules.d
	sudo udevadm control --reload
	sudo udevadm trigger

prebuild:
	sudo chown -R `id -un` .pio/build
	git pull --rebase --autostash

build-tx: prebuild
	pio run -t clean -e TX && pio debug -e TX

build-rx: prebuild
	pio run -t clean -e RX && pio debug -e RX

build-using-container:
	docker exec rc-security-module bash -c 'cd /workspaces/rc-security-module; pio run -t clean -e TX'
	docker exec rc-security-module bash -c 'cd /workspaces/rc-security-module; pio debug -e TX'
	docker exec rc-security-module bash -c 'cd /workspaces/rc-security-module; pio run -t clean -e RX'
	docker exec rc-security-module bash -c 'cd /workspaces/rc-security-module; pio debug -e RX'

build: build-tx build-rx
	@echo 'Successfully built RC Security Module TX and RX firmwares'
