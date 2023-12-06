#ifndef JSON_MINIMAL_H_
#define JSON_MINIMAL_H_

bool ExtractJsonObject(char* src, const char* name, char** dst);
bool ExtractJsonArray(char* src, const char* name, char** dst);
bool GetJsonElementValue(char* src, const char* name, char* dst, size_t dst_size);

#endif //JSON_MINIMAL_H_