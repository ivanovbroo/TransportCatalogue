#include "request_handler.h"

int main(int argc, const char** argv) {
    using namespace request_handler;
    request_handler::ProgrammType type = request_handler::ParseProgrammType(argc, argv);

    if (type == request_handler::ProgrammType::UNKNOWN) {
        return 1;
    }

    RequestHandlerProcess rhp(std::cin, std::cout);
    if (type == request_handler::ProgrammType::MAKE_BASE) {
        rhp.ExecuteMakeBaseRequests();
    }
    else if (type == request_handler::ProgrammType::PROCESS_REQUESTS) {
        rhp.ExecuteProcessRequests();
    }
    else {
        return 2;
    }

    return 0;
}
