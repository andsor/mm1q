"""
mm1q

Simulate M/M/1 Queue at fixed service rate :math:`\mu = 1.0` and counts how
many times it returns to zero queue length within a fixed simulation time
window.

.. codeauthor:: Andreas Sorge <as@ds.mpg.de>
"""

"""
Run simulation with
  runs == 10^3 at each value of lambda
  T == 10^6 simulation time: this ensures that the standard deviation of lambda
                             around its mean is approximately 10^{-3}
  lambda_0 = 0.1
  lambda_1 = 10.0
Use geometric increments for lambda and employ 1,001 tasks to get at a
resolution of 10^{-2} * lambda.
"""

import math
import json
import subprocess
import os
import errno
import numpy as np


def smart_parameter_loop_index(index):

    n = int(index)
    if n <= 0:
        return 0.0

    # return the number of bits at the current resolution
    b = lambda n: int(math.ceil(math.log(n, 2))) if n else 0

    # return the (integer) denominator at the current resolution
    d = lambda n: int(math.pow(2, b(n))) if n else 0

    # return the index of the current position at the current resolution
    k = lambda n: int(n - d(n) / 2 - 1) if n > 1 else 0

    # reverses the bits
    bitreversal = lambda original, numbits: (
        sum(
            1 << (numbits - 1 - i) for i in range(numbits) if original >> i & 1
        ))

    # return the (integer) nominator at the current resolution
    nominator = lambda n: 2 * bitreversal(k(n), b(n) - 1) + 1

    return float(nominator(n)) / float(d(n)) if n else 0.0


def GetData(arrivalRate, serviceRate, simulationTime):
    """Retrieves the data from command line call"""
    cmd = "/home/sorge/local/bin/cmm1q -a%f -s%f -T%f" % (
        arrivalRate, serviceRate, simulationTime
    )
    data = json.loads(subprocess.Popen(
        cmd.split(), stdout=subprocess.PIPE, shell=False
    ).stdout.read().decode())
    return data['HasReturnedToZero'] == 'true', data['CurrentTime']


def main():

    mu = 1.0
    runs = int(1e3)
    simulationTime = 1e6
    lambda0 = 0.1
    lambda1 = 10.0

    task_id = int(os.getenv('SGE_TASK_ID')) - 1
    filename = (
        '/scratch02/sorge/mm1q/%i.dat' % (task_id + 1)
    )

    # create directory
    directory = os.path.dirname(filename)
    if directory:
        try:
            os.makedirs(directory)
        except OSError as exception:
            if exception.errno != errno.EEXIST:
                raise

    lambdaIndex = smart_parameter_loop_index(task_id)
    arrivalRate = lambda0 * math.pow(lambda1 / lambda0, lambdaIndex)

    returnToZeros = np.zeros(runs)
    returnTimes = np.zeros(runs)
    for run in range(0, runs):
        returnToZeros[run], returnTimes[run] = GetData(
            arrivalRate, mu, simulationTime
        )

    # write to file
    with open(filename, 'w') as f:
        # write headers
        f.write(
            "#run\tlambda\tmu\tsimulationTime\treturnTime\thasReturned\n"
        )
        for run in range(0, runs):
            f.write(
                "%u\t%f\t%f\t%f\t%f\t%i" % (
                    run, arrivalRate, mu, simulationTime,
                    returnTimes[run], returnToZeros[run]
                )
            )


if __name__ == '__main__':
    import sys

    sys.exit(main())
