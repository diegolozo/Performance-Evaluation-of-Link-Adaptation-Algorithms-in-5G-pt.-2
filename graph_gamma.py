import os
import sys
import pandas as pd
import matplotlib.pyplot as plt

def plot_gamma_vs_time(file_path):

    if not os.path.exists(file_path):
        print(f"The fila: '{file_path}' does not exist.")
        return
    try:
        data = pd.read_csv(file_path, delimiter=";", skipinitialspace=True)
    except Exception as e:
        print(f"Error reading file: {e}")
        return

    required_columns = ["Time", "Gamma"]
    if not all(col in data.columns for col in required_columns):
        print(f"The file must contain the following columns: {required_columns}")
        return


    plt.figure(figsize=(10, 6))
    plt.plot(data["Time"], data["Gamma"], marker="o", linestyle="-", label="Gamma")
    plt.title("Gamma vs. Time", fontsize=16)
    plt.xlabel("Time (s)", fontsize=14)
    plt.ylabel("Gamma", fontsize=14)
    plt.grid(True, linestyle="--", alpha=0.7)
    plt.legend(fontsize=12)

    output_folder = os.path.dirname(file_path)  
    output_file = os.path.join(output_folder, "Gamma_vs_Tiempo.png") 
    plt.savefig(output_file, dpi=300)
    plt.close() 

    print(f"Graph saved in: {output_file}")

if __name__ == "__main__":
    PATH = sys.argv[1]
    plot_gamma_vs_time(PATH)
