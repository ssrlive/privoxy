
/*
 * Master list of supported actions.
 *
 * Not really a header, since it generates code.
 *
 * This is included (3 times!) from actions.c
 * Each time, DEFINE_ACTION_MULTI(), DEFINE_ACTION_STRING(),
 * DEFINE_ACTION_BOOL(), and DEFINE_ACTION_ALIAS
 * are defined to suitable values beforehand.
 */
DEFINE_ACTION_MULTI ("add-header",                              ACTION_MULTI_ADD_HEADER)
DEFINE_ACTION_BOOL  ("block",           ACTION_BLOCK)
DEFINE_ACTION_BOOL  ("fast-redirects",  ACTION_FAST_REDIRECTS)
DEFINE_ACTION_BOOL  ("filter",          ACTION_FILTER)
DEFINE_ACTION_BOOL  ("hide-forwarded",  ACTION_HIDE_FORWARDED)
DEFINE_ACTION_STRING("hide-from",       ACTION_HIDE_FROM,       ACTION_STRING_FROM)
DEFINE_ACTION_STRING("hide-referer",    ACTION_HIDE_REFERER,    ACTION_STRING_REFERER)
DEFINE_ACTION_STRING("hide-user-agent", ACTION_HIDE_USER_AGENT, ACTION_STRING_USER_AGENT)
DEFINE_ACTION_BOOL  ("image",           ACTION_IMAGE)
DEFINE_ACTION_BOOL  ("no-cookies-read", ACTION_NO_COOKIE_READ)
DEFINE_ACTION_BOOL  ("no-cookies-set",  ACTION_NO_COOKIE_SET)
DEFINE_ACTION_BOOL  ("no-popups",       ACTION_NO_POPUPS)
DEFINE_ACTION_BOOL  ("vanilla-wafer",   ACTION_VANILLA_WAFER)
DEFINE_ACTION_MULTI ("wafer",                                   ACTION_MULTI_WAFER)
#if DEFINE_ACTION_ALIAS
DEFINE_ACTION_BOOL  ("no-popup",        ACTION_NO_POPUPS)
DEFINE_ACTION_STRING("hide-referrer",   ACTION_HIDE_REFERER,    ACTION_STRING_REFERER)
#endif /* if DEFINE_ACTION_ALIAS */
