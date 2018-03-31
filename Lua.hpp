#pragma once

#include <lua.hpp>
#include <assert.h>
#include <stdexcept>
#include <type_traits>
#include <string>
#include <memory>
#include <vector>
#include <functional>

/*
	OPTIONS:

	IMPLEMENTATION OPTIONS (required):

	- LUA_WRAPPER_IMPLEMENTATION_LUAJIT
	Ensures compatiblity with LuaJIT.

	- LUA_WRAPPER_IMPLEMENTATION_LUA53
	Ensures compatibility with Lua 5.3.

	FUNCTION RETURN VALUE OPTIONS:

	- LUA_WRAPPER_SINGLE_RETURN
	A Lua function may not return more than a
	single variable.

	- LUA_WRAPPER_TABLE_RETURN
	A Lua function may return as many variables
	as it wants. 2 or more variables will be
	returned as a table.

	- LUA_WRAPPER_VECTOR_RETURN
	A Lua function may return as many variables
	as it wants. All return values will be
	returned as an std::vector.

	MISC OPTIONS (optional):

	- LUA_WRAPPER_STATELESS_STRINGS
	Allows stateless locals to hold string
	values. This option may incur performance
	penalties.

	- LUA_WRAPPER_SMART_FUNCTIONS
	Allows for std::functions to be passed
	as functions and for Lua functions to
	be obtained as an std::function. This
	option may incur performance penalites.
*/

#define LUA_WRAPPER_IMPLEMENTATION_LUAJIT
#define LUA_WRAPPER_STATELESS_STRINGS
#define LUA_WRAPPER_TABLE_RETURN
#define LUA_WRAPPER_SMART_FUNCTIONS

#if !defined(LUA_WRAPPER_SINGLE_RETURN) && !defined(LUA_WRAPPER_TABLE_RETURN) && !defined(LUA_WRAPPER_VECTOR_RETURN)
#pragma error("No function return method selected")
#endif

#if !defined(LUA_WRAPPER_IMPLEMENTATION_LUAJIT) || !defined(LUA_WRAPPER_IMPLEMENTATION_LUA53)
#pragma error("No implementation selected")
#endif

namespace lua
{
	using Number = lua_Number;
	using Integer = lua_Integer;

	class state;
	class table_index;
	class local;

	class state
	{
		friend class local;
		friend class table_index;

	public:
		state();
		state(state &&s);
		~state();

		operator lua_State*();

		state(const state&) = delete;
		state& operator=(const state&) = delete;

		void open_libs();

		void do_string(const char *n);

		local create_table(int narr = 0, int nrec = 0);
		local create_string(const char *s);

		local get_global(const char *n);
		void set_global(local lcl, const char *n);

	private:
		lua_State * L;
	};

	class table_index
	{
		friend class local;

	public:
		~table_index();

		operator local();
		//table_index& operator=(local &rhs);
		table_index& operator=(local rhs);

	private:
		state * s;
		int tblRef, idxRef;

		table_index(state &s, int tblRef, local &idx);
		table_index();

		table_index(const table_index &ti);

		table_index& operator=(const table_index &rhs);

		void check_valid();

		local get_value();
	};

	class local
	{
		friend class state;
		friend class table_index;

	public:
		local();
		local(state &s);
		local(const local &lcl);
		local(local &&lcl);
		local(bool b);
		local(lua_Number n);
		local(const char *str);
		local(lua_CFunction f);
		local(void *p);
		local(state &s, bool b);
		local(state &s, lua_Number n);
		local(state &s, const char *str);
		local(state &s, lua_CFunction f);
		local(state &s, void *p);
		~local();
		local& operator=(const local &rhs);

		bool is_nil();
		bool is_boolean();
		bool is_number();
		bool is_integer();
		bool is_string();
		bool is_function();
		bool is_cfunction();
		bool is_userdata();
		bool is_lightuserdata();
		bool is_thread();
		bool is_table();

		void set_as_nil();
		void set_as_boolean(bool b);
		void set_as_number(lua_Number n);
		void set_as_integer(lua_Integer i);
		void set_as_string(const char *s);
		void set_as_cfunction(lua_CFunction f);
#ifdef LUA_WRAPPER_SMART_FUNCTIONS
		void set_as_function(std::function<void()> f);
#endif
		void set_as_lightuserdata(void *p);

		bool to_boolean();
		lua_Number to_number();
		lua_Integer to_integer();
		const char* to_string();
		lua_CFunction to_cfunction();
		void* to_userdata();

		void table_set(local key, local value);
		void table_set(lua_Integer key, local value);
		local table_get(local key);
		local table_get(lua_Integer key);

		size_t length();

#ifdef LUA_WRAPPER_INDEXABLE_LOCALS
		table_index& operator[](local key);
		table_index& operator[](lua_Integer key);
		//table_index& operator[](const std::string &key);
#endif

		local& operator=(bool rhs);
		local& operator=(lua_Number rhs);
		local& operator=(lua_CFunction rhs);
		local& operator=(void *p);
		local& operator=(const char *s);

		
#if defined(LUA_WRAPPER_SINGLE_RETURN) || defined(LUA_WRAPPER_TABLE_RETURN)
		template<typename T, typename... Args>
		local operator()(T arg, Args... args);
		template<typename T>
		local operator()(T arg);
		local operator()();
#elif define(LUA_WRAPPER_VECTOR_RETURN)
		template<typename T, typename... Args>
		std::vector<local> operator()(T arg, Args... args);
		template<typename T>
		std::vector<local> operator()(T arg);
		std::vector<local> operator()();
#endif

	private:
		enum class type
		{
			nil,
			boolean,
			number,
			integer,
			string,
#ifdef LUA_WRAPPER_STATELESS_STRINGS
			stateless_string,
#endif
			function,
			cfunction,
/*
#ifdef LUA_WRAPPER_SMART_FUNCTIONS
			smart_function,
#endif
*/
			userdata,
			lightuserdata,
			thread,
			table,
		};

		void copy_value(const local &lcl);

		bool is_ref_type();
		int duplicate_ref();
		void release();

		void load_ref_value_no_type(int idx = -1);
		void load_ref_value(int idx = -1);
		void load_value(int idx = -1);

		void push_ref_value();
		void push_value(lua_State *L = nullptr);

		static lua_Integer numberToInteger(lua_Number number);
		void integerize();

#ifdef LUA_WRAPPER_STATELESS_STRINGS
		static char* create_string(const char *s);
		static char* copy_string(char *s);
		static void release_string(char *s);
		static uint32_t* get_string_refc(char *s);
#endif

		void check_state();
		void check_state_consistancy(lua_State *L);
		void check_is_function();
		void check_is_table();

		void prep_call();
#if defined(LUA_WRAPPER_SINGLE_RETURN) || defined(LUA_WRAPPER_TABLE_RETURN)
		local do_call();
#elif define(LUA_WRAPPER_VECTOR_RETURN)
		std::vector<local> do_call();
#endif

		template<typename T, typename... Args>
		void push_call_args(T arg, Args... args);
		template<typename T>
		void push_call_args(T arg);

		state *s;
		lua_State *L;

		type t;

		union
		{
			bool boolean;
			lua_Number number;
			lua_Integer integer;
			lua_CFunction cfunction;
			void *lightuserdata;
			int ref;

#ifdef LUA_WRAPPER_STATELESS_STRINGS
			/*
				NOTE: NOT a pointer to the base address of the memory allocation.
				The first 4 bytes of the memory block is reservered for the reference
				count. Must be used with the appropriate functions (create_string,
				copy_string, and release_string).
			*/
			char *string;
#endif
		} value;

		table_index tindex;

		uint32_t cargs;
	};

	//nil value
	const local nil;

	/* state */

	state::state()
	{
		L = luaL_newstate();
	}

	state::state(state &&s) : L(s.L)
	{
		s.L = nullptr;
	}

	state::~state()
	{
		if (L != nullptr)
			lua_close(L);
	}

	state::operator lua_State*()
	{
		return L;
	}

	void state::open_libs()
	{
		luaL_openlibs(L);
	}

	void state::do_string(const char *n)
	{
		if (luaL_dostring(L, n))
		{
			std::runtime_error err(lua_tostring(L, 1));
			lua_pop(L, 1);
			throw err;
		}
	}

	local state::create_table(int narr, int nrec)
	{
		lua_createtable(L, narr, nrec);

		local lcl(*this);
		lcl.t = local::type::table;
		lcl.load_ref_value_no_type();

		return lcl;
	}

	local state::create_string(const char *s)
	{
		local lcl(*this);
		lcl.set_as_string(s);
		return lcl;
	}

	local state::get_global(const char *n)
	{
		lua_getglobal(L, n);
		local lcl(*this);
		lcl.load_value();
		lua_pop(L, 1);
		return lcl;
	}

	void state::set_global(local lcl, const char *n)
	{
		lua_setglobal(L, n);
		lcl.push_value();
	}

	/* table_index */

	table_index::operator local()
	{
		return get_value();
	}

	/*
	table_index& table_index::operator=(local &rhs)
	{
		check_valid();
		rhs.check_state_consistancy(s->L);
		lua_rawgeti(s->L, LUA_REGISTRYINDEX, tblRef);
		lua_rawgeti(s->L, LUA_REGISTRYINDEX, idxRef);
		rhs.push_value(s->L);
		lua_settable(s->L, -3);
		return *this;
	}
	*/

	table_index& table_index::operator=(local rhs)
	{
		check_valid();
		rhs.check_state_consistancy(s->L);
		lua_rawgeti(s->L, LUA_REGISTRYINDEX, tblRef);
		lua_rawgeti(s->L, LUA_REGISTRYINDEX, idxRef);
		rhs.push_value(s->L);
		lua_settable(s->L, -3);
		lua_pop(s->L, 1);
		return *this;
	}

	table_index::table_index(state &s, int tblRef, local &idx)
	{
		idx.check_state_consistancy(s.L);
		this->s = &s;
		this->tblRef = tblRef;
		idx.push_value(s.L);
		idxRef = luaL_ref(s.L, LUA_REGISTRYINDEX);
	}

	table_index::table_index() : s(nullptr)
	{
		tblRef = LUA_REFNIL;
		idxRef = LUA_REFNIL;
	}

	table_index::table_index(const table_index &ti)
	{
		s = ti.s;
		tblRef = ti.tblRef;
		lua_rawgeti(s->L, LUA_REGISTRYINDEX, ti.idxRef);
		idxRef = luaL_ref(s->L, LUA_REGISTRYINDEX);
	}

	table_index::~table_index()
	{
		if (idxRef != LUA_REFNIL)
			luaL_unref(s->L, LUA_REGISTRYINDEX, idxRef);
	}

	table_index& table_index::operator=(const table_index &rhs)
	{
		if (idxRef != LUA_REFNIL)
			luaL_unref(s->L, LUA_REGISTRYINDEX, idxRef);
		s = rhs.s;
		if (s != nullptr)
		{
			lua_rawgeti(s->L, LUA_REGISTRYINDEX, rhs.idxRef);
			idxRef = luaL_ref(s->L, LUA_REGISTRYINDEX);
			tblRef = rhs.tblRef;
		}
		else
		{
			tblRef = LUA_REFNIL;
			idxRef = LUA_REFNIL;
		}
		return *this;
	}

	void table_index::check_valid()
	{
		if (s == nullptr)
			throw std::logic_error("Attempted to use an invalid table_index object");
	}

	local table_index::get_value()
	{
		check_valid();
		if (tblRef == LUA_REFNIL)
			throw std::logic_error("Attempt to get the value of a table_index that is not connected to a table");
		lua_rawgeti(s->L, LUA_REGISTRYINDEX, tblRef);
		lua_rawgeti(s->L, LUA_REGISTRYINDEX, idxRef);
		lua_gettable(s->L, -2);
		local l(*s);
		l.load_value();
		lua_pop(s->L, 2);
		return l;
	}

	/* local */

	local::local() : s(nullptr), L(nullptr), t(type::nil)
	{

	}

	local::local(state &s) : s(&s), L(s.L)
	{

	}

	local::local(const local &lcl)
	{
		copy_value(lcl);
	}

	local::local(bool b) : s(nullptr), L(nullptr), t(type::boolean)
	{
		value.boolean = b;
	}

	local::local(lua_Number n) : s(nullptr), L(nullptr), t(type::number)
	{
		value.number = n;
	}

#ifdef LUA_WRAPPER_STATELESS_STRINGS
	local::local(const char *str) : s(nullptr), L(nullptr), t(type::stateless_string)
	{
		value.string = create_string(str);
	}
#endif

	local::local(lua_CFunction f) : s(nullptr), L(nullptr), t(type::cfunction)
	{
		value.cfunction = f;
	}

	local::local(void *p) : s(nullptr), L(nullptr), t(type::lightuserdata)
	{
		value.lightuserdata = p;
	}

	local::local(state &s, bool b) : s(&s), L(s.L), t(type::boolean)
	{
		value.boolean = b;
	}

	local::local(state &s, lua_Number n) : s(&s), L(s.L), t(type::number)
	{
		value.number = n;
	}

	local::local(state &s, const char *str) : s(&s), L(s.L), t(type::string)
	{
		lua_pushstring(L, str);
		load_ref_value();
	}

	local::local(state &s, lua_CFunction f) : s(&s), L(s.L), t(type::cfunction)
	{
		value.cfunction = f;
	}

	local::local(state &s, void *p) : s(&s), L(s.L), t(type::lightuserdata)
	{
		value.lightuserdata = p;
	}

	local::local(local &&lcl) : s(lcl.s), L(lcl.L), t(lcl.t), value(lcl.value), cargs(0)
	{
		lcl.t = type::nil;
	}

	local::~local()
	{
		release();
	}

	local& local::operator=(const local &rhs)
	{
		release();
		copy_value(rhs);

		return *this;
	}

	void local::copy_value(const local &lcl)
	{
		s = lcl.s;
		L = lcl.L;
		t = lcl.t;
		value = lcl.value;
		cargs = 0; //if lcl.cargs is not 0 then something is seriously wrong

		if (is_ref_type())
		{
			push_ref_value();
			load_ref_value_no_type();
			lua_pop(L, 1);
		}

#ifdef LUA_WRAPPER_STATELESS_STRINGS
		if (t == type::stateless_string)
			value.string = copy_string(lcl.value.string);
#endif
	}

	bool local::is_nil()
	{
		return t == type::nil;
	}

	bool local::is_boolean()
	{
		return t == type::boolean;
	}

	bool local::is_number()
	{
		return t == type::number || t == type::integer;
	}

	bool local::is_integer()
	{
		return t == type::integer;
	}

	bool local::is_string()
	{
		return t == type::string;
	}

	bool local::is_function()
	{
		return t == type::function || t == type::cfunction;
	}

	bool local::is_cfunction()
	{
		return t == type::cfunction;
	}

	bool local::is_userdata()
	{
		return t == type::userdata || t == type::lightuserdata;
	}

	bool local::is_lightuserdata()
	{
		return t == type::lightuserdata;
	}

	bool local::is_thread()
	{
		return t == type::thread;
	}

	bool local::is_table()
	{
		return t == type::table;
	}

	void local::set_as_nil()
	{
		release();
		t = type::nil;
	}

	void local::set_as_boolean(bool b)
	{
		release();
		t = type::boolean;
		value.boolean = b;
	}

	void local::set_as_number(lua_Number n)
	{
		release();
		t = type::number;
		value.number = n;
		integerize();
	}

	void local::set_as_integer(lua_Integer i)
	{
		release();
		t = type::integer;
		value.integer = i;
	}

	void local::set_as_string(const char *s)
	{
#ifdef LUA_WRAPPER_STATELESS_STRINGS
		if (L == nullptr)
		{
			release();
			t = type::stateless_string;
			size_t slen = std::strlen(s);
			value.string = reinterpret_cast<char*>(std::malloc(slen + 1));
			std::memcpy(value.string, s, slen + 1);
			return;
		}
#endif

		check_state();
		release();
		t = type::string;
		lua_pushstring(L, s);
		load_ref_value_no_type();
		lua_pop(L, 1);
	}

	void local::set_as_cfunction(lua_CFunction f)
	{
		release();
		t = type::cfunction;
		value.cfunction = f;
	}

	void local::set_as_function(std::function<void()> f)
	{
		release();
		t = type::function;
		
	}

	void local::set_as_lightuserdata(void *p)
	{
		release();
		t = type::lightuserdata;
		value.lightuserdata = p;
	}

	bool local::to_boolean()
	{
		if (t == type::boolean)
			return value.boolean;

		return false;
	}

	lua_Number local::to_number()
	{
		if (t == type::number)
			return value.number;
		else if (t == type::integer)
			return static_cast<lua_Number>(value.integer);
		else
			return static_cast<lua_Number>(0);
	}

	lua_Integer local::to_integer()
	{
		if (t == type::integer)
			return value.integer;
		else if (t == type::number)
			return numberToInteger(value.number);

		return static_cast<lua_Integer>(0);
	}

	const char* local::to_string()
	{
		if (t == type::string)
		{
			push_ref_value();
			const char *s = lua_tostring(L, -1);
			lua_pop(L, 1);
			return s;
		}
#ifdef LUA_WRAPPER_STATELESS_STRINGS
		else if (t == type::stateless_string)
			return value.string;
#endif

		return nullptr;
	}

	lua_CFunction local::to_cfunction()
	{
		if (t == type::cfunction)
			return value.cfunction;

		return nullptr;
	}

	void* local::to_userdata()
	{
		if (t == type::userdata)
		{
			push_ref_value();
			void *p = lua_touserdata(L, -1);
			lua_pop(L, 1);
			return p;
		}
		else if (t == type::lightuserdata)
			return value.lightuserdata;

		return nullptr;
	}

	void local::local::table_set(local key, local value)
	{
		check_is_table();
		key.check_state_consistancy(L);
		value.check_state_consistancy(L);
		push_ref_value();
		key.push_value(L);
		value.push_value(L);
		lua_settable(L, -3);
		lua_pop(L, 1);
	}

	void local::table_set(lua_Integer key, local value)
	{
		check_is_table();
		value.check_state_consistancy(L);
		push_ref_value();
		lua_pushinteger(L, key);
		value.push_value(L);
		lua_settable(L, -3);
		lua_pop(L, 1);
	}

	local local::table_get(local key)
	{
		check_is_table();
		key.check_state_consistancy(L);
		push_ref_value();
		key.push_value(L);
		lua_gettable(L, -2);
		local lcl(*s);
		lcl.load_value();
		lua_pop(L, 1);
		return lcl;
	}

	local local::table_get(lua_Integer key)
	{
		check_is_table();
		push_ref_value();
		lua_pushinteger(L, key);
		lua_gettable(L, -2);
		local lcl(*s);
		lcl.load_value();
		lua_pop(L, 1);
		return lcl;
	}

	size_t local::length()
	{
		size_t length = 0;

		if (t == type::table || t == type::string)
		{
			push_ref_value();
			length = lua_objlen(L, -1);
			lua_pop(L, 1);
		}
#ifdef LUA_WRAPPER_STATELESS_STRINGS
		else if (t == type::stateless_string)
			length = std::strlen(value.string);
#endif

		return length;
	}

#ifdef LUA_WRAPPER_INDEXABLE_LOCALS
	/*
	table_index& local::operator[](local key)
	{
		check_is_table();
		key.check_state_consistancy(L);
		
		key.push_value(L);
		int r = luaL_ref(L, LUA_REGISTRYINDEX);
		tidx = table_index(*this, r);

		return tidx;
	}
	*/

	table_index& local::operator[](local key)
	{
		check_is_table();
		tindex = table_index(*s, value.ref, key);
		return tindex;
	}

	table_index& local::operator[](lua_Integer key)
	{
		check_is_table();
		local lcl((lua_Number)key);
		tindex = table_index(*s, value.ref, lcl);
		return tindex;
	}

	/*
	table_index& local::operator[](const std::string &key)
	{
		check_is_table();
		local lcl(*s, key.c_str());
		tindex = table_index(*s, value.ref, lcl);
		return tindex;
	}
	*/
#endif

	local& local::operator=(bool rhs)
	{
		set_as_boolean(rhs);
		return *this;
	}

	local& local::operator=(lua_Number rhs)
	{
		set_as_number(rhs);
		return *this;
	}

	local& local::operator=(lua_CFunction rhs)
	{
		set_as_cfunction(rhs);
		return *this;
	}

	local& local::operator=(void *p)
	{
		set_as_lightuserdata(p);
		return *this;
	}

	local& local::operator=(const char *s)
	{
		set_as_string(s);
		return *this;
	}

	bool local::is_ref_type()
	{
		return t == type::string || t == type::function || t == type::userdata || t == type::thread || t == type::table;
	}

	int local::duplicate_ref()
	{
		assert(is_ref_type());
		lua_rawgeti(L, LUA_REGISTRYINDEX, value.ref);
		return value.ref = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	void local::release()
	{
		if (is_table())
			tindex = table_index();

		if (L != nullptr && is_ref_type())
			luaL_unref(L, LUA_REGISTRYINDEX, value.ref);
#ifdef LUA_WRAPPER_STATELESS_STRINGS
		else if (t == type::stateless_string)
			release_string(value.string);
#endif
	}

	void local::load_ref_value_no_type(int idx)
	{
		lua_pushvalue(L, idx);
		value.ref = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	void local::load_ref_value(int idx)
	{
		if (lua_isstring(L, idx))
			t = type::string;
		else if (lua_isfunction(L, idx))
			t = type::function;
		else if (lua_isuserdata(L, idx))
			t = type::userdata;
		else if (lua_isthread(L, idx))
			t = type::thread;
		else if (lua_istable(L, idx))
			t = type::table;

		load_ref_value_no_type(idx);
	}

	void local::load_value(int idx)
	{
		if (lua_isnil(L, idx)) {
			t = type::nil;
		}
		else if (lua_isboolean(L, idx)) {
			t = type::boolean;
			value.boolean = lua_toboolean(L, idx);
		}
#ifdef LUA_WRAPPER_IMPLEMENTATION_LUAJIT
		else if (lua_isnumber(L, idx)) {
			if ((lua_Number)lua_tointeger(L, idx) == lua_tonumber(L, idx)) {
				t = type::integer;
				value.integer = lua_tointeger(L, idx);
			}
			else {
				t = type::number;
				value.number = lua_tonumber(L, idx);
			}
		}
#else
		else if (lua_isinteger(L, idx)) {
			t = type::integer;
			value.integer = lua_tointeger(L, idx);
		}
		else if (lua_isnumber(L, idx)) {
			t = type::number;
			value.number = lua_tonumber(L, idx);
		}
#endif
		else if (lua_iscfunction(L, idx)) {
			t = type::cfunction;
			value.cfunction = lua_tocfunction(L, idx);
		}
		else if (lua_islightuserdata(L, idx)) {
			t = type::lightuserdata;
			value.lightuserdata = lua_touserdata(L, idx);
		}
		else {
			load_ref_value(idx);
		}
	}

	void local::push_ref_value()
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, value.ref);
	}

	void local::push_value(lua_State *L)
	{
		if (L == nullptr)
			L = this->L;

		switch (t)
		{
		case type::nil:              lua_pushnil(L);                                break;
		case type::boolean:          lua_pushboolean(L, value.boolean);             break;
		case type::number:           lua_pushnumber(L, value.number);               break;
		case type::integer:          lua_pushinteger(L, value.integer);             break;
		case type::string:           push_ref_value();                              break;
#ifdef LUA_WRAPPER_STATELESS_STRINGS
		case type::stateless_string: lua_pushstring(L, value.string);               break;
#endif
		case type::function:         push_ref_value();                              break;
		case type::cfunction:        lua_pushcfunction(L, value.cfunction);         break;
		case type::userdata:         push_ref_value();                              break;
		case type::lightuserdata:    lua_pushlightuserdata(L, value.lightuserdata); break;
		case type::thread:           push_ref_value();                              break;
		case type::table:            push_ref_value();                              break;
		}
	}

	lua_Integer local::numberToInteger(lua_Number number)
	{
		static_assert(std::is_same<lua_Number, double>::value, "lua_Number MUST be double");

		union
		{
			double d;
			int i[2];
		} u;

		u.d = number + 6755399441055744.0;
		return u.i[Platform::checkEndianness() == Platform::Endianness::Big ? 1 : 0];
	}

	void local::integerize()
	{
		assert(t == type::number || t == type::integer);

		if (t == type::integer)
			return;

		value.integer = numberToInteger(value.number);
		t = type::integer;
	}

#ifdef LUA_WRAPPER_STATELESS_STRINGS
	char* local::create_string(const char *s)
	{
		size_t slen = std::strlen(s);
		char *ns = reinterpret_cast<char*>(std::malloc(sizeof(uint32_t) + slen + 1));
		(*reinterpret_cast<uint32_t*>(ns)) = 1;
		ns += sizeof(uint32_t);
		std::memcpy(ns, s, slen + 1);
		return ns;
	}

	char* local::copy_string(char *s)
	{
		uint32_t *refc = get_string_refc(s);
		(*refc)++;
		return s;
	}

	void local::release_string(char *s)
	{
		uint32_t *refc = get_string_refc(s);
		if (--(*refc) == 0)
			std::free(refc);
	}

	uint32_t* local::get_string_refc(char *s)
	{
		return reinterpret_cast<uint32_t*>(s - sizeof(uint32_t));
	}
#endif

	void local::check_state()
	{
		if (s == nullptr)
			throw std::logic_error("Cannot operate on a reference-type local that is not attached to a state");
	}

	void local::check_state_consistancy(lua_State *L)
	{
		if (this->L != nullptr && this->L != L)
			throw std::logic_error("Inconsistant state between locals");
	}

	void local::check_is_function()
	{
		if (t != type::function)
			throw std::logic_error("Cannot call a local that is not a function");
	}

	void local::check_is_table()
	{
		if (t != type::table)
			throw std::logic_error("Cannot index a local that is not a table");
	}

	template<typename T, typename... Args>
#if defined(LUA_WRAPPER_SINGLE_RETURN) || defined(LUA_WRAPPER_TABLE_RETURN)
		local local::operator()(T arg, Args... args)
#elif define(LUA_WRAPPER_VECTOR_RETURN)
		std::vector<local> local::operator()(T arg, Args... args)
#endif
	{
		check_is_function();
		prep_call();
		push_call_args(arg, std::forward<Args>(args)...);
		return do_call();
	}

	template<typename T>
#if defined(LUA_WRAPPER_SINGLE_RETURN) || defined(LUA_WRAPPER_TABLE_RETURN)
	local local::operator()(T arg)
#elif define(LUA_WRAPPER_VECTOR_RETURN)
	std::vector<local> local::operator()(T arg)
#endif
	{
		check_is_function();
		prep_call();
		push_call_args(arg);
		return do_call();
	}

#if defined(LUA_WRAPPER_SINGLE_RETURN) || defined(LUA_WRAPPER_TABLE_RETURN)
	local local::operator()()
#elif define(LUA_WRAPPER_VECTOR_RETURN)
	std::vector<local> local::operator()()
#endif
	
	{
		prep_call();
		return do_call();
	}

	void local::prep_call()
	{
		//std::cout << "Pushing function" << std::endl;
		//std::cout << "top: " << lua_gettop(L) << std::endl;

		push_ref_value();
		cargs = 0;
	}

#if defined(LUA_WRAPPER_SINGLE_RETURN) || defined(LUA_WRAPPER_TABLE_RETURN)
	local local::do_call()
#elif define(LUA_WRAPPER_VECTOR_RETURN)
	std::vector<local> local::do_call()
#endif
	{
		int prevTop = lua_gettop(L);

		if (lua_pcall(L, cargs, LUA_MULTRET, 0))
		{
			const char *err = lua_tostring(L, -1);
			lua_pop(L, 1);
			throw std::runtime_error(err);
		}

		int currTop = lua_gettop(L);

		int retc = -prevTop + currTop + cargs + 1;

		if (retc == 0)
			return local(*s);
		else if (retc == 1)
		{
			local lcl(*s);
			lcl.load_value(-1);
			return lcl;
		}
#if defined(LUA_WRAPPER_SINGLE_RETURN)
		else
		{
			throw std::runtime_error("A function may not return more than 1 value");
		}
#elif defined(LUA_WRAPPER_TABLE_RETURN)
		else
		{
			local tbl = s->create_table(retc, 0);
			for (int i = 0; i < retc; i++)
			{
				local retv(*s);
				retv.load_value(-i - 1);
				tbl.table_set(i, retv);
			}
			lua_pop(L, retc);
			return tbl;
		}
#elif define(LUA_WRAPPER_VECTOR_RETURN)
		else
		{
			std::vector<local> returnValues(retc);
			for (int i = 0; i < retc; i++)
			{
				local lcl(*s);
				lcl.load_value(-i - 1);
				returnValues[i] = lcl;
			}
			lua_pop(L, retc);
			return returnValues;
		}
#endif
	}

	template<typename T, typename... Args>
	void local::push_call_args(T arg, Args... args)
	{
		push_call_args(arg);
		push_call_args(std::forward<Args>(args)...);
	}

	template<typename T>
	void local::push_call_args(T arg) = delete;

	template<>
	void local::push_call_args<local>(local arg)
	{
		arg.check_state_consistancy(L);
		arg.push_value(L);
		cargs++;
	}
}
