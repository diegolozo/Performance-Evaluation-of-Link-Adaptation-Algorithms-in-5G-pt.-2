from typing import Any
import random
import time
import numpy as np
import os


class AmcManager:

    def __init__(self, amc_type,folder):
      print(f"VOY A CREAR LA CLASE {amc_type}")
      self.amc_type = amc_type # exp, hybrid, moving average
      self.flow_dict = dict()
      self.folder = folder

    def __call__(self, flow_id, *args: Any, **kwds: Any) -> bool:
      print(f"EJECTUANDO LA CLASE {self.amc_type}")
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

class Hybrid_Bler:

    def __init__(self,flow_id,*args: Any, **kwds: Any) -> None:
      print("AMC.PY: CREE UN HYBRID_BLER")
      self.SINR_EFF_DB = kwds['sinr_eff']
      self.ALPHA = 0.3
      self.BETA = -0.08
      self.GAMMA = 10.0
      self.FIXED_BLER_TARGET = 0.1
      self.LOG = AmcLog("Hybrid_Bler", flow_id, kwds.get("__log_folder", os.getcwd()))

    def __call__(self, *args: Any, **kwds: Any) -> bool:
      simulation_time = kwds["simulation_time"]

      if self.SINR_EFF_DB <= self.GAMMA:
        bler_target = self.ALPHA * np.exp(self.BETA * self.SINR_EFF_DB)
      else:
        bler_target = self.FIXED_BLER_TARGET

      self.LOG.write_log_file(simulation_time = simulation_time,
                              sinr_eff = self.SINR_EFF_DB,
                              bler_target = bler_target
                              )      
      print("AMC.PY: CALCULADO EL BLER TARGET")
      return bler_target



# class Adaptive_Red:
#     """
#       Implementation of Adaptive RED algorithm for a queue.

#       Attributes:
#         MIN_TH (float): Minimum threshold value in percentage.
#         MAX_TH (float): Maximum threshold value in percentage.
#         LOWER_TARGET (float): Lower bound of the target for average queue size.
#         UPPER_TARGET (float): Upper bound of the target for average queue size.
#         TX_TIME (float): Transmission time for a packet (packet size / bandwith).
#         DECREASE_FACTOR (float): Decrease factor to adapt max_p.
#         W_Q (float): Weight associated with the instantaneous queue lenght.
#         INTERVAL_UPDATE (float): Time interval to update max_p.
#         last_update_time (float): Store the last update time in seconds.
#         max_p (float): Maximum drop probability.
#         count (int): Counts the number of packets enqueued.
#         old_avg_size (float): Old calculated queue length obtained in the previous step in percentage.
#         q_time (float): Time when the queue becomes empty (starts idle mode).
#     """
#     def __init__(self,flow_id,*args: Any, **kwds: Any) -> None:
#       self.MIN_TH = kwds['min_th']
#       self.MAX_TH = 1
#       self.LOWER_TARGET = self.MIN_TH + 0.4 * (self.MAX_TH - self.MIN_TH)
#       self.UPPER_TARGET = self.MIN_TH + 0.6 * (self.MAX_TH - self.MIN_TH)
#       self.TX_TIME = 1500*8/600e6 # Mean Packet Size / DataRate
#       self.DECREASE_FACTOR = 0.9
#       # link_capacity = 600e6 # For 200 MHz Bandwith
#       self.W_Q = 0.002 #1 - np.exp(-1/link_capacity)
#       self.INTERVAL_UPDATE = 0.1
#       self.last_update_time = 0
#       self.max_p = 0.5
#       self.count = -1
#       self.old_avg_size = 0
#       self.q_time = 0

#       self.LOG = AqmLog("ARED", flow_id, kwds.get("__log_folder", os.getcwd()))

#     def get_increment(self, max_p: float) -> float:
#       """Calculate increment parameter."""
#       return min(0.01, max_p/4.0)

#     def update_max_p(self, new_avg_size: float, simulation_time: float) -> None:
#       """Update max_p value every INTERVAL_UPDATE seconds."""
#       interval = simulation_time - self.last_update_time

#       if interval >= self.INTERVAL_UPDATE:
#         self.last_update_time = simulation_time
#         if (new_avg_size > self.UPPER_TARGET) and (self.max_p <= 0.5):
#           self.max_p += self.get_increment(self.max_p)
#         elif (new_avg_size < self.LOWER_TARGET) and (self.max_p >= 0.01):
#           self.max_p *= self.DECREASE_FACTOR
      
#       return

#     def set_q_time(self, emptying_time) -> None:
#       """Update q_time."""
#       self.q_time = emptying_time
#       return

#     def calculate_average(self, buffer_size: float, simulation_time: float) -> float:
#       """Calculates the average queue length based on the current buffer size."""

#       if buffer_size > 0:
#           new_avg_size = (1 - self.W_Q) * self.old_avg_size + self.W_Q * buffer_size

#       else:
#           m = (simulation_time - self.q_time)/self.TX_TIME
#           new_avg_size = np.power(1-self.W_Q, m) * self.old_avg_size

#       self.old_avg_size = new_avg_size

#       return new_avg_size

#     def calculate_probability(self, new_avg_size: float) -> float:
#         """Calculates drop probability based on the calculated new average size."""

#         Pb = self.max_p * (new_avg_size - self.MIN_TH) / (self.MAX_TH - self.MIN_TH)
#         den = (1 - self.count * Pb)

#         if den > 0:
#           Pa = Pb/den
#         else:
#           Pa = 1

#         return Pa

#     def __call__(self, *args: Any, **kwds: Any) -> bool:
#       """
#       Implements Adaptive RED algorithm.

#       Args:
#         buffer_size (float): Current buffer fill percentage.
#         simulation_time (float): Current simulation time in seconds.
#         emptying_time (float): Time when the queue becomes empty (starts idle mode), same as q_time.
#         is_buffer_full (bool): Checks if the buffer is full.

#       Returns:
#         drop_packet (bool): Returns if the packet has to be dropped. True if it has to be dropped, False if it has to be enqueued.
#       """

#       buffer_size = kwds["fill_perc"]
#       simulation_time = kwds["simulation_time"]
#       emptying_time = kwds["emptying_time"]
#       is_buffer_full = kwds["is_buffer_full"]
#       self.set_q_time(emptying_time)
#       new_avg_size = self.calculate_average(buffer_size, simulation_time)

#       self.update_max_p(new_avg_size, simulation_time)

#       # Immediatley drop
#       if is_buffer_full or (new_avg_size > self.MAX_TH):
#         self.count = 0
#         Pa = 1
#         drop_packet = True

#       # Immediatley enqueue
#       elif new_avg_size <= self.MIN_TH:
#         self.count = -1
#         Pa = 0
#         drop_packet = False

#       # Calculate drop probability
#       else:
#         self.count += 1
#         Pa = self.calculate_probability(new_avg_size)
#         R = random.random()

#         # Drop
#         if Pa > R:
#           self.count = 0
#           drop_packet = True

#         # Enqueue
#         else:
#           drop_packet = False

#       self.LOG.write_red_file(simulation_time = simulation_time,
#                               buffer_size = buffer_size,
#                               new_avg_size = new_avg_size,
#                               max_p = self.max_p,
#                               count = self.count,
#                               Pa = Pa,
#                               drop_packet = drop_packet,
#                               is_buffer_full = is_buffer_full)
#       return drop_packet


