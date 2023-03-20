
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

//extern void text_error(const char *format, ...);
extern void psg_abort(int error);
    
// NOTE don't call this: use ourstrcpy() instead. See below.

int _safestrcpy(const char* fileName, const int lineNum, char* dest, size_t destSize, const char* source) {

    size_t sourceLen;
    size_t destCharIndex;
    
    if (dest == NULL) {

        printf("Error: Line %d in file %s provided NULL destination!!\n", lineNum, fileName);
        psg_abort(4201);
        return 0;  // dummy return value: aids testing
    }

    if (destSize <= 0) {

        printf("Error: Line %d in file %s provided destination string size <= 0!!\n", lineNum, fileName);
        psg_abort(4202);
        return 0;  // dummy return value: aids testing
    }

    if (source == NULL) {

        printf("Error: Line %d in file %s provided NULL source!!\n", lineNum, fileName);
        psg_abort(4203);
        return 0;  // dummy return value: aids testing
    }

    sourceLen = strlen(source);

    for (destCharIndex = 0; destCharIndex < destSize; destCharIndex++) {

        if (destCharIndex < sourceLen) {

            dest[destCharIndex] = source[destCharIndex];
        }
        else {

            dest[destCharIndex] = (char) 0;
            
            return 0;  // truncation didn't happen
        }
    }

    if (sourceLen >= destSize) {

        printf("WARNING: STRING TRUNCATION: Line %d in file %s tried to assign %ud chars to %ud size destination\n", lineNum, fileName, (sourceLen+1) , destSize);

        dest[destSize-1] = 0;
        
        return 1;  // truncation happened
    }

    return 0;  // truncation didn't happen
}

// NOTE don't call this: use ourstrcat() instead. See below.

int _safestrcat(const char* fileName, const int lineNum, char* dest, size_t destSize, const char* source) {

    size_t origLen;
    size_t sourceLen;
    size_t destCharIndex;
    size_t srcCharIndex;
    
    if (dest == NULL) {

        printf("Error: Line %d in file %s provided NULL destination!!\n", lineNum, fileName);
        psg_abort(4204);
        return 0;  // dummy return value: aids testing
    }

    if (destSize <= 0) {

        printf("Error: Line %d in file %s provided destination string size <= 0!!\n", lineNum, fileName);
        psg_abort(4205);
        return 0;  // dummy return value: aids testing
    }

    if (source == NULL) {

        printf("Error: Line %d in file %s provided NULL source!!\n", lineNum, fileName);
        psg_abort(4206);
        return 0;  // dummy return value: aids testing
    }

    origLen = strlen(dest);
  
    if (origLen >= destSize) {

        printf("Error: Line %d in file %s provided malformed destination string!!\n", lineNum, fileName);
        psg_abort(4207);
        return 0;  // dummy return value: aids testing
    }
    
    sourceLen = strlen(source);
    
    for (destCharIndex = origLen; destCharIndex < destSize; destCharIndex++) {

        srcCharIndex = destCharIndex - origLen;
        
        if (srcCharIndex < sourceLen) {

            dest[destCharIndex] = source[srcCharIndex];
        }
        else {

            dest[destCharIndex] = (char) 0;
            
            return 0;  // truncation did not happen
        }
    }

    if (origLen + sourceLen >= destSize) {

        dest[destSize-1] = 0;
        
        printf("WARNING: STRING TRUNCATION: Line %d in file %s tried to assign %ud chars to %ud size destination\n", lineNum, fileName, (sourceLen+1) , (destSize - origLen));

        return 1;  // truncation happened
    }

    return 0;  // truncation did not happen
}

// NOTE don't call this: use oursprintf() instead. See below.

int _safesprintf(const char* fileName, const int lineNum, int* retCode, char* dest, size_t destSize, const char* fmt, va_list args) {

    // how big is big enough? how big is too big?
    
    #define MAX_BUFF_SZ_HERE 1280
    
    char buffer[MAX_BUFF_SZ_HERE];
    
    int buffContentsLen;
    
    if (dest == NULL) {

        printf("Error: Line %d in file %s provided NULL destination!!\n", lineNum, fileName);
        psg_abort(4208);
        return 0;  // dummy return value: aids testing
    }

    if (destSize <= 0) {

        printf("Error: Line %d in file %s provided destination string size <= 0!!\n", lineNum, fileName);
        psg_abort(4209);
        return 0;  // dummy return value: aids testing
    }

    if (fmt == NULL) {

        printf("Error: Line %d in file %s provided NULL format string!!\n", lineNum, fileName);
        psg_abort(4210);
        return 0;  // dummy return value: aids testing
    }

    *retCode = vsprintf(buffer, fmt, args);

    buffContentsLen = strlen(buffer);    

    if (buffContentsLen >= MAX_BUFF_SZ_HERE) {

        printf("Error: internal buffer was overwritten on line %d in file %s!!\n", lineNum, fileName);
        psg_abort(4211);
        return 0;  // dummy return value: aids testing
    }

    return _safestrcpy(fileName, lineNum, dest, destSize, buffer);
}

char* ourstrcpy(const char* fileName, const int lineNum, char* dest, size_t destSize, const char* source) {

    _safestrcpy(fileName, lineNum, dest, destSize, source);

    return dest;
}

char* ourstrcat(const char* fileName, const int lineNum, char* dest, size_t destSize, const char* source) {

    _safestrcat(fileName, lineNum, dest, destSize, source);

    return dest;
}

int oursprintf(const char* fileName, const int lineNum, char* dest, size_t destSize, const char* fmt, ...) {

    int retCode = 0;
    
    va_list args;

    va_start(args, fmt);

    _safesprintf(fileName, lineNum, &retCode, dest, destSize, fmt, args);

    va_end(args);

	if (retCode >= destSize) {  // truncation happened: fix sprintf() return value.
	
		retCode = destSize - 1;
	}
    
    return retCode;
}

