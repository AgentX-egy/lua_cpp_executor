#include <iostream>
#include "LuaExecutor.hpp"

void dumpPosition(const LuaExecutor& lua) {
    lua_t x = lua.getTable("position", "x");
    lua_t y = lua.getTable("position", "y");
    std::cout << "x=" << std::get<double>(x) << ", y=" << std::get<double>(y)
              << std::endl;
}

void dumpSeq(LuaExecutor& lua) {
    lua_t v1 = lua.getTable("seq", 1);
    lua_t v2 = lua.getTable("seq", 2);
    lua_t v3 = lua.getTable("seq", 3);
    std::cout << "seq={" << std::get<double>(v1) << ", " << std::get<double>(v2)
              << ", " << std::get<double>(v3) << "}" << std::endl;
}

int main(int argc, char** argv) {
    if (argc != 2) return 1;

    LuaExecutor lua(argv[1]);
    lua.tcall("Destinations", "new", {"dst"}, pass_self::no);
    lua.tcall("dst", "wish", {"London", "Paris", "Amsterdam"}, pass_self::yes);
    lua.tcall("dst", "went", {"Paris"}, pass_self::yes);
    auto visited   = lua.vtcall("dst", "list_visited", {}, pass_self::yes);
    auto unvisited = lua.vtcall("dst", "list_unvisited", {}, pass_self::yes);

    std::cout << "Visited:" << std::endl;
    for (auto place : visited) {
        std::cout << place << std::endl;
    }

    std::cout << "Unvisited:" << std::endl;
    for (auto place : unvisited) {
        std::cout << place << std::endl;
    }
}
