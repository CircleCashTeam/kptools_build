#ifndef _WINSUP_STR_COMPAT_H
#define _WINSUP_STR_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

void *memmem(const void *haystack, int haystacklen, const void *needle, int needlelen);

#ifdef __cplusplus
}
#endif
#endif // _WINSUP_STR_COMPAT_H