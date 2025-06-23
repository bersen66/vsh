#!/usr/bin/python

import pandas as pd
import numpy as np
import argparse

def gen_doubles(max):
    result = list()
    for i in range(0, max):
        result.append(float(i))
    return result

def gen_doubles_desc(max):
    result = list()
    for i in range(max, 0, -1):
        result.append(float(i))
    return result



def generate_dataset(output_file, number_of_rows):

    bimodal_data = np.concatenate([
        np.round(np.random.normal(loc=200, scale=(number_of_rows // 2), size=number_of_rows // 2)).astype(int).astype(float),
        np.round(np.random.normal(loc=800, scale=(number_of_rows // 2), size=number_of_rows // 2)).astype(int).astype(float)
    ])
    np.random.shuffle(bimodal_data) 

    df = pd.DataFrame({
        'uniform_small_set':
        np.random.choice([1.0, 2.0, 3.0, 4.0], size=number_of_rows),
        'uniform_mid_set':
        np.random.choice(gen_doubles(100), size=number_of_rows),
        'uniform_large_set':
        np.random.choice(gen_doubles(10000), size=number_of_rows),
        'uniform_more_uniq':
        np.random.choice(gen_doubles(2*number_of_rows), size=number_of_rows),


        'arithm_grow':
        gen_doubles(number_of_rows),

        'arithm_desc':
        gen_doubles_desc(number_of_rows),

        'normal':
        np.round(np.random.normal(loc=0, scale=(number_of_rows // 2), size=number_of_rows)).astype(int).astype(float),


        'bimodal': bimodal_data,
    })

    df.to_parquet(output_file, compression=None)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog='Distribution generator',
        description=
        'Generate parquet file where each column represents key distribution',
        exit_on_error=True,
        add_help=True,
    )

    parser.add_argument('-f', '--filename', type=str, required=True)

    parser.add_argument('-n',
                        '--rows_number',
                        help='sets number of rows in each column',
                        nargs=1,
                        type=int,
                        required=True)

    args = parser.parse_args()

    df = generate_dataset(args.filename, args.rows_number[0])

