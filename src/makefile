version_1:
	gcc hcmalloc.c  -fPIC -shared -o libhcmalloc.so  -lpthread


version_2:
	gcc hcmalloc.c  -shared -o libhcmalloc.so  -lpthread

3:
	gcc -o my_program main.c  ./hcmalloc.c -lpthread

4:
	gcc -o v4 hcmalloc.c -lpthread
	./v4
	./v4
	./v4
	./v4
	./v4



5:
	gcc -O3 -o v5 hcmalloc.c -lpthread
	./v5
	./v5
	./v5
	./v5
	./v5


mutex:
	gcc -o mutex_test hcmalloc.c  -lpthread
	./mutex_test
	./mutex_test
	./mutex_test
	./mutex_test
	./mutex_test
	

spin: 
	gcc  -o spin_test hcmalloc.c  -Dspin -lpthread
	./spin_test
	./spin_test
	./spin_test
	./spin_test
	./spin_test