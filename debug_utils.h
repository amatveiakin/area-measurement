#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

// In debug mode, pause the program.
// In release mode, do nothing.
#ifdef _MSC_VER
#  if defined _DEBUG && (_MSC_VER < 1500 || _MSC_VER >= 1600) // Fix for msvc 2008
#    include <intrin.h>
#    define DEBUG_PAUSE()                 __debugbreak ()
#  else
#    define DEBUG_PAUSE()                 do { } while (0)
#  endif
#else // !_MSC_VER
#  if !defined (QT_NO_DEBUG) && (defined (__i386__) || defined (__x86_64__))
#    define DEBUG_PAUSE()                 asm ("int $3")
#  else
#    define DEBUG_PAUSE()                 do { } while (0)
#  endif
#endif // !_MSC_VER


#define ASSERT_RETURN_V(expression, return_value)   \
do {                                                \
  if (!(expression)) {                              \
    DEBUG_PAUSE ();                                 \
    return return_value;                            \
  }                                                 \
} while (0)

#define ASSERT_RETURN(expression)     ASSERT_RETURN_V(expression,)
#define ERROR_RETURN_V(return_value)  ASSERT_RETURN_V(false,return_value)
#define ERROR_RETURN()                ASSERT_RETURN_V(false,)


#endif // DEBUG_UTILS_H
