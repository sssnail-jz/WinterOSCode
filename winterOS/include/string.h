#ifndef _WINTEROS_STRING_H_
#define _WINTEROS_STRING_H_

PUBLIC void *memcpy(void *p_dst, void *p_src, int size);
PUBLIC void memset(void *p_dst, char ch, int size);
PUBLIC int strlen(const char *p_str);
PUBLIC int strcmp(const char *s1, const char *s2);
PUBLIC char *strcat(char *s1, const char *s2);
PUBLIC char *strcpy(char *dst, const char *src);
PUBLIC int subIndex_tail(char *haystack, char *needle);

#define phys_copy memcpy
#define phys_set memset

#endif
