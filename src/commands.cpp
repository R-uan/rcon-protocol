#include "commands.hpp"
#include <string_view>

using namespace std;

string_view echo(string_view arg) { return arg; }

string_view run_command(string_view input) {
  size_t pos = input.find(' ');
  string_view command(input.data(), pos);
  string_view argument(pos == string_view::npos ? "" : input.data() + pos + 1);

  if (command == "echo") {
    return echo(argument);
  } else {
    return "command does not exist";
  }
}
