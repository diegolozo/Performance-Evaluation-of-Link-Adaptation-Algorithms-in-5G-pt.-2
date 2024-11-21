from typing import Any
import random
import time
import numpy as np
import re
import os


class AmcManager:

    def __init__(self, amc_type,folder):
      self.amc_type = amc_type # exp moving average and Q-Learning
      self.flow_dict = dict()
      self.folder = folder

    def __call__(self, flow_id, *args: Any, **kwds: Any) -> bool:
      if flow_id not in self.flow_dict.keys():
        kwds["__log_folder"] = self.folder
        self.flow_dict[flow_id] = self.amc_type(flow_id,*args, **kwds)
      
      final_value = self.flow_dict[flow_id](*args, **kwds)
      
      return final_value

class AmcLog:
    def __init__(self, amc, flow_id, folder):
        self.file_name = os.path.join(folder,f"{amc}_log_{flow_id}.csv")
        self.create_log_file()

    def create_log_file(self):
        first_row = "Time;SINR Eff;BLER Target;Gamma\n"
        with open(self.file_name, "w") as amc_log:
            amc_log.write(first_row)
        return

    def write_log_file(self, *args: Any, **kwds: Any):
        Time = kwds["simulation_time"]
        SINR_Eff = kwds["sinr_eff"]
        BLER_Target = kwds["bler_target"]
        Gamma = kwds["gamma"]
        current_row = f"{Time};{SINR_Eff};{BLER_Target};{Gamma}\n"
        with open(self.file_name, "a") as amc_log:
            amc_log.write(current_row)
        return

class Exp_Mean_Bler:
    """
      Implementation of Exponential Mobile Mean Bler Target algorithm.

      Attributes:

    """
    def __init__(self,flow_id,*args: Any, **kwds: Any) -> None:
      self.SMOOTH_FACTOR = 0.2
      self.ALPHA = 0.3
      self.BETA = -0.08
      self.FIXED_BLER_TARGET = 0.1
      self.gamma_smoothed = 10
      self.LOG = AmcLog("Exp_Mean_Bler", flow_id, kwds.get("__log_folder", os.getcwd()))

    def __call__(self, *args: Any, **kwds: Any) -> bool:
      simulation_time = kwds["simulation_time"]
      sinr_eff_db = kwds['sinr_eff']
      sinr_eff_db = min(sinr_eff_db, 30) 

      new_gamma_smoothed = self.SMOOTH_FACTOR * sinr_eff_db + (1 - self.SMOOTH_FACTOR) * self.gamma_smoothed
      self.gamma_smoothed = new_gamma_smoothed 

      if sinr_eff_db <= new_gamma_smoothed:
        bler_target = self.ALPHA * np.exp(self.BETA * sinr_eff_db)
      else:
        bler_target = self.FIXED_BLER_TARGET

      self.LOG.write_log_file(simulation_time = simulation_time,
                              sinr_eff = sinr_eff_db,
                              bler_target = bler_target,
                              gamma = new_gamma_smoothed
                            )      
      return bler_target

class Q_Learn_BLER:
    """
      Implementation of Q-Learning Bler Target algorithm.

      Attributes:

    """
    def __init__(self,flow_id,*args: Any, **kwds: Any):
      self.ALPHA = 0.3
      self.BETA = -0.08
      self.FIXED_BLER_TARGET = 0.3

      self.gamma_values = np.linspace(8, 12, num=20)
      self.Q_table = np.zeros(len(self.gamma_values))
      self.learn_rate = 0.3
      self.gamma_discount = 0.9

      self.LOG = AmcLog("Q-Learn_Bler", flow_id, kwds.get("__log_folder", os.getcwd()))
      self.folder = kwds["__log_folder"]

    def extract_instant_throughput(self, file_path):
        instant_throughput = None
        flow2_found = False
        pattern_flow2 = r"Flow 2"
        pattern_throughput = r"instantThroughput: ([\d\.]+) Mbps"

        try:
            with open(file_path, 'r') as file:
                for line in file:
                    if re.search(pattern_flow2, line):
                        flow2_found = True  
                    if flow2_found:
                        match = re.search(pattern_throughput, line)
                        if match:
                            instant_throughput = float(match.group(1)) 
                            break
        except FileNotFoundError:
            instant_throughput = None
        except Exception as e:
            print(f"Error al leer el archivo: {e}")

        return instant_throughput

    def choose_gamma(self):
        if np.random.rand() < 0.1:
            return np.random.randint(len(self.gamma_values))
        else:
            return np.argmax(self.Q_table)


    def update_q(self, gamma_index, reward):
        max_future_q = np.max(self.Q_table)
        self.Q_table[gamma_index] += self.learn_rate * (reward + self.gamma_discount * max_future_q - self.Q_table[gamma_index])

    def bler_reward(self, sinr_eff_db, gamma):
        if sinr_eff_db <= gamma:
          bler_estimated = self.ALPHA * np.exp(self.BETA * sinr_eff_db)
        else:
          bler_estimated = self.FIXED_BLER_TARGET
        
        reward = -abs(bler_estimated - self.FIXED_BLER_TARGET)  

        if abs(reward) < 0.01:
            reward += 1  

        if gamma < 8 or gamma > 12:
            penalty = abs(10 - gamma) * 0.1  
            reward -= penalty

        
        return reward, bler_estimated

    def __call__(self, *args: Any, **kwds: Any) -> bool:

      file_path = os.path.join(self.folder, "FlowOutput.txt")
      throughput = self.extract_instant_throughput(file_path)
      if throughput is None:
          throughput = 0
      simulation_time = kwds["simulation_time"]
      sinr_eff_db = kwds['sinr_eff']
      sinr_eff_db = min(sinr_eff_db, 30) 

      gamma_index = self.choose_gamma()
      gamma = self.gamma_values[gamma_index]
      reward, bler_estimated = self.bler_reward(sinr_eff_db, gamma)
      reward += throughput
      self.update_q(gamma_index, reward)

      self.LOG.write_log_file(simulation_time = simulation_time,
                              sinr_eff = sinr_eff_db,
                              bler_target = bler_estimated,
                              gamma = gamma
                              )      
      return bler_estimated