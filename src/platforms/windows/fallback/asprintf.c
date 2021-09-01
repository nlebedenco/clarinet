#include "portability.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


#if !defined(HAVE_GNU_ASPRINTF)
int
asprintf(char **strp, const char *format, ...)
{
	va_list args;
	int ret;

	va_start(args, format);
	ret = vasprintf(strp, format, args);
	va_end(args);
	return (ret);
}
#endif

#if !defined(HAVE_GNU_VASPRINTF)
int
vasprintf(char **strp, const char *format, va_list args)
{
	int len;
	size_t str_size;
	char *str;
	int ret;

	len = _vscprintf(format, args);
	if (len < 0) 
    {
		*strp = NULL;
		return (-1);
	}
    
	str_size = len + 1;
	str = malloc(str_size);
	if (str == NULL) 
    {
		*strp = NULL;
		return (-1);
	}
    
	ret = vsnprintf(str, str_size, format, args);
	if (ret < 0) 
    {
		free(str);
		*strp = NULL;
		return (-1);
	}
    
	*strp = str;
	
    /* vsnprintf() shouldn't truncate the string, as we have allocated a buffer large enough to hold the string, so its
	 * return value should be the number of characters printed. */
	return (ret);
}
#endif
