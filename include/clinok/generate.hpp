
#ifndef OPTION
#define OPTION(...)
#endif

#ifndef BOOLEAN
#define BOOLEAN(...) OPTION(bool, __VA_ARGS__)
#endif

#ifndef STRING
#define STRING(...) OPTION(::std::string_view, __VA_ARGS__)
#endif

#ifndef ENUM
#define ENUM(...)
#endif

#ifndef TAG
#define TAG(...) OPTION(void, __VA_ARGS__)
#endif

#ifndef INTEGER
#define INTEGER(...) OPTION(::std::int_least64_t, __VA_ARGS__)
#endif

#ifndef PATH
#define PATH(...) OPTION(::std::filesystem::path, __VA_ARGS__)
#endif

#ifndef ALIAS
#define ALIAS(a, b)
#endif

#include program_options_file
TAG(help, "list of all options")

#undef BOOLEAN
#undef STRING
#undef TAG
#undef ENUM
#undef PATH
#undef OPTION
#undef INTEGER
#undef ALIAS
