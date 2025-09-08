
#ifndef SET_LOGIC_TYPE
  #define SET_LOGIC_TYPE(NAME, ...)
#endif

#ifndef SET_PLACEHOLDER
  #define SET_PLACEHOLDER(NAME, ...)
#endif

#ifndef DECLARE_STRING_ENUM
  #define DECLARE_STRING_ENUM(...)
#endif

#ifndef OPTION
  #define OPTION(TYPE, NAME, DESCRIPTION, ...)
#endif

#ifndef BOOLEAN
  #define BOOLEAN(...) OPTION(bool, __VA_ARGS__)
#endif

#ifndef STRING
  #define STRING(...) OPTION(::std::string_view, __VA_ARGS__)
#endif

#ifndef TAG
  #define TAG(NAME, DESCRIPTION) OPTION(void, NAME, DESCRIPTION)
#endif

#ifndef INTEGER
  #define INTEGER(...) OPTION(::std::int_least64_t, __VA_ARGS__)
#endif

#ifndef ALIAS
  #define ALIAS(a, b)
#endif

#ifndef ALLOW_ADDITIONAL_ARGS
  #define ALLOW_ADDITIONAL_ARGS
#endif

#ifndef RENAME
  #define RENAME(OLDNAME, NEWNAME)
#endif

#include program_options_file
TAG(help, "list of all options")

#undef BOOLEAN
#undef STRING
#undef TAG
#undef OPTION
#undef INTEGER
#undef ALIAS
#undef ALLOW_ADDITIONAL_ARGS
#undef DECLARE_STRING_ENUM
#undef RENAME
#undef SET_LOGIC_TYPE
#undef SET_PLACEHOLDER
