#ifndef _PORTABLE
#define _PORTABLE

//TYPEDEF
typedef unsigned __int8		UINT8_T;
typedef unsigned __int16    UINT16_T;
typedef unsigned __int32	UINT32_T;
typedef unsigned __int32	SIZE_T;


typedef UINT16_T BLOÑK_OFFSET;
typedef UINT16_T BLOÑK_SIZE;
typedef UINT8_T  BLOÑK_ALIGMENT;

/* Define NULL pointer value */
#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#ifndef TRUE
#define TRUE 1
#endif 

#ifndef FALSE
#define FALSE 0
#endif

#endif