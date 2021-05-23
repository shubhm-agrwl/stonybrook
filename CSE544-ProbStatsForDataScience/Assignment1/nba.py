import numpy as np

def simulation(N):
  def ques_a(p_lac=0.5, N=1):
    p_lac_win_3 = np.sum(np.random.binomial(4, p_lac, N)==3)/N
    return p_lac_win_3

  def ques_c(p_den=0.5, N=1):
    p_b_win_5th_match = np.sum(np.random.binomial(1, p_den, N)==1)/N
    p_b_win_6th_match = np.sum(np.random.binomial(1, p_den, N)==1)/N
    p_b_win_7th_match = np.sum(np.random.binomial(1, p_den, N)==1)/N
    p_b_win = p_b_win_5th_match * p_b_win_6th_match * p_b_win_7th_match
    return p_b_win

  def ques_e(N=1):
    p_b_win_5th_match = np.sum(np.random.binomial(1, 0.25, N)==1)/N
    p_b_win_6th_match = np.sum(np.random.binomial(1, 0.75, N)==1)/N
    p_b_win_7th_match = np.sum(np.random.binomial(1, 0.25, N)==1)/N
    p_b_win = p_b_win_5th_match * p_b_win_6th_match * p_b_win_7th_match
    return p_b_win


  print('For N =', N, "the simulated value for (a) is", ques_a(N=N))
  print('For N =', N, "the simulated value for (c) is", ques_c(N=N))
  print('For N =', N, "the simulated value for (e) is", ques_e(N=N))



for i in [10**3,10**4,10**5,10**6,10**7]:
    simulation(i)
