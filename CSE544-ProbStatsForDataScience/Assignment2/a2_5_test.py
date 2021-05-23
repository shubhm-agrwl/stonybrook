from a2_5 import steady_state_power
import numpy as np

a = np.array([[0.9,0.1,0,0],[0,0,0.5,0.5],[0,0,0.9,0.1],[0.8,0.2,0,0]])
print (steady_state_power(a))