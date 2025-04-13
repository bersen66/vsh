#include <arrow/io/api.h>
#include <arrow/status.h>
#include <cstdlib>
#include <parquet/arrow/reader.h>
#include <iostream>
#include <vsh/vsh.hpp>

arrow::Status RunMain() {
    
    return arrow::Status::OK();
}

int main(void) {
    arrow::Status res = RunMain();
    if (!res.ok()) {
        std::cerr << res << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


