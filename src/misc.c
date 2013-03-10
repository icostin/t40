#include <t40.h>

/* t40_lib_name *************************************************************/
T40_API char const * C41_CALL t40_lib_name ()
{
  return "topor-v0-"
#if T40_STATIC
    "static"
#else
    "dynamic"
#endif
    "-"
#if NDEBUG
    "release"
#else
    "debug"
#endif
    ;
}

/* t40_status_name **********************************************************/
T40_API char const * C41_CALL t40_status_name (uint_t sc)
{
  switch (sc)
  {
    C41_CASE_RET_STR(T40_OK);
    C41_CASE_RET_STR(T40_FAILED);
    C41_CASE_RET_STR(T40_NO_CODE);
    C41_CASE_RET_STR(T40_MEM_ERROR);
  default:
    return "T40__UNKNOWN";
  }
}

