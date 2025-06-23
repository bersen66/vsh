#!/usr/bin/python
#!/usr/bin/python

import matplotlib.pyplot as plt
import os
import subprocess
import numpy as np

# Global variables, inspired by show_distributions.py
path_to_driver = os.getenv("DRIVER")
supported_algo = ["hash", "quantile", "bash"]
algo_colors = {'hash': 'orange', 'quantile': 'green', 'bash': 'blue'}
partitions_num = [5, 10, 15, 20, 25, 30]
parquet_column_max_idx = 8
dists = {
    "10k": 10000,
    "100k": 100000,
    "1m": 1000000,
}
case_ = [
    'Случайное целое (равновероятное) в диапазоне [0, 4]',
    'Случайное целое (равновероятное) в диапазоне [0, 100]',
    'Случайное целое (равновероятное) в диапазоне [0, 10000]',
    'Cлучайное целое (равновероятное) в диапазоне [0, 2 * кол-во строк]',
    'Возрастающая арифметическая прогрессия [0, кол-во строк]',
    'Убывающая арифметическая прогрессия [кол-во строк, 0]',
    'Случайное целое нормальное распределение',
    'Перемешанное объединение 2х нормальных распределений',
]

def parse_exec_time_stdout(stdout: str) -> int:
    """Parses the execution time from the C++ driver's stdout."""
    try:
        # The C++ driver prints "X ms\n"
        # Split by space and take the first part (the number)
        return int(stdout.strip().split(' ')[0])
    except (ValueError, IndexError) as e:
        print(f"Error parsing execution time: {stdout}. Error: {e}")
        return -1 # Indicate an error

def create_performance_plot(performance_data, column, rows_num, dataset_suffix):
    """Generates and saves a performance plot."""
    plt.figure(figsize=(10, 6))

    for algo, times in performance_data.items():
        plt.plot(partitions_num, times, marker='o', linestyle='-', color=algo_colors[algo], label=algo)

    title = f'Производительность алгоритмов на распределении: {case_[column]} ({rows_num} элементов, датасет {dataset_suffix})'
    plt.title(title)
    plt.xlabel('Количество партиций')
    plt.ylabel('Время выполнения (мс)')
    plt.xticks(partitions_num)
    plt.legend()
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.savefig(f"performance_{dataset_suffix}_column_{column}.png", bbox_inches='tight', dpi=100)
    plt.close()

def main():
    global path_to_driver

    if path_to_driver is None:
        print("Path to driver not specified! Please set the DRIVER environment variable.")
        exit(1)

    print(f"DRIVER = {path_to_driver}")

    for suff, rows in dists.items():
        path_to_dataset = f"distributions{suff}.parquet"
        
        # Ensure the dataset exists before attempting to process it
        if not os.path.exists(path_to_dataset):
            print(f"Dataset {path_to_dataset} not found. Skipping.")
            continue

        for column_idx in range(0, parquet_column_max_idx):
            performance_data = dict()

            for algo in supported_algo:
                exec_times = []
                for partitions in partitions_num:
                    cmd_str = f"PRINT_EXEC_TIME=1 SHOW_DISTR=0 {path_to_driver} {algo} {path_to_dataset} {partitions} {column_idx}"
                    result = subprocess.run(cmd_str, shell=True, capture_output=True, text=True)

                    if result.returncode != 0:
                        print(f"Error running command: {cmd_str}")
                        print(f"Stderr: {result.stderr}")
                        exec_times.append(0) # Append 0 or handle error differently if needed
                    else:
                        time_ms = parse_exec_time_stdout(result.stdout)
                        exec_times.append(time_ms)
                    
                    print(f"Command: {cmd_str}")
                    print(f"Return code: {result.returncode}")
                    print(f"Stdout: {result.stdout.strip()}")
                    print(f"Stderr: {result.stderr.strip()}")
                performance_data[algo] = exec_times
            
            create_performance_plot(performance_data, column_idx, rows, suff)
            print(f"Generated plot for dataset {suff}, column {column_idx}")

if __name__ == '__main__':
    main()
