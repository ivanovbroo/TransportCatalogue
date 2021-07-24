#include <transport_catalogue.pb.h>

#include <fstream>

#include "request_handler.h"

namespace transport_serialization {

	void Serialize(std::ofstream& out, const request_handler::RequestHandler& request_handler);

	void Deserialize(request_handler::RequestHandler& request_handler, std::ifstream& in);

} // namespace transport_serialization
