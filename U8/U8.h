#pragma once

namespace CopyExample {
	namespace U8 {

uint32_t CharSize(const char *text);
char *NextChar(const char *str);
uint32_t StringLength(const char *str);
int CompareNoCase(const char *str1, const char *str2, uint32_t length = -1);
int Compare(const char *str1, const char *str2, uint32_t length = -1);

	}
}
