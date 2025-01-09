#!/ bin / bash
echo P4 directory path is $1
#cd into p4factory directory
cd $1
shift

#Always compile bf_switch
#FIXME we need the hack on the following lines until the issue
# with switch compilation(in a precompiled workspace) is resolved
##### hack ######
cd build/
rm -rf switch-tofino p4-tests bf-diags
rm .submodules.configured
cd ../
##### hack ######
echo Compiling P4 program bf_switch
./tools/compile.sh bf_switch

prog_args="--with-p4c --p4 -j4"

for n in $(seq 1 $#); do
  echo Compiling P4 program $1 with params $prog_args
  ./tools/compile.sh $1 $prog_args
  shift
done
