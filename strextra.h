#ifndef _STRETRA_H_
#define _STRETRA_H_


char * strmerge(char *s1, char *s2);
/*
 * Concatena las cadenas en s1 y s2 devolviendo nueva memoria (debe ser
 * liberada por el llamador con free())
 *
 * USAGE:
 *
 * merge = strmerge(s1, s2);
 *
 * REQUIRES:
 *     s1 != NULL &&  s2 != NULL
 *
 * ENSURES:
 *     merge != NULL && strlen(merge) == strlen(s1) + strlen(s2)
 *
 */

char * strmerge_and_free(char *s1, char *s2);
/*
 * Le concatena a s1 la cadena s2, liberando la memoria que asigna la función strmerge.
 * 
 * USAGE:
 *
 * s1 = strmerge_and_free(s1, s2);
 *
 * REQUIRES:
 *     s1 != NULL && s2 != NULL
 *     unsigned int tam = strlen(s1)
 *     // (s1 != NULL y debe tener memoria alocada)
 *
 * ENSURES:
 *     s1 != NULL y strlen(s1) == tam + strlen(s2)
 */

#endif
