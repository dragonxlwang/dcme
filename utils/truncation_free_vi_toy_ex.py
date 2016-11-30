import numpy as np
import scipy as sp
import scipy.special
import matplotlib.pyplot as plt


def odds(x, eta_1, eta_2):
    # VI
    vi_1 = x * sp.special.psi(eta_1[0]) - x * sp.special.psi(np.sum(eta_1))
    vi_2 = x * sp.special.psi(eta_2[0]) - x * sp.special.psi(np.sum(eta_2))
    vi_odd = vi_1 - vi_2
    lc_1 = np.sum(np.log(np.arange(x) + eta_1[0])) - \
        np.sum(np.log(np.arange(x) + np.sum(eta_1)))
    lc_2 = np.sum(np.log(np.arange(x) + eta_2[0])) - \
        np.sum(np.log(np.arange(x) + np.sum(eta_2)))
    lc_odd = lc_1 - lc_2
    return (lc_odd, vi_odd)

eta_a = np.ones(10)
eta_b = np.ones(10)

# for i in range(10):
#     if(i == 0):
#         eta_a[i] = 0.1
#     else:
#         eta_a[i] = 1.0
# for i in range(10):
#     eta_b[i] = 0.1

for i in range(10):
    eta_a[i] = 1.0
for i in range(10):
    eta_b[i] = 0.1

lc_odd = np.empty(5)
vi_odd = np.empty(5)
for i in range(5):
    (x, y) = odds(i + 1, eta_a, eta_b)
    lc_odd[i] = x
    vi_odd[i] = y

print 'LC'
print lc_odd
print 'VI'
print vi_odd
plt.plot(range(0, 5), lc_odd, range(0, 5), vi_odd)
plt.show()
