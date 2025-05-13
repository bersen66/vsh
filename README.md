# Equi-depth гистограммы:

В данной работе реализовано несколько алгоритмов построения  `equi-depth` гистограмм.

* С помощью квантилей (файлы: [quantile_hist.hpp](./lib/vsh/include/vsh/quantile_hist.hpp), [quantile_hist.cpp](./lib/vsh/src/quantile_hist.cpp))
* Алгоритм `BASH-BL` из статьи [Fast and Accurate Computation of
Equi-Depth Histograms over Data Streams](./doc/Fast_and_Accurate_Computation_Of_Equi-Depth_Histograms_Over_Data_Streams.pdf) (файлы: [bar_splitting_hist.hpp](./lib/vsh/include/vsh/bar_splitting_hist.hpp), [bar_splitting_hist.cpp](./lib/vsh/src/bar_splitting_hist.cpp))

## Сборка прооекта

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j $(nproc)
```

## Структура проекта:

* [lib/driver/](./lib/driver/) -- консольная утилита для построения гистограмм по датасетам
* [lib/vsh/](./lib/vsh/) -- библиотека алгоритмов для построения гистограмм