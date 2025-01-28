#ifndef CRINGE_ENVIRONMENT_H
#define CRINGE_ENVIRONMENT_H

#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

auto get_env(const std::string_view &given_key) -> std::string;

#endif
