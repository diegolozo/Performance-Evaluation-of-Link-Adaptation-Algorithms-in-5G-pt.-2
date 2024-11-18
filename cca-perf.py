import cca_perf_py as py_binding
from ns3ai_utils import Experiment
import sys
import os
import traceback
import argparse
import signal
from time import localtime, strftime, time  # For the time being
import subprocess
from lib.amc import AmcManager, Exp_Mean_Bler, Q_Learn_BLER

AMC_OPTS = {
      "exp_mean_bler_target": 5,
      "q_learn_bler_target": 6
}


AMC_FUNC = {
      "exp_mean_bler_target": Exp_Mean_Bler,
      "q_learn_bler_target": Q_Learn_BLER
}

def handler_timeout(signum, frame):
    raise TimeoutError("Timeout reached")

def argument_parser():
    parser = argparse.ArgumentParser(
        prog=os.path.basename(__file__),
        description="Does stuff in contrib/nr/model/nr-amc.cc",
        epilog="Thanks to ns3-ai team for their work. Made by AG-DT."
    )

    parser.add_argument("--amc", default="exp_mean_bler_target", help="")

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

    amc_number=args.amc.lower()
    experiment_args_dict["amcAlgo"] = AMC_OPTS[amc_number]
    experiment_args_dict.pop("amc", None)

    return args, experiment_args_dict

def main(amc: str, folder: str, exp_args: dict):
    VECTOR_SIZE = 32  # This is a complication | We would need to know the number of flows beforehand | Can we set it on the fly?
    NS3_PATH = os.getcwd().split("scratch")[0]  # Let's set it dynamically
    NS3_SETTINGS = f" --cwd={folder} --no-build"
    EXP_NAME = "ccaperf-exec" + NS3_SETTINGS
    SEGMENT_HASH = str(hash(folder))
    exp_args["aiSegmentName"] = SEGMENT_HASH

    amc_final = amc.lower()

    print(f"El valor de amc recibido es: {amc_final}")
    print(f"El valor de amc a usar es: {AMC_OPTS[amc_final]}")

    amc_manager = AmcManager(AMC_FUNC[amc_final], folder)

    if not os.path.isdir(folder):
        os.mkdir(folder)
    
    exp = Experiment(EXP_NAME, NS3_PATH, py_binding,
                     handleFinish=True, useVector=True, vectorSize=VECTOR_SIZE, segName=SEGMENT_HASH)
    msgInterface = exp.run(show_output=True, setting=exp_args)
    print(f"NS3AI Memory space name: {SEGMENT_HASH}", flush=True)

    # Establecer el timeout (en segundos)
    timeout_duration = 3600  # 300 segundos = 5 minutos (cambia según lo necesario)
    signal.signal(signal.SIGALRM, handler_timeout)
    signal.alarm(timeout_duration)  # Establecer el tiempo límite de la alarma

    start_time = time()  # Registrar el inicio del proceso

    try:
        while True:
            # Verificar si ha pasado demasiado tiempo
            if time() - start_time > timeout_duration:
                raise TimeoutError("Timeout reached during execution")

            # Recibir del lado C++
            msgInterface.PyRecvBegin()
            if msgInterface.PyGetFinished():
                break

            # Enviar al lado C++
            msgInterface.PySendBegin()
            vector_data = msgInterface.GetCpp2PyVector()[0]
            _sinr_eff = vector_data.sinr_eff
            _simulation_time = vector_data.simulation_time
            bler = amc_manager(0, sinr_eff=_sinr_eff, simulation_time=_simulation_time)
            msgInterface.GetPy2CppVector()[0].bler_target = bler

            msgInterface.PyRecvEnd()
            msgInterface.PySendEnd()

    except TimeoutError as e:
        print(f"TimeoutError: {e}")
        # Terminar el experimento y liberar recursos
        print("Timeout reached, exiting...")
        if exp.proc:
            exp.proc.terminate()  # Terminar el proceso asociado con la simulación
            print("Simulation process terminated.")
        # Verificar si simCmd es un proceso y terminarlo
        if isinstance(exp.simCmd, subprocess.Popen):
            exp.simCmd.terminate()  # Terminar el proceso asociado con la simulación
            print("SimCmd process terminated.")
        sys.exit(1)

    except Exception as e:
        exc_type, exc_value, exc_traceback = sys.exc_info()
        print("Exception occurred: {}".format(e))
        print("Traceback:")
        traceback.print_tb(exc_traceback)
        sys.exit(1)

    else:
        pass

    finally:
        print("Finally exiting...")
        del exp
        signal.alarm(0)

if __name__ == "__main__":
    args, exp_args = argument_parser()
    main(args.amc, args.cwd, exp_args)