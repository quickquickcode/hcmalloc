gcc -o pt.out ./exp_5.c -lpthread
gcc -o mem.out  ../mem_malloc/src/memmalloc.c  ./exp_5.c -DMEM_MALLOC   -I../mem_malloc/include -lpthread
gcc -o hc.out  ../src/hcmalloc.c  ./exp_5.c -DHC_MALLOC   -I../src -lpthread


i="1"
BASE="10000"
SLOP="10000"
THNUM="1"

while [ $i -lt 17 ]
do
    TIMES=$(($BASE+$i*$SLOP))
    echo -n $(($i*64))" ">>hc
    echo -n $(($i*64))" ">>pt
    echo -n $(($i*64))" ">>mem
    ./hc.out $(($i*64)) $THNUM >>hc
    ./pt.out $(($i*64)) $THNUM >>pt
    ./mem.out $(($i*64)) $THNUM >>mem
    echo "" >> hc
    echo "" >> pt
    echo "" >> mem
    i=$(($i+1))
done
python3 static_analy.py 5
rm pt
rm hc
rm mem

 


