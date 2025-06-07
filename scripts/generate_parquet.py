#!/usr/bin/python

import pandas as pd
import numpy as np

num_rows = 100

df = pd.DataFrame({
    'normal_low_scale':  np.random.normal(loc=5, scale=2, size=num_rows),
    'normal_mid_scale':  np.random.normal(loc=0.0, scale=100_000, size=num_rows),
    'normal_high_scale': np.random.normal(loc=0.0, scale=1_000_000, size=num_rows),
})

if __name__ == "__main__":
    print(f"{df['normal_low_scale']}")
