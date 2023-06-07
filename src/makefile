v1:
	gcc hcmalloc.c -fPIC -shared -o libhcmalloc.so  -lpthread

v2:
	gcc hcmalloc.c -fPIC -shared -Dspin -o libhcmalloc.so  -lpthread

v3:
	gcc hcmalloc.c -fPIC -shared -DOver_alloc -o libhcmalloc.so  -lpthread

v4:
	gcc hcmalloc.c -fPIC -shared -DSafe -o libhcmalloc.so  -lpthread