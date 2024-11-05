from typing import Any
import random
import time
import numpy as np
import os

class AqmManager:

    def __init__(self, aqm_type,folder):
      self.aqm_type = aqm_type # RED, ARED, CoDel
      self.flow_dict = dict()
      self.folder = folder

    def __call__(self, flow_id, *args: Any, **kwds: Any) -> bool:
      
      if flow_id not in self.flow_dict.keys():
        kwds["__log_folder"] = self.folder
        self.flow_dict[flow_id] = self.aqm_type(flow_id,*args, **kwds)
      
      decision = self.flow_dict[flow_id](*args, **kwds)
      
      return decision

class AqmLog:
    def __init__(self, aqm, flow_id, folder):
        self.file_name = os.path.join(folder,f"{aqm}_log_{flow_id}.csv")
        if aqm.lower() == "codel":
            self.create_codel_file()
        else:
            self.create_red_file()

    def create_red_file(self):
        first_row = "Time;Buffer Size;Average Size;Max P;Count;Drop Probability;Drop Packet;Buffer Full\n"
        with open(self.file_name, "w") as aqm_log:
            aqm_log.write(first_row)
        return

    def create_codel_file(self):
        first_row = "Time;Packet Delay;State;Count;Drop Packet;Fragmented\n"
        with open(self.file_name, "w") as aqm_log:
            aqm_log.write(first_row)
        return

    def write_red_file(self, *args: Any, **kwds: Any):
        Time = kwds["simulation_time"]
        Buffer_Size = kwds["buffer_size"]
        Average_Size = kwds["new_avg_size"]
        Max_P = kwds["max_p"]
        Count = kwds["count"]
        Drop_Probability = kwds["Pa"]
        Drop_Packet = kwds["drop_packet"]
        Buffer_Full = kwds["is_buffer_full"]
        current_row = f"{Time};{Buffer_Size};{Average_Size};{Max_P};{Count};{Drop_Probability};{Drop_Packet};{Buffer_Full}\n"
        with open(self.file_name, "a") as aqm_log:
            aqm_log.write(current_row)
        return

    def write_codel_file(self, *args: Any, **kwds: Any):
        Time = kwds["simulation_time"]
        Packet_Delay = kwds["delay"]
        State = kwds["codel_current_state"]
        Count = kwds["count"]
        Drop_Packet = kwds["drop_packet"]
        Fragmented = kwds["is_fragmented"]
        current_row = f"{Time};{Packet_Delay};{State};{Count};{Drop_Packet};{Fragmented}\n"
        with open(self.file_name, "a") as aqm_log:
            aqm_log.write(current_row)
        return

class Adaptive_Red:
    """
      Implementation of Adaptive RED algorithm for a queue.

      Attributes:
        MIN_TH (float): Minimum threshold value in percentage.
        MAX_TH (float): Maximum threshold value in percentage.
        LOWER_TARGET (float): Lower bound of the target for average queue size.
        UPPER_TARGET (float): Upper bound of the target for average queue size.
        TX_TIME (float): Transmission time for a packet (packet size / bandwith).
        DECREASE_FACTOR (float): Decrease factor to adapt max_p.
        W_Q (float): Weight associated with the instantaneous queue lenght.
        INTERVAL_UPDATE (float): Time interval to update max_p.
        last_update_time (float): Store the last update time in seconds.
        max_p (float): Maximum drop probability.
        count (int): Counts the number of packets enqueued.
        old_avg_size (float): Old calculated queue length obtained in the previous step in percentage.
        q_time (float): Time when the queue becomes empty (starts idle mode).
    """
    def __init__(self,flow_id,*args: Any, **kwds: Any) -> None:
      self.MIN_TH = kwds['min_th']
      self.MAX_TH = 1
      self.LOWER_TARGET = self.MIN_TH + 0.4 * (self.MAX_TH - self.MIN_TH)
      self.UPPER_TARGET = self.MIN_TH + 0.6 * (self.MAX_TH - self.MIN_TH)
      self.TX_TIME = 1500*8/600e6 # Mean Packet Size / DataRate
      self.DECREASE_FACTOR = 0.9
      # link_capacity = 600e6 # For 200 MHz Bandwith
      self.W_Q = 0.002 #1 - np.exp(-1/link_capacity)
      self.INTERVAL_UPDATE = 0.1
      self.last_update_time = 0
      self.max_p = 0.5
      self.count = -1
      self.old_avg_size = 0
      self.q_time = 0

      self.LOG = AqmLog("ARED", flow_id, kwds.get("__log_folder", os.getcwd()))

    def get_increment(self, max_p: float) -> float:
      """Calculate increment parameter."""
      return min(0.01, max_p/4.0)

    def update_max_p(self, new_avg_size: float, simulation_time: float) -> None:
      """Update max_p value every INTERVAL_UPDATE seconds."""
      interval = simulation_time - self.last_update_time

      if interval >= self.INTERVAL_UPDATE:
        self.last_update_time = simulation_time
        if (new_avg_size > self.UPPER_TARGET) and (self.max_p <= 0.5):
          self.max_p += self.get_increment(self.max_p)
        elif (new_avg_size < self.LOWER_TARGET) and (self.max_p >= 0.01):
          self.max_p *= self.DECREASE_FACTOR
      
      return

    def set_q_time(self, emptying_time) -> None:
      """Update q_time."""
      self.q_time = emptying_time
      return

    def calculate_average(self, buffer_size: float, simulation_time: float) -> float:
      """Calculates the average queue length based on the current buffer size."""

      if buffer_size > 0:
          new_avg_size = (1 - self.W_Q) * self.old_avg_size + self.W_Q * buffer_size

      else:
          m = (simulation_time - self.q_time)/self.TX_TIME
          new_avg_size = np.power(1-self.W_Q, m) * self.old_avg_size

      self.old_avg_size = new_avg_size

      return new_avg_size

    def calculate_probability(self, new_avg_size: float) -> float:
        """Calculates drop probability based on the calculated new average size."""

        Pb = self.max_p * (new_avg_size - self.MIN_TH) / (self.MAX_TH - self.MIN_TH)
        den = (1 - self.count * Pb)

        if den > 0:
          Pa = Pb/den
        else:
          Pa = 1

        return Pa

    def __call__(self, *args: Any, **kwds: Any) -> bool:
      """
      Implements Adaptive RED algorithm.

      Args:
        buffer_size (float): Current buffer fill percentage.
        simulation_time (float): Current simulation time in seconds.
        emptying_time (float): Time when the queue becomes empty (starts idle mode), same as q_time.
        is_buffer_full (bool): Checks if the buffer is full.

      Returns:
        drop_packet (bool): Returns if the packet has to be dropped. True if it has to be dropped, False if it has to be enqueued.
      """

      buffer_size = kwds["fill_perc"]
      simulation_time = kwds["simulation_time"]
      emptying_time = kwds["emptying_time"]
      is_buffer_full = kwds["is_buffer_full"]
      self.set_q_time(emptying_time)
      new_avg_size = self.calculate_average(buffer_size, simulation_time)

      self.update_max_p(new_avg_size, simulation_time)

      # Immediatley drop
      if is_buffer_full or (new_avg_size > self.MAX_TH):
        self.count = 0
        Pa = 1
        drop_packet = True

      # Immediatley enqueue
      elif new_avg_size <= self.MIN_TH:
        self.count = -1
        Pa = 0
        drop_packet = False

      # Calculate drop probability
      else:
        self.count += 1
        Pa = self.calculate_probability(new_avg_size)
        R = random.random()

        # Drop
        if Pa > R:
          self.count = 0
          drop_packet = True

        # Enqueue
        else:
          drop_packet = False

      self.LOG.write_red_file(simulation_time = simulation_time,
                              buffer_size = buffer_size,
                              new_avg_size = new_avg_size,
                              max_p = self.max_p,
                              count = self.count,
                              Pa = Pa,
                              drop_packet = drop_packet,
                              is_buffer_full = is_buffer_full)
      return drop_packet


class Red:
    """
      Implementation of RED algorithm for a queue.

      Attributes:
        W_Q (float): Weight associated with the instantaneous queue lenght.
        MIN_TH (float): Minimum threshold value in percentage.
        MAX_TH (float): Maximum threshold value in percentage.
        MAX_P (float): Maximum drop probability.
        TX_TIME (float): transmission time for a packet (packet size / bandwith).
        count (int): Counts the number of packets enqueued.
        old_avg_size (float): Old calculated queue length obtained in the previous step in percentage.
        q_time (float): Time when the queue becomes empty (starts idle mode).
    """
    def __init__(self,flow_id,*args: Any, **kwds: Any) -> None:
      self.MIN_TH = kwds['min_th']
      self.MAX_TH = 1
      # link_capacity = 600e6 # For 200 MHz Bandwith
      self.W_Q = 0.002 #1 - np.exp(-1/link_capacity)
      self.MAX_P = 0.5
      self.TX_TIME = 1500*8/600e6 # Mean Packet Size / DataRate
      self.count = -1
      self.old_avg_size = 0
      self.q_time = 0

      self.LOG = AqmLog("RED", flow_id, kwds.get("__log_folder", os.getcwd()))

    def set_q_time(self, emptying_time) -> None:
      """Update q_time."""
      self.q_time = emptying_time
      return

    def calculate_average(self, buffer_size: float, simulation_time: float) -> float:
      """Calculates the average queue length based on the current buffer size."""

      if buffer_size > 0:
          new_avg_size = (1 - self.W_Q) * self.old_avg_size + self.W_Q * buffer_size

      else:
          m = (simulation_time - self.q_time)/self.TX_TIME
          new_avg_size = np.power(1-self.W_Q, m) * self.old_avg_size

      self.old_avg_size = new_avg_size

      return new_avg_size

    def calculate_probability(self, new_avg_size: float) -> float:
        """Calculates drop probability based on the calculated new average size."""

        Pb = self.MAX_P * (new_avg_size - self.MIN_TH) / (self.MAX_TH - self.MIN_TH)
        den = (1 - self.count * Pb)

        if den > 0:
          Pa = Pb/den
        else:
          Pa = 1

        return Pa

    def __call__(self, *args: Any, **kwds: Any) -> bool:
      """
      Implements RED algorithm.

      Args:
        buffer_size (float): Current buffer fill percentage.
        simulation_time (float): Current simulation time in seconds.
        emptying_time (float): Time when the queue becomes empty (starts idle mode), same as q_time.
        is_buffer_full (bool): Checks if the buffer is full.

      Returns:
        drop_packet (bool): Returns if the packet has to be dropped. True if it has to be dropped, False if it has to be enqueued.
      """

      buffer_size = kwds["fill_perc"]
      simulation_time = kwds["simulation_time"]
      emptying_time = kwds["emptying_time"]
      is_buffer_full = kwds["is_buffer_full"]

      self.set_q_time(emptying_time)
      new_avg_size = self.calculate_average(buffer_size, simulation_time)

      # Immediatley drop
      if is_buffer_full or (new_avg_size > self.MAX_TH):
        self.count = 0
        Pa = 1
        drop_packet = True

      # Immediatley enqueue
      elif new_avg_size <= self.MIN_TH:
        self.count = -1
        Pa = 0
        drop_packet = False

      # Calculate drop probability
      else:
        self.count += 1
        Pa = self.calculate_probability(new_avg_size)
        R = random.random()

        # Drop
        if Pa > R:
          self.count = 0
          drop_packet = True

        # Enqueue
        else:
          drop_packet = False

      self.LOG.write_red_file(simulation_time = simulation_time,
                              buffer_size = buffer_size,
                              new_avg_size = new_avg_size,
                              max_p = self.MAX_P,
                              count = self.count,
                              Pa = Pa,
                              drop_packet = drop_packet,
                              is_buffer_full = is_buffer_full)

      return drop_packet
    

class CoDel:
    """
    Implementation of CoDel algorithm for a queue, given the delay of a outgoing packet.

    Attributes:
      TIME_TARGET (float): Maximum packet delay allowed before switching to Dropping state.
      TIME_INTERVAL (float): Time interval which CoDel will wait until switching to Dropping state and start droppin packets.
      next_drop_time (float): Time to drop the next packet based on how many packets have been dropped.
      interval_timer_starting_time (float): Stores when the interval timer started.
      interval_timer_started (float): Indicates if the interval timer has started.
      codel_states (dict): Maps the two possibles CoDel states: Non-Dropping and Dropping.
      codel_current_state (int): CoDel current state.
      count (int): Number of packets that have been dropped during Dropping state.
    """
    def __init__(self,flow_id,*args: Any, **kwds: Any) -> None:
        self.TIME_TARGET = kwds["time_target"]  # Target queue delay in seconds
        self.TIME_INTERVAL = 100/1000.0  # Interval time in seconds (100 ms)
        self.next_drop_time = 0
        self.interval_timer_starting_time = 0
        self.interval_timer_started = False
        self.codel_states = {"Dropping" : 0, "Non-dropping" : 1}
        self.codel_current_state = self.codel_states["Non-dropping"]
        self.count = 0 # Packets dropped during Dropping state
        self.next_drop_time = 0

        self.LOG = AqmLog("CoDel", flow_id, kwds.get("__log_folder", os.getcwd()))

    def packet_is_delayed(self, current_packet_delay: float) -> bool:
        """Checks if the packet delay (in seconds) is greater than the defined Target time (in seconds)."""
        return current_packet_delay > self.TIME_TARGET

    def reset_stats(self) -> None:
        """Reset parameters before exiting Dropping state."""
        self.interval_timer_starting_time = 0
        self.interval_timer_started = False
        self.count = 0
        self.next_drop_time = 0
        self.switch_state("Non-dropping")
        return

    def switch_state(self,state: str) -> None:
        """Switch CoDel state between Non-Dropping and Dropping."""
        self.codel_current_state = self.codel_states[state]
        return

    def calculate_drop_time(self, simulation_time: float) -> None:
        """Calculates the next dropping time during Dropping state, following CoDel rules."""
        val = self.TIME_INTERVAL # in seconds
        sqrt = np.sqrt(self.count)
        self.next_drop_time =  simulation_time + (val / sqrt)
        return

    def interval_timer(self, simulation_time: float) -> float:
        """Calculates the elapsed time since the interval timer started until now."""
        return simulation_time - self.interval_timer_starting_time

    def __call__(self,  *args: Any, **kwds: Any) -> bool:
        """
        Runs CoDel algorithm given the delay of a packet and decides if the packet
        has to be dropped or not.

        Args:
          packet_delay (float): Delay of the outgoing packet in seconds.
          simulation_time (float): Current simulation time in seconds.
          is_fragmented (bool): Checks if the packet is fragmented.

        Returns:
          dropping_packet (bool): Decision indicating whether the packet is dropped.
        """

        packet_delay = kwds["delay"]/1000.0 # miliseconds to seconds
        simulation_time = kwds["simulation_time"]
        is_fragmented = kwds["is_fragmented"]

        dropping_packet = False

        # Non-Dropping state
        if self.codel_current_state == self.codel_states["Non-dropping"]:

            if self.packet_is_delayed(packet_delay):

                if not self.interval_timer_started:
                  # Time interval timer starts
                  self.interval_timer_started = True
                  self.interval_timer_starting_time = simulation_time

                  # dequeue packet

                else:
                  # The timer already started

                  if self.interval_timer(simulation_time) > self.TIME_INTERVAL:
                      # Packets were delay for over 100 ms. Entering dropping
                      # phase
                      self.switch_state("Dropping")

                      # Drop the packet
                      dropping_packet = True
                      self.count+=1

                      # Calculate next dropping time
                      self.calculate_drop_time(simulation_time)

                  else:
                    pass
                    # The timer hasn't reached 100 ms, dequeue packet
            else:
              # Packet delay < 5 ms, dequeue packet
              pass
              
        # Dropping state
        else:

            if self.packet_is_delayed(packet_delay):
              if simulation_time >= self.next_drop_time:
                # Time to drop another packet
                dropping_packet = True

                self.count+=1

                # Calculate next dropping time again
                self.calculate_drop_time(simulation_time)

              else:
                # The next drop time is not yet up, dequeue packet
                pass

            else:
              # Packets are not delayed anymore. Switch to Non-dropping phase
              # and reset stats, dequeue packet
              self.reset_stats()

        self.LOG.write_codel_file(simulation_time = simulation_time,
                                  delay = packet_delay,
                                  codel_current_state = self.codel_current_state,
                                  count = self.count,
                                  drop_packet = dropping_packet,
                                  is_fragmented = is_fragmented)
        return dropping_packet
    
class Dummy:

    def __init__(self,*args: Any, **kwds: Any) -> None:
        pass

    def __call__(self, *args: Any, **kwds: Any) -> bool:
        return False