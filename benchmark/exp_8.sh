gcc -o pt.out ./exp_7.c -lpthread
gcc -o mem.out  ../mem_malloc/src/memmalloc.c  ./exp_7.c -DMEM_MALLOC   -I../mem_malloc/include -lpthread
gcc -o hc.out  ../src/hcmalloc.c  ./exp_7.c -DHC_MALLOC   -I../src -lpthread


i="0"
BASE="10000"
SLOP="10000"
THNUM="100"

while [ $i -lt 15 ]
do
    TIMES=$(($BASE+$i*$SLOP))
    echo -n $TIMES" ">>hc
    echo -n $TIMES" ">>pt
    echo -n $TIMES" ">>mem
    ./hc.out $TIMES $THNUM >>hc
    ./pt.out $TIMES $THNUM >>pt
    ./mem.out $TIMES $THNUM >>mem
    echo "" >> hc
    echo "" >> pt
    echo "" >> mem
    i=$(($i+1))
done
python3 static_analy.py 8
rm hc
rm mem
rm pt

 


