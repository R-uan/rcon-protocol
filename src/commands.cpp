#include "commands.hpp"
#include <iostream>
#include <string_view>

using namespace std;

string_view echo(string_view arg) {
  std::cout << "[SERVER] echo " << arg << std::endl;
  return arg;
}

string_view run_command(string_view input) {
  size_t pos = input.find(' ');
  string_view command(input.data(), pos);
  string_view argument(pos == string_view::npos ? "" : input.data() + pos + 1);

  if (command == "echo") {
    return echo(argument);
  } else {
    return "[SERVER] command does not exist";
  }
}
