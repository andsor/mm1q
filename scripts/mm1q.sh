#!/bin/bash

# request Bash
#$ -S /bin/bash

# source our profile
source ~/.profile

#export PYTHONPATH=$PYTHONPATH:/home/sorge/repos/mm1q/scripts

/usr/nld/bin/python -O /home/sorge/repos/mm1q/scripts/mm1q.py

