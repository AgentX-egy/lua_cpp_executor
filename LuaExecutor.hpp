#include <filesystem>
#include <lua.hpp>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

using lua_t    = std::variant<std::nullptr_t, double, bool, std::string>;
using params_t = const std::initializer_list<lua_t>;
enum class pass_self : bool { yes = true, no = false };

class LuaExecutor {
private:
    lua_State* const L;

public:
    LuaExecutor();
    virtual ~LuaExecutor();
    LuaExecutor(std::filesystem::path path);
    void operator()(std::string_view script) const;

    lua_t getGlobal(std::string_view name) const;
    void setGlobal(std::string_view name, const lua_t& value);

    lua_t getTable(std::string_view table, std::string_view key) const;
    void setTable(std::string_view table, std::string_view key,
                  const lua_t& value);

    lua_t getTable(std::string_view array, uint32_t index) const;
    void setTable(std::string_view array, uint32_t index, lua_t value);

    lua_t call(std::string_view function, params_t params) const;
    std::vector<lua_t> vcall(std::string_view function, params_t params) const;

    lua_t tcall(std::string_view table, std::string_view member_function,
                params_t params, const pass_self _pass_self) const;
    std::vector<lua_t> vtcall(std::string_view table,
                              std::string_view member_function, params_t params,
                              const pass_self _pass_self) const;

private:
    lua_t popValue() const;
    std::vector<lua_t> popValues(int count) const;
    std::string popString() const;

    lua_t getValue(int index = -1) const;
    void pushValue(const lua_t& value) const;
    void pcall(int nargs = 0, int nresults = 0) const;
};

extern std::string getLuaValueString(lua_t& x);
extern std::ostream& operator<<(std::ostream& os, lua_t& l);