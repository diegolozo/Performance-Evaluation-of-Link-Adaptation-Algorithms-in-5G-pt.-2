import cca_perf_py as py_binding
from ns3ai_utils import Experiment
import sys
import os
import traceback
import argparse
from time import localtime, strftime  # For the time being

from lib.aqm import AqmManager, Red, CoDel, Dummy, Adaptive_Red


def argument_parser():
    parser = argparse.ArgumentParser(
        prog=os.path.basename(__file__),
        description="",
        epilog="Thanks to ns3-ai team for their work. Made by AG-DT."
    )


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


    return args, experiment_args_dict

def main(folder: str, exp_args: dict):
    VECTOR_SIZE = 32  # This is a complication | We would need to know the number of flows beforehand | Can we set it on the fly?
    NS3_PATH = os.getcwd().split("scratch")[0]  # Let's set it dynamically
    # EXEC_SETTINGS = {"useAI": True, "verbose": True}  # " --frequency=xx"
    # FOLDER = os.getcwd() + "/out/" + "ai-exp-execution-" + strftime("%Y_%m_%d-%H_%M_%S", localtime()) 
    NS3_SETTINGS = f" --cwd={folder} --no-build"
    EXP_NAME = "ccaperf-exec" + NS3_SETTINGS
    SEGMENT_HASH = str(hash(folder))
    exp_args["aiSegmentName"] = SEGMENT_HASH



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
                _sinr_eff = vector_data.sinr_eff
                msgInterface.GetPy2CppVector()[i].drop_packet = enqueue_manager(i, sinr_eff=_sinr_eff)
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