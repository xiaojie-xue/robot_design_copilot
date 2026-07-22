#include "robot_engine/design/calculation.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

namespace {

using Json = nlohmann::json;

[[nodiscard]] Json read_input(const int argc, char **argv) {
  if (argc > 2) {
    throw std::runtime_error("usage: robot-engine-design [INPUT.json]");
  }
  if (argc == 2) {
    std::ifstream stream(argv[1]);
    if (!stream) {
      throw std::runtime_error(std::string("cannot open input: ") + argv[1]);
    }
    return Json::parse(stream);
  }
  return Json::parse(std::cin);
}

} // namespace

int main(const int argc, char **argv) {
  try {
    const auto result = robot_engine::design::calculate(read_input(argc, argv));
    std::cout << result.dump(2) << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "Design input could not be read: " << error.what() << '\n';
    return 2;
  }
}
