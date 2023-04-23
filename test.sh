images=("in.txt")
modes=(
#       "static,1"
#       "static,2"
#       "static,4"
#       "static,8"
#       "static,16"
#       "static,32"
#       "static,64"
      "static,128"
#       "static,256"
#       "static,512"
      "static,1024"
#       "static,2048"
#       "static,4096"
      "static,8192"
#       "static"
#       "dynamic,1"
#       "dynamic,2"
#       "dynamic,4"
#       "dynamic,8"
#       "dynamic,16"
#       "dynamic,32"
#       "dynamic,64"
      "dynamic,128"
#       "dynamic,256"
#       "dynamic,512"
      "dynamic,1024"
#       "dynamic,2048"
#       "dynamic,4096"
      "dynamic,8192"
#       "dynamic"
#       "auto"
       )
if [[ $# -ne 2 ]]; then
      echo "not enough arguments. <cpp name> and <times> required"
      exit 0
fi
TIMES=$2
g++ -fopenmp -O2 $1 -o omp4
N=${#images[@]}
for i in $(seq 1 $N); do
    reses[${#reses[@]}]=$(./omp4 -1 ${images[$i-1]} "output/$i.pgm" | pcregrep -o1 -i "([0-9]+,[0-9]+,[0-9]+),[0-9]+")
done

for m in ${modes[@]}; do
  export OMP_SCHEDULE=$m
  for THREADS in $(seq 8 8); do
    MID="0.000"
    for _ in $(seq 1 $TIMES); do
      for i in $(seq 1 $N); do
          out=$(./omp4 $THREADS ${images[$i-1]} "output/$i.pgm")
          res=$(pcregrep -o1 -i "([0-9]+,[0-9]+,[0-9]+),([0-9]+)" <<< $out)
          time=$(pcregrep -o2 -i "([0-9]+,[0-9]+,[0-9]+),([0-9]+)" <<< $out)
          if [ $res != ${reses[$i-1]} ]; then
            echo "Error! Threads: $THREADS; Mode: ($m); Input: ${images[$i-1]}; Expected: ${reses[$i-1]}; Found: $res"
          fi
          MID=$(bc <<< "$MID + $time")
      done
    done
    echo "Threads: $THREADS; Mode: ($m); Times: $TIMES; Total time: $MID; Average: $(bc <<< "scale=3;$MID / $N / $TIMES")"
  done
done

MID="0.000"
THREADS="NO_OMP"
for _ in $(seq 1 $TIMES); do
  for i in $(seq 1 $N); do
      out=$(./omp4 -1 ${images[$i-1]} "output/$i.pgm")
      res=$(pcregrep -o1 -i "([0-9]+,[0-9]+,[0-9]+),([0-9]+)" <<< $out)
      time=$(pcregrep -o2 -i "([0-9]+,[0-9]+,[0-9]+),([0-9]+)" <<< $out)
      if [ $res != ${reses[$i-1]} ]; then
        echo "Error! Threads: $THREADS; Input: ${images[$i-1]}; Expected: ${reses[$i-1]}; Found: $res"
      fi
      MID=$(bc <<< "$MID + $time")
  done
done
echo "Threads: $THREADS; Times: $TIMES; Total time: $MID; Average: $(bc <<< "scale=3;$MID / $N / $TIMES")"
