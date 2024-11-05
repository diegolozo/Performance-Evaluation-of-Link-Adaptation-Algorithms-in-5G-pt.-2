import cca_perf_py as py_binding
from ns3ai_utils import Experiment
import sys
import os
import traceback
import argparse
from time import localtime, strftime  # For the time being

from lib.aqm import AqmManager, Red, CoDel, Dummy, Adaptive_Red

AQM_OPTS = {
    "red": Red,
    "ared": Adaptive_Red,
    "codel": CoDel,
    "dummy": Dummy
}

def argument_parser():
    parser = argparse.ArgumentParser(
        prog=os.path.basename(__file__),
        description="Runs cca-perf ns3 simulation using algorithms implemented in Python to manage RLC Layers's buffers",
        epilog="Thanks to ns3-ai team for their work. Made by AG-DT."
    )

    parser.add_argument("-e", "--enqueue", default="dummy", help="Enqueue Active Queue Management algorithm to be used. \
                                                                    Minimum threshold for RED and ARED can be included with the percentage\
                                                                    next to the AQM, e.g., RED90 means RED with a minimum threshold of 90%.")
    parser.add_argument("-d", "--dequeue", default="dummy",  help="Dequeue Active Queue Management algorithm to be used.\
                                                                    Time Target for CoDel can be included with the time in ms nexto to\
                                                                    the AQM, e.g., CODEL5 means CoDel with a Time Target of 5 ms.")
    parser.add_argument("--cwd", default=os.getcwd(), help="Directory where the experiment results will be saved.")
    args, unknown_args = parser.parse_known_args()
    arg_val = " ".join(unknown_args).replace("--", "-").replace("=", " ").replace("\n", "").split("-")

    experiment_args_dict = dict()
    for av in arg_val:
        _av = list(filter(lambda x: x != "", av.split(" ")))

        if len(_av) == 1:
            experiment_args_dict[_av[0]] = True
        elif len(_av) == 2:
            experiment_args_dict[_av[0]] = _av[1]

    experiment_args_dict["AQMEnq"] = args.enqueue.lower()
    experiment_args_dict["AQMDeq"] = args.dequeue.lower()
    experiment_args_dict.pop("enqueue", None)
    experiment_args_dict.pop("dequeue", None)

    return args, experiment_args_dict

def main(enq: str, deq: str, folder: str, exp_args: dict):
    VECTOR_SIZE = 32  # This is a complication | We would need to know the number of flows beforehand | Can we set it on the fly?
    NS3_PATH = os.getcwd().split("scratch")[0]  # Let's set it dynamically
    # EXEC_SETTINGS = {"useAI": True, "verbose": True}  # " --frequency=xx"
    # FOLDER = os.getcwd() + "/out/" + "ai-exp-execution-" + strftime("%Y_%m_%d-%H_%M_%S", localtime()) 
    NS3_SETTINGS = f" --cwd={folder} --no-build"
    EXP_NAME = "ccaperf-exec" + NS3_SETTINGS
    SEGMENT_HASH = str(hash(folder))
    exp_args["aiSegmentName"] = SEGMENT_HASH

    # Enqueue configs
    if args.enqueue.lower() == "dummy":
        min_th = -1
    else:
        aqm_enq_set = {"red", "ared"}
        enqueue_configs = args.enqueue.lower().split("d",1)
        enq = enqueue_configs[0] + "d"
        if enq not in aqm_enq_set:
            print(f"Given enqueue AQM algorithm {enq} invalid. Valid options: RED, ARED.")
            print(f"Aborting...")
            exit(1)
        else:
            try:
                min_th = float(enqueue_configs[1])
            except Exception as e:
                print("Exception occurred: {}".format(e))
                print("Traceback:")
                if enqueue_configs[1] == '':
                    print("Minimum threshold was left blank. You need to specify it.")
                else: 
                    print("Minimum threshold not specified correctly.")
                    print(f"Minimum threshold given: {enqueue_configs[1]}")
                exit(1)
            assert 0 <= min_th, f"Minimum threshold value given {min_th} is less than 0%."
            assert min_th < 100, f"Minimum threshold value given {min_th} is equal or greater than 100%."
            min_th = min_th/100.0

    # Dequeue configs
    if args.dequeue.lower() == "dummy":
        time_target = 0
    else:
        aqm_deq_set = {"codel"} # for now
        dequeue_configs = args.dequeue.lower().split("l",1)
        deq = dequeue_configs[0] + "l"
        if deq not in aqm_deq_set:
            print(f"Given dequeue AQM algorithm {deq} invalid. Valid options: CODEL")
            print(f"Aborting...")
            exit(1)
        else:
            try:
                time_target = float(dequeue_configs[1])
            except Exception as e:
                print("Exception occurred: {}".format(e))
                print("Traceback:")
                if dequeue_configs[1] == '':
                    print("Time target was left blank. You need to specify it.")
                else: 
                    print("Time target not specified correctly.")
                    print(f"Time target given: {dequeue_configs[1]}")
                exit(1)
            assert time_target > 0, f"Time target value given {time_target} is less than 0 ms."
            time_target = time_target/1000.0

    print(f"AQM Enq: {enq}")
    print(f"min_th: {min_th}")
    print(f"AQM Deq: {deq}")
    print(f"time target: {time_target}")

    enqueue_manager = AqmManager(AQM_OPTS[enq.lower()], folder)
    dequeue_manager = AqmManager(AQM_OPTS[deq.lower()], folder)

    if not os.path.isdir(folder):
        os.mkdir(folder)
    
    exp = Experiment(EXP_NAME, NS3_PATH, py_binding,
                    handleFinish=True, useVector=True, vectorSize=VECTOR_SIZE, segName=SEGMENT_HASH)
    msgInterface = exp.run(show_output=True, setting=exp_args)
    print(f"NS3AI Memory space name: {SEGMENT_HASH}", flush=True)

    try:

        while True:
            # receive from C++ side
            msgInterface.PyRecvBegin()
            if msgInterface.PyGetFinished():
                break

            # send to C++ side
            msgInterface.PySendBegin()
            for i in range(len(msgInterface.GetCpp2PyVector())):
                # calculate the sums
                vector_data = msgInterface.GetCpp2PyVector()[i]
                _delay = vector_data.delay
                _fillperc = vector_data.fill_perc
                _just_called = vector_data.just_called
                _simulation_time = vector_data.simulation_time
                _emptying_time = vector_data.emptying_time
                _is_buffer_full = vector_data.is_buffer_full
                _is_fragmented = vector_data.is_fragmented

                if vector_data.entering:
                    msgInterface.GetPy2CppVector()[i].drop_packet = enqueue_manager(i, 
                                                                                    delay=_delay, 
                                                                                    fill_perc=_fillperc, 
                                                                                    just_called=_just_called, 
                                                                                    simulation_time=_simulation_time,
                                                                                    emptying_time = _emptying_time,
                                                                                    min_th = min_th,
                                                                                    is_buffer_full = _is_buffer_full)

                if vector_data.exiting:
                    msgInterface.GetPy2CppVector()[i].drop_packet = dequeue_manager(i, 
                                                                                    delay=_delay, 
                                                                                    fill_perc=_fillperc, 
                                                                                    just_called=_just_called, 
                                                                                    simulation_time=_simulation_time,
                                                                                    emptying_time=_emptying_time,
                                                                                    time_target = time_target,
                                                                                    is_fragmented = _is_fragmented)
                
                vector_data.just_called = False
                
            msgInterface.PyRecvEnd()
            msgInterface.PySendEnd()

    except Exception as e:
        exc_type, exc_value, exc_traceback = sys.exc_info()
        print("Exception occurred: {}".format(e))
        print("Traceback:")
        traceback.print_tb(exc_traceback)
        exit(1)

    else:
        pass

    finally:
        print("Finally exiting...")
        del exp

if __name__ == "__main__":
    args, exp_args = argument_parser()
    main(args.enqueue, args.dequeue, args.cwd, exp_args)