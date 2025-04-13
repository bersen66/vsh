add_executable(driver main.cpp)
target_link_libraries(driver vsh Arrow::arrow_static Parquet::parquet_static)
