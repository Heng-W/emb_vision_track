#ifndef UTIL_COMMON_H
#define UTIL_COMMON_H

// 禁用拷贝构造和拷贝赋值函数
#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&) = delete; \
    TypeName& operator=(const TypeName&) = delete
#endif


#endif // UTIL_COMMON_H
