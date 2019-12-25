all: pkg-sf-rest-cli
	

.PHONY : pkg-sf-rest-cli clean pkg-sf-rest-cli-bin

pkg-sf-rest-cli : ./dist/sfrestcli.tar.gz


./dist/sfrestcli.tar.gz :
	mkdir -p build
	mkdir -p dist
	rm -rf ./build/sf_rest_cli
	cp -r sf_rest_cli ./build
	cd ./build/sf_rest_cli ; python3 setup.py sdist
	cp ./build/sf_rest_cli/dist/sfrestcli* ./dist/sfrestcli.tar.gz

BIN_TARGET=./dist/sf_rest_cli_$(_RELEASE)_$(_VERSION).tar
pkg-sf-rest-cli-bin: $(BIN_TARGET)
$(BIN_TARGET):
	python3 -B check_import.py
	mkdir -p build
	mkdir -p dist/sf_rest_cli
	rm -rf ./build/sf_rest_cli
	cp -r sf_rest_cli ./build
	cd ./build/sf_rest_cli ; pyinstaller  -F  sf_rest_cli.py
	cp ./build/sf_rest_cli/dist/sf_rest_cli  ./dist/sf_rest_cli/
	cp ./build/sf_rest_cli/README ./dist/sf_rest_cli/
	cd ./dist ; tar cvf sf_rest_cli_$(_RELEASE)_$(_VERSION).tar sf_rest_cli
	 

clean:
	rm -rf dist
	rm -rf build