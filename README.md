# spank-plugin
spank plugins for scaling GPU/CPU frequency for slurm users. 

for testing:
-

- in the login node, compile the code like this:
`gcc  -Wall -O0 -ggdb -pedantic -fPIC -o gpufreqspank.o -c gpufreqspank.c && gcc -shared -o gpufreqspank.so gpufreqspank.o` 
- in the same node, copy the `gpufreqspank.so` `cpufreqspank.so` to `/usr/lib64/slurm/`
- in the same node, add `required gpufreqspank.so` `required cpufreqspank.so` to `/etc/slurm/plugstack.conf`
- repeat the step 2 and 3 on the compute node
- reload `slurmd`
