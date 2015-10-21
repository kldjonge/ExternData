/* ED_XLSFile.c - Excel XLSX functions header
 *
 * Copyright (C) 2015 tbeu
 *
 * This file is part of ExternData.
 * 
 * ExternData is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * ExternData is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with ExternData; if not, see http://www.gnu.org/licenses
 *
 */

#if !defined(ED_XMLFILE_H)
#define ED_XMLFILE_H

#include <stdlib.h>

void* ED_createXML(const char* fileName);
void ED_destroyXML(void* _xml);
double ED_getDoubleFromXML(void* _xml, const char* varName);
const char* ED_getStringFromXML(void* _xml, const char* varName);
int ED_getIntFromXML(void* _xml, const char* varName);
void ED_getDoubleArray1DFromXML(void* _xml, const char* varName, double* a, size_t n);
void ED_getDoubleArray2DFromXML(void* _xml, const char* varName, double* a, size_t m, size_t n);

#endif
