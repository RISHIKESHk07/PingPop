buildmake:
	cd build;  pwd | echo ; \
	cmake --build .
run:
	cd build; \
	./game

