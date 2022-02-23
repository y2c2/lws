MAKE = make

default:
	$(MAKE) -C deps/httpparse
	$(MAKE) -C deps/qv
	$(MAKE) -C deps/libujson
	$(MAKE) -C deps/xenonjs/deps/ec
	$(MAKE) -C src

release:
	$(MAKE) -C deps/httpparse MODE=release
	$(MAKE) -C deps/qv MODE=release
	$(MAKE) -C deps/libujson MODE=release
	$(MAKE) -C deps/xenonjs/deps/ec MODE=release
	$(MAKE) -C src MODE=release

clean:
	$(MAKE) clean -C deps/httpparse
	$(MAKE) clean -C deps/qv
	$(MAKE) clean -C deps/libujson
	$(MAKE) clean -C deps/xenonjs/deps/ec
	$(MAKE) clean -C src

