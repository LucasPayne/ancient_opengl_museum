# Script for generating coefficients to use for curves and surfaces.
import sys

def multinomial_coefficient(n, indices):
    if sum(indices) != n:
        print("ERROR")
        sys.exit()
    nfac = 1
    for i in range(1,n+1):
        nfac *= i;
    indexfac = [1 for _ in indices]
    for i in range(len(indices)):
        for j in range(1,indices[i]+1):
            indexfac[i] *= j
    for t in indexfac:
        nfac = nfac // t
    return nfac

def choose(n, i):
    return multinomial_coefficient(n, [i, n - i])

num_n = 12
print("{")
for n in range(num_n+1):
    string = "    "
    for i in range(n+1):
        string += str(choose(n, i)) + ", "
    for i in range(n+1,num_n+1):
        string += "0, "
    print(string)
print("}")

num_n = 12
print("{")
for n in range(num_n+1):
    string = "    "
    for i in range(num_n+1):
        for j in range(n-i+1):
            k = n-i-j
            string += str(multinomial_coefficient(n, [i,j,k])) + ", "
        for _ in range(k+1,num_n+1):
            string += "0, "
        string += "\n        "
    print(string)
print("}")
