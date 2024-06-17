#ifndef DOTENV_H__
#define DOTENV_H__
#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace dotenv {
    void parse(std::string_view input, std::unordered_map<std::string, std::string>& env, bool overwrite);

    inline void parse(const std::string_view input, std::unordered_map<std::string, std::string>& env) {
        ::dotenv::parse(input, env, false);
    }
}

#endif
