#!/bin/bash

#request Bash
#$ -S /bin/bash

# source our profile
. ~/.profile

# get script directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# maximum number of jobs is 75,000
qsub -cwd -V -q frigg.q,skadi.q -t 1-1001 -e /scratch02/sorge/mm1q/error -o /scratch02/sorge/mm1q/output $DIR/mm1q.sh 

