from typing import Any
import random
import time
import numpy as np
import os


class AmcManager:

    def __init__(self, amc_type,folder):
      self.amc_type = amc_type # exp, hybrid, moving average
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
        first_row = "Time;SINR Eff;BLER Target\n"
        with open(self.file_name, "w") as amc_log:
            amc_log.write(first_row)
        return

    def write_log_file(self, *args: Any, **kwds: Any):
        Time = kwds["simulation_time"]
        SINR_Eff = kwds["sinr_eff"]
        BLER_Target = kwds["bler_target"]
        current_row = f"{Time};{SINR_Eff};{BLER_Target}\n"
        with open(self.file_name, "a") as amc_log:
            amc_log.write(current_row)
        return

class Python_Bler:
    """
      Implementation of Python Bler Target algorithm.

      Attributes:
        ALPHA (float)
        BETA (float)
        GAMMA (float)
        FIXED_BLER_TARGET (float)
    """
    def __init__(self,flow_id,*args: Any, **kwds: Any) -> None:
      self.ALPHA = 0.3
      self.BETA = -0.08
      self.GAMMA = 10.0
      self.FIXED_BLER_TARGET = 0.1
      self.LOG = AmcLog("Python_Bler", flow_id, kwds.get("__log_folder", os.getcwd()))

    def __call__(self, *args: Any, **kwds: Any) -> bool:
      simulation_time = kwds["simulation_time"]
      sinr_eff_db = kwds['sinr_eff']

      if sinr_eff_db <= self.GAMMA:
        bler_target = self.ALPHA * np.exp(self.BETA * sinr_eff_db)
      else:
        bler_target = self.FIXED_BLER_TARGET

      self.LOG.write_log_file(simulation_time = simulation_time,
                              sinr_eff = sinr_eff_db,
                              bler_target = bler_target
                              )      
      return bler_target