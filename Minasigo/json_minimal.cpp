/*Minimal JSON extractor.*/

#include <string.h>
#include <malloc.h>

#include "json_minimal.h"

/*JSON特性体の抽出*/
bool ExtractJsonObject(char* src, const char* name, char** dst)
{
	char* p = nullptr;
	char* pp = src;
	char* q = nullptr;
	char* qq = nullptr;
	size_t nLen = 0;
	int iCount = 0;

	p = strstr(pp, name);
	if (p == nullptr)return false;

	pp = strchr(p, ':');
	if (pp == nullptr)return false;
	++pp;

	for (;;)
	{
		q = strchr(pp, '}');
		if (q == nullptr)return false;

		qq = strchr(pp, '{');
		if (qq == nullptr)break;

		if (q < qq)
		{
			--iCount;
			pp = q + 1;
		}
		else
		{
			++iCount;
			pp = qq + 1;
		}

		if (iCount == 0)break;
	}

	nLen = q - p + 1;
	char* buffer = static_cast<char*>(malloc(nLen + 1));
	if (buffer == nullptr)return false;
	memcpy(buffer, p, nLen);
	*(buffer + nLen) = '\0';
	*dst = buffer;

	return true;

}
/*JSON配列の抽出*/
bool ExtractJsonArray(char* src, const char* name, char** dst)
{
	char* p = nullptr;
	char* pp = src;
	char* q = nullptr;
	char* qq = nullptr;
	size_t nLen = 0;
	int iCount = 0;

	p = strstr(pp, name);
	if (p == nullptr)return false;

	pp = strchr(p, ':');
	if (pp == nullptr)return false;
	++pp;

	for (;;)
	{
		q = strchr(pp, ']');
		if (q == nullptr)return false;

		qq = strchr(pp, '[');
		if (qq == nullptr)break;

		if (q < qq)
		{
			--iCount;
			pp = q + 1;
		}
		else
		{
			++iCount;
			pp = qq + 1;
		}

		if (iCount == 0)break;
	}

	nLen = q - p + 1;
	char* buffer = static_cast<char*>(malloc(nLen + 1));
	if (buffer == nullptr)return false;
	memcpy(buffer, p, nLen);
	*(buffer + nLen) = '\0';
	*dst = buffer;

	return true;
}
/*JSON区切り位置探索*/
char* FindJsonValueEnd(char* src)
{
	const char ref[] = ",}\"";

	for (char* p = src; p != nullptr; ++p)
	{
		for (size_t i = 0; i < sizeof(ref); ++i)
		{
			if (*p == ref[i])
			{
				return p;
			}
		}
	}

	return nullptr;
}
/*JSON要素の値を取得*/
bool GetJsonElementValue(char* src, const char* name, char* dst, size_t dst_size)
{
	char* p = nullptr;
	char* pp = src;
	size_t nLen = 0;

	p = strstr(pp, name);
	if (p == nullptr)return false;

	pp = strchr(p, ':');
	if (pp == nullptr)return false;
	++pp;

	p = FindJsonValueEnd(pp);
	if (p == nullptr)return false;
	if (*p == '"')
	{
		pp = p + 1;
		p = strchr(pp, '"');
		if (p == nullptr)return false;
	}

	nLen = p - pp;
	if (nLen > dst_size)return false;
	memcpy(dst, pp, nLen);
	*(dst + nLen) = '\0';

	return true;
}