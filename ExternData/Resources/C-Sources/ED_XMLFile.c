/* ED_XMLFile.c - XML functions
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

#if defined(__gnu_linux__)
#define _GNU_SOURCE 1
#endif

#include <string.h>
#if defined(_MSC_VER)
#define strdup _strdup
#endif
#include "ED_locale.h"
#include "bsxml.h"
#include "ModelicaUtilities.h"
#include "../Include/ED_XMLFile.h"

typedef struct {
	char* fileName;
	XmlNodeRef root;
	ED_LOCALE_TYPE loc;
} XMLFile;

void* ED_createXML(const char* fileName)
{
	XmlParser xmlParser;
	XMLFile* xml = (XMLFile*)malloc(sizeof(XMLFile));
	if (xml == NULL) {
		ModelicaError("Memory allocation error\n");
		return NULL;
	}
	xml->fileName = strdup(fileName);
	if (xml->fileName == NULL) {
		free(xml);
		ModelicaError("Memory allocation error\n");
		return NULL;
	}

	xml->root = XmlParser_parse_file(&xmlParser, fileName);
	if (xml->root == NULL) {
		free(xml->fileName);
		free(xml);
		if (XmlParser_getErrorLineSet(&xmlParser) != 0) {
			ModelicaFormatError("Error \"%s\" in line %lu: Cannot parse file \"%s\"\n",
				XmlParser_getErrorString(&xmlParser), XmlParser_getErrorLine(&xmlParser), fileName);
		}
		else {
			ModelicaFormatError("Cannot read \"%s\": %s\n", fileName, XmlParser_getErrorLine(&xmlParser));
		}
		return NULL;
	}
	xml->loc = ED_INIT_LOCALE;
	return xml;
}

void ED_destroyXML(void* _xml)
{
	XMLFile* xml = (XMLFile*)_xml;
	if (xml != NULL) {
		if (xml->fileName != NULL) {
			free(xml->fileName);
		}
		XmlNode_deleteTree(xml->root);
		ED_FREE_LOCALE(xml->loc);
		free(xml);
	}
}

static char* findValue(XmlNodeRef* root, const char* varName, const char* fileName)
{
	char* token = NULL;
	char* buf = strdup(varName);
	if (buf != NULL) {
		int elementError = 0;
		token = strtok(buf, ".");
		if (token == NULL) {
			elementError = 1;
		}
		while (token != NULL && elementError == 0) {
			int i;
			int foundToken = 0;
			for (i = 0; i < XmlNode_getChildCount(*root); i++) {
				XmlNodeRef child = XmlNode_getChild(*root, i);
				if (XmlNode_isTag(child, token)) {
					*root = child;
					token = strtok(NULL, ".");
					foundToken = 1;
					break;
				}
			}
			if (foundToken == 0) {
				elementError = 1;
			}
		}
		free(buf);
		if (elementError == 1) {
			ModelicaFormatError("Error in line %i: Cannot find element \"%s\" in file \"%s\"\n",
				XmlNode_getLine(*root), varName, fileName);
		}
		XmlNode_getValue(*root, &token);
	}
	else {
		ModelicaError("Memory allocation error\n");
	}
	return token;
}

double ED_getDoubleFromXML(void* _xml, const char* varName)
{
	double ret = 0.;
	XMLFile* xml = (XMLFile*)_xml;
	if (xml != NULL) {
		XmlNodeRef root = xml->root;
		char* token = findValue(&root, varName, xml->fileName);
		if (token != NULL) {
			if (ED_strtod(token, xml->loc, &ret)) {
				ModelicaFormatError("Error in line %i: Cannot read double value \"%s\" from file \"%s\"\n",
					XmlNode_getLine(root), token, xml->fileName);
			}
		}
		else {
			ModelicaFormatError("Error in line %i: Cannot read double value from file \"%s\"\n",
				XmlNode_getLine(root), xml->fileName);
		}
	}
	return ret;
}

const char* ED_getStringFromXML(void* _xml, const char* varName)
{
	XMLFile* xml = (XMLFile*)_xml;
	if (xml != NULL) {
		XmlNodeRef root = xml->root;
		char* token = findValue(&root, varName, xml->fileName);
		if (token != NULL) {
			char* ret = ModelicaAllocateString(strlen(token));
			strcpy(ret, token);
			return (const char*)ret;
		}
		else {
			ModelicaFormatError("Error in line %i: Cannot read value from file \"%s\"\n",
				XmlNode_getLine(root), xml->fileName);
		}
	}
	return "";
}

int ED_getIntFromXML(void* _xml, const char* varName)
{
	int ret = 0;
	XMLFile* xml = (XMLFile*)_xml;
	if (xml != NULL) {
		XmlNodeRef root = xml->root;
		char* token = findValue(&root, varName, xml->fileName);
		if (token != NULL) {
			if (ED_strtoi(token, xml->loc, &ret)) {
				ModelicaFormatError("Error in line %i: Cannot read int value \"%s\" from file \"%s\"\n",
					XmlNode_getLine(root), token, xml->fileName);
			}
		}
		else {
			ModelicaFormatError("Error in line %i: Cannot read int value from file \"%s\"\n",
				XmlNode_getLine(root), xml->fileName);
		}
	}
	return ret;
}

void ED_getDoubleArray1DFromXML(void* _xml, const char* varName, double* a, size_t n)
{
	XMLFile* xml = (XMLFile*)_xml;
	if (xml != NULL) {
		XmlNodeRef root = xml->root;
		int iLevel = 0;
		char* token = findValue(&root, varName, xml->fileName);
		while (token == NULL && XmlNode_getChildCount(root) > 0) {
			/* Try children if root is empty */
			root = XmlNode_getChild(root, 0);
			XmlNode_getValue(root, &token);
			iLevel++;
		}
		if (token != NULL) {
			char* buf = strdup(token);
			if (buf != NULL) {
				size_t i = 0;
				int iSibling = 0;
				XmlNodeRef parent = XmlNode_getParent(root);
				int nSiblings = XmlNode_getChildCount(parent);
				int line = XmlNode_getLine(root);
				int foundSibling = 0;
				token = strtok(buf, "[]{},; \t");
				while (i < n) {
					if (token != NULL) {
						if (ED_strtod(token, xml->loc, &a[i++])) {
							free(buf);
							ModelicaFormatError("Error in line %i: Cannot read double value \"%s\" from file \"%s\"\n",
								line, token, xml->fileName);
							return;
						}
						token = strtok(NULL, "[]{},; \t");
					}
					else if (++iSibling < nSiblings) {
						/* Retrieve value from next sibling */
						XmlNodeRef child = XmlNode_getChild(parent, iSibling);
						if (child != root && XmlNode_isTag(child, XmlNode_getTag(root))) {
							foundSibling = 1;
							XmlNode_getValue(child, &token);
							line = XmlNode_getLine(child);
							free(buf);
							if (token != NULL) {
								buf = strdup(token);
								if (buf != NULL) {
									token = strtok(buf, "[]{},; \t");
								}
								else {
									ModelicaError("Memory allocation error\n");
									return;
								}
							}
							else {
								ModelicaFormatError("Error in line %: Cannot read empty (%lu.) element \"%s\" from file \"%s\"\n",
									line, (unsigned long)(iSibling + 1), varName, xml->fileName);
								return;
							}
						}
					}
					else {
						/* Error: token is NULL and no (more) siblings */
						free(buf);
						if (foundSibling != 0) {
							const char* levels[] = {"", "child ", "grandchild ", "great-grandchild ", "great-great-grandchild "};
							XmlNodeRef child = XmlNode_getChild(parent, nSiblings - 1);
							line = XmlNode_getLine(child);
							if (iLevel > 4) {
								iLevel = 0;
							}
							ModelicaFormatError("Error after line %i: Cannot find %lu. %selement of \"%s\" in file \"%s\"\n",
								line, (unsigned long)(iSibling + 1), levels[iLevel], varName, xml->fileName);
						}
						else {
							ModelicaFormatError("Error in line %i: Cannot read %lu double values of \"%s\" from file \"%s\"\n",
								line, (unsigned long)n, varName, xml->fileName);
						}
						return;
					}
				}
				free(buf);
			}
			else {
				ModelicaError("Memory allocation error\n");
			}
		}
		else {
			ModelicaFormatError("Error in line %i: Cannot read empty element \"%s\" in file \"%s\"\n",
				XmlNode_getLine(root), varName, xml->fileName);
		}
	}
}

void ED_getDoubleArray2DFromXML(void* _xml, const char* varName, double* a, size_t m, size_t n)
{
	ED_getDoubleArray1DFromXML(_xml, varName, a, m*n);
}
