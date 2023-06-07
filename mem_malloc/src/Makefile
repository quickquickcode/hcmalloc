##需要日志功能时，需要把-DFILE_NAME换成可执行文件名称，例如可执行文件名称是memmalloc.out,需要把-DFILE_NAME='"log.out"换成-DFILE_NAME='"memmalloc.out"'

#标准版本，不带抗单子翻转和日志功能
version_1:
	gcc memmalloc.c -I../include -fPIC -shared -o memmalloc.so 

#不带抗单粒子翻转功能，带日志功能
version_2:
	gcc memmalloc.c log.c -DLOG -DFILE_NAME='"log.out" -I../include -fPIC -shared -o memmalloc.so 

#采用奇校验法抗单粒子翻转，带日志功能
version_3:
	gcc memmalloc.c log.c -DLOG -DFILE_NAME='"log.out" -DSEU -I../include -fPIC -shared -o memmalloc.so

#采用三模冗余法抗单粒子翻转，带日志功能
version_4: 
	gcc memmalloc.c log.c -DLOG -DFILE_NAME='"log.out" -DTMR -I../include -fPIC -shared -o memmalloc.so

#采用奇校验法抗单粒子翻转，不带日志功能
version_5: 
	gcc memmalloc.c  -DSEU -I../include -fPIC -shared -o memmalloc.so

#采用三模冗余法抗单粒子翻转，不带日志功能
version_6: 
	gcc memmalloc.c  -DTMR -I../include -fPIC -shared -o memmalloc.so

clean:
	rm -f *.o *.out *.so
