#include "LuaExecutor.hpp"
#include <cassert>
#include <iostream>

LuaExecutor::LuaExecutor() : L(luaL_newstate()) { luaL_openlibs(L); }

LuaExecutor::LuaExecutor(std::filesystem::path path) : LuaExecutor() {
    if (luaL_loadfile(L, path.string().c_str())) {
        std::cerr << "Failed to prepare file: " << popString() << std::endl;
        return;
    }
    pcall();
}

void LuaExecutor::operator()(std::string_view script) const {
    if (luaL_loadstring(L, script.data())) {
        std::cerr << "Failed to prepare script: " << popString() << std::endl;
        return;
    }
    pcall();
}

LuaExecutor::~LuaExecutor() { lua_close(L); }

lua_t LuaExecutor::getGlobal(std::string_view name) const {
    lua_getglobal(L, name.data());
    return popValue();
}

void LuaExecutor::setGlobal(std::string_view name, const lua_t& value) {
    pushValue(value);
    lua_setglobal(L, name.data());
}

lua_t LuaExecutor::getTable(std::string_view table,
                            std::string_view key) const {
    const int type = lua_getglobal(L, table.data());
    assert(type == LUA_TTABLE);
    // lua_pushstring(L, key.data());
    // lua_gettable(L, -2);
    lua_getfield(L, -1, key.data());
    lua_t result = popValue();
    lua_pop(L, 1);
    return result;
}

void LuaExecutor::setTable(std::string_view table, std::string_view key,
                           const lua_t& value) {
    const int type = lua_getglobal(L, table.data());
    assert(type == LUA_TTABLE);
    lua_pushstring(L, key.data());
    pushValue(value);
    lua_settable(L, -3);
    lua_pop(L, 1);
}

lua_t LuaExecutor::getTable(std::string_view array, uint32_t index) const {
    const int type = lua_getglobal(L, array.data());
    assert(type == LUA_TTABLE);
    lua_geti(L, -1, index);
    lua_t result = popValue();
    lua_pop(L, 1);
    return result;
}

void LuaExecutor::setTable(std::string_view array, uint32_t index,
                           lua_t value) {
    const int type = lua_getglobal(L, array.data());
    assert(type == LUA_TTABLE);

    pushValue(value);
    lua_seti(L, -2, index);

    lua_pop(L, 1);
}

lua_t LuaExecutor::call(std::string_view function, params_t params) const {
    const int type = lua_getglobal(L, function.data());
    assert(type == LUA_TFUNCTION);

    for (const lua_t& param : params) {
        pushValue(param);
    }

    pcall(params.size(), 1);

    return popValue();
}

std::vector<lua_t> LuaExecutor::vcall(std::string_view function,
                                      params_t params) const {
    const int stackSz = lua_gettop(L);

    const int type = lua_getglobal(L, function.data());
    assert(type == LUA_TFUNCTION);

    for (const lua_t& param : params) {
        pushValue(param);
    }

    pcall(params.size(), LUA_MULTRET);
    const int nresults = lua_gettop(L) - stackSz;

    return ((nresults) ? (popValues(nresults)) : (std::vector<lua_t> {}));
}

lua_t LuaExecutor::tcall(std::string_view table,
                         std::string_view member_function, params_t params,
                         const pass_self _pass_self) const {
    const int typeT = lua_getglobal(L, table.data());
    assert(typeT == LUA_TTABLE);
    const int typeF = lua_getfield(L, -1, member_function.data());
    assert(typeF == LUA_TFUNCTION);

    if (_pass_self == pass_self::yes) {
        lua_getglobal(L, table.data());
    }
    for (const lua_t& param : params) {
        pushValue(param);
    }
    const int nargs = params.size() + static_cast<int>(_pass_self);
    pcall(nargs, 1);

    lua_t result = popValue();
    lua_pop(L, 1);

    return result;
}

std::vector<lua_t> LuaExecutor::vtcall(std::string_view table,
                                       std::string_view member_function,
                                       params_t params,
                                       const pass_self _pass_self) const {
    const int type = lua_getglobal(L, table.data());
    assert(type == LUA_TTABLE);

    const int stackSz = lua_gettop(L);

    const int typeF = lua_getfield(L, -1, member_function.data());
    assert(typeF == LUA_TFUNCTION);

    if (_pass_self == pass_self::yes) {
        lua_getglobal(L, table.data());
    }
    for (const lua_t& param : params) {
        pushValue(param);
    }

    const int nargs = params.size() + static_cast<int>(_pass_self);
    pcall(nargs, LUA_MULTRET);

    const int nresults = lua_gettop(L) - stackSz;
    auto result =
        ((nresults) ? (popValues(nresults)) : (std::vector<lua_t> {}));
    lua_pop(L, 1);
    return result;
}

lua_t LuaExecutor::popValue() const {
    lua_t value {getValue()};
    lua_pop(L, 1);
    return value;
}

std::vector<lua_t> LuaExecutor::popValues(int n) const {
    std::vector<lua_t> result;
    result.reserve(n);
    for (int i = n; i > 0; --i) {
        result.emplace_back(getValue(-i));
    }
    lua_pop(L, n);
    return result;
}

std::string LuaExecutor::popString() const {
    std::string value {std::get<std::string>(popValue())};
    return value;
}

lua_t LuaExecutor::getValue(int index) const {
    switch (lua_type(L, index)) {
        case LUA_TNIL: return {static_cast<std::nullptr_t>(nullptr)};
        case LUA_TBOOLEAN: return {static_cast<bool>(lua_toboolean(L, index))};
        case LUA_TNUMBER: return {static_cast<double>(lua_tonumber(L, index))};
        case LUA_TSTRING:
            return {static_cast<std::string>(lua_tostring(L, index))};
        default: return {static_cast<std::nullptr_t>(nullptr)};
    }
}

void LuaExecutor::pushValue(const lua_t& value) const {
    static struct LuaPusher
    {
        lua_State* const L;
        LuaPusher(lua_State* L) : L(L) {};
        void operator()(const std::nullptr_t p) const { lua_pushnil(L); };
        void operator()(const double p) const { lua_pushnumber(L, p); };  //
        void operator()(const bool p) const {
            lua_pushboolean(L, static_cast<int>(p));
        };
        void operator()(const std::string& p) const {
            lua_pushstring(L, p.c_str());
        };
    } pusher {L};
    std::visit(pusher, value);
}

void LuaExecutor::pcall(int nargs, int nresults) const {
    if (lua_pcall(L, nargs, nresults, 0)) {
        std::cerr << "Failed to execute Lua code: " << popString() << std::endl;
    }
}

std::string getLuaValueString(lua_t& x) {
    static struct ToString
    {
        std::string operator()(std::nullptr_t p) {
            using std::string_literals::operator""s;
            return "nullptr"s;
        };
        std::string operator()(double p) { return std::to_string(p); };

        std::string operator()(bool p) {
            using std::string_literals::operator""s;
            return p ? "true"s : "false"s;
        };
        std::string operator()(std::string& p) { return std::move(p); };
    } to_string;
    return std::visit(to_string, x);
}

std::ostream& operator<<(std::ostream& os, lua_t& l) {
    os << getLuaValueString(l);
    return os;
}