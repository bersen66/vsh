#!/usr/bin/python

import pandas as pd
import numpy as np
import argparse

def generate_dataset(flags):
    df = pd.DataFrame({
        'normal_low_scale': np.random.normal(loc=5, scale=2, size=flags.rows_number),
        'normal_mid_scale': np.random.normal(loc=0.0, scale=100_000, size=flags.rows_number),
        'normal_high_scale':np.random.normal(loc=0.0, scale=1_000_000, size=flags.rows_number),
        'normal_extra_high_scale':np.random.normal(loc=0.0, scale=1_000_000_000, size=flags.rows_number),
        'small set': np.random.choice([1.1, 2.2, 3.3, 4.4], size=flags.rows_number)
    })

    df.to_parquet(flags.filename, compression=None)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog='Distribution generator',
        description=
        'Generate parquet file where each column represents key distribution',
        exit_on_error=True,
        add_help=True,
    )

    parser.add_argument('-f',
                        '--filename',
                        type=str,
                        required=True)

    parser.add_argument('-n',
                        '--rows_number',
                        help='sets number of rows in each column',
                        nargs=1,
                        type=int,
                        required=True)

    args = parser.parse_args()

    df = generate_dataset(args)
