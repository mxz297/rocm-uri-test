libtest.so: libtest.c
	gcc -o libtest.so -fpic -shared libtest.c \
		-I/opt/rocm/include \
		-I/opt/rocm/include/roctracer \
		-L/opt/rocm/lib \
		-lrocm-dbgapi -ldl -lroctracer64
