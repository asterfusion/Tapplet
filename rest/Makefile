all: pkg-sf-rest
	

.PHONY : pkg-sf-rest clean


pkg-sf-rest: ./dist/restd ./dist/create_ssl_pki 

./dist/restd :
	python3  -B check_import.py
	pyinstaller -F restd.py

./dist/create_ssl_pki:
	pyinstaller -F create_ssl_pki.py

clean:
	rm -rf dist
	rm -rf build
	rm -rf __pycache__
	rm -rf restd.spec create_ssl_pki.spec
	
