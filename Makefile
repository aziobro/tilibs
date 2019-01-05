all:
	cd libticonv/trunk && PKG_CONFIG_PATH=$(PKG_CONFIG_PATH):$(TRAVIS_BUILD_DIR)/prefix/lib/pkgconfig ./configure --prefix=$(TRAVIS_BUILD_DIR)/prefix && make clean install && cd ../..
	cd libtifiles/trunk && PKG_CONFIG_PATH=$(PKG_CONFIG_PATH):$(TRAVIS_BUILD_DIR)/prefix/lib/pkgconfig ./configure --prefix=$(TRAVIS_BUILD_DIR)/prefix && make clean install && cd ../..
	cd libticables/trunk && PKG_CONFIG_PATH=$(PKG_CONFIG_PATH):$(TRAVIS_BUILD_DIR)/prefix/lib/pkgconfig ./configure --prefix=$(TRAVIS_BUILD_DIR)/prefix --enable-logging --enable-libusb10 && make clean install && cd ../..
	cd libticalcs/trunk && PKG_CONFIG_PATH=$(PKG_CONFIG_PATH):$(TRAVIS_BUILD_DIR)/prefix/lib/pkgconfig ./configure --prefix=$(TRAVIS_BUILD_DIR)/prefix && make clean install && cd ../..
