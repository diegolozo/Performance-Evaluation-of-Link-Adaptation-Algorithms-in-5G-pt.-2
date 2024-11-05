import matplotlib.pyplot as plt
import functools
import time

__DEBUG_ALL = False

# Set text colors
CLEAR = '\033[0m'
RED = '\033[0;31m'
GREEN = '\033[0;32m'
YELLOW = '\033[0;33m'
BLUE = '\033[0;34m'
MAGENTA = '\033[0;35m'
CYAN = '\033[0;36m'

# Set Background colors
BG_RED = '\033[0;41m'
BG_GREEN = '\033[0;42m'
BG_YELLOW = '\033[0;43m'
BG_BLUE = '\033[0;44m'
BG_MAGENTA = '\033[0;45m'
BG_CYAN = '\033[0;46m'


# Error for the number of arguments
class ArgumentError(BaseException):
    pass

# ----------------------------------------------------------
# Decorator for time
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# It calculates the time it takes to execute the function
# and prints its name (that must be passed as an argument)
# and the time it took to execute.
#
# e.g.
# @info_n_time_decorator("Custom function")
# def myfunc():
#   if works:
#       return True
#   else:
#       return False
#
# ----------------------------------------------------------
def info_n_time_decorator(name, debug=False):

    def actual_decorator(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):

            print(CYAN + name + CLEAR, end="...", flush=True)
            tic = time.time()

            func_ret = None
            try:
                func_ret = func(*args, **kwargs)
            except KeyboardInterrupt:
                print(f"{RED}User canceled.{CLEAR}", end="")
            except Exception as e:
                if debug or __DEBUG_ALL:
                    print(f"Exception thrown: {e}")
            finally:
                plt.close()

            if func_ret is not None:
                toc = time.time()
                print(f"\tProcessed in: %.2f" % (toc-tic))
            else:
                print(RED + "\tError while processing. Skipped." + CLEAR)

            return func_ret

        return wrapper
    return actual_decorator