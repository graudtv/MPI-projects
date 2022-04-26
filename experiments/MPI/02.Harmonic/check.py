#!usr/bin/env python3

import sys
import subprocess

def summ(N):
        res = 0
        for i in range(1, N + 1):
                res += 1 / i
        return res


def test(N, nproc = 4):
        process = subprocess.run(['mpirun', '-n', str(nproc), './prog', '-q', str(N)],
                     capture_output=True, text=True)
        if process.returncode != 0:
                print(f"test({N})\t\t -- FAILED: Process returned with non-zero status")
                print("PROCESS STDERR CONTENTS:")
                print(process.stderr)
                return
        res = float(process.stdout)
        correct_res = summ(N)
        if abs(res - correct_res) <= 0.00001:
                print(f"test({N}) = {res}\t\t -- OK")
                return
        else:
                print(f"test({N}) = {res}\t\t -- FAILED: correct result = {correct_res}")
                return

def main():
        test(10)
        test(100)
        test(101)
        test(1000)
        test(10000)
        test(100000, 20)
        test(10000000, 10)

if __name__ == '__main__':
        main()