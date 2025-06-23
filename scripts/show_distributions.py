#!/usr/bin/python

import matplotlib.pyplot as plt
import os
import subprocess
import numpy as np

path_to_driver = os.getenv("DRIVER")
path_to_dataset = os.getenv("DATASET")
supported_algo = ["hash", "quantile", "bash"]

algo_colors = {'hash': 'orange', 'quantile': 'green', 'bash': 'blue'}

partitions_num = [5, 10, 15, 20, 25, 30]
parquet_column_max_idx = 8
dists = {
    "10k": 10000,
    "100k": 100000,
    "1m": 1000000,
}

rows_num = 0

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


def parse_driver_stdout(stdout: str):
    return [int(x) for x in stdout.strip().split()]


def create_histogram_image(hist_dict, partitions, column):
    bin_positions = np.arange(partitions)
    bar_width = 0.8 / len(hist_dict)

    # Рисуем столбцы для каждого алгоритма
    for i, (algo, values) in enumerate(hist_dict.items()):
        # Сдвигаем позиции для каждого алгоритма
        offset = i * bar_width
        plt.bar(
            bin_positions + offset,
            values,
            width=bar_width,
            color=algo_colors[algo],
            alpha=0.7,
            label=algo,
        )

    title = f'{case_[column]} (колонка={column}, партиции={partitions}, количество элементов={rows_num})'
    plt.title(title)
    plt.xlabel('Номер бина')
    plt.ylabel('Количество строк на партиции')
    plt.xticks(bin_positions + bar_width * (len(hist_dict) - 1) / 2,
               bin_positions)
    plt.legend()
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.yscale('log')
    plt.axhline(rows_num // partitions, linestyle='--', color='black')

    plt.tight_layout()
    plt.savefig(f"{title}.png", bbox_inches='tight', dpi=100)
    plt.close()


def visualize_algo_distributions(column: int):
    '''Прогон разных алгоритмов на одном распределении с увеличением количества партиций'''
    for partitions in partitions_num:

        histograms = dict()

        for algo in supported_algo:
            cmd_str = f"PRINT_EXEC_TIME=1 SHOW_DISTR=1 {path_to_driver} {algo} {path_to_dataset} {partitions} {column}"
            result = subprocess.run(cmd_str,
                                    shell=True,
                                    capture_output=True,
                                    text=True)

            histograms[f"{algo}"] = parse_driver_stdout(result.stdout)

            title = f'{case_[column]} (колонка={column}, партиции={partitions}, количество элементов={rows_num})'

            print(f"Command: {cmd_str}")
            print(f"Return code: {result.returncode}")
            print(f"Stdout: {result.stdout}")
            print(f"Stderr: {result.stderr}")
        create_histogram_image(histograms, partitions, column)

def main():
    global path_to_dataset
    global rows_num

    if path_to_driver is None:
        print("Path to driver not specified!")
        exit()

    print(f"DRIVER = {path_to_driver}")

    for suff, rows in dists.items():
        path_to_dataset = f"distributions{suff}.parquet"
        rows_num = rows
        for column_idx in range(0, parquet_column_max_idx):
            visualize_algo_distributions(column_idx)


if __name__ == '__main__':
    main()
