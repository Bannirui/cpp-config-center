include(FetchContent)

FetchContent_Declare(
        sqlite_orm
        GIT_REPOSITORY https://github.com/fnc12/sqlite_orm.git
        GIT_TAG        v1.9
)
# 不用FetchContent_MakeAvailable的原因是它的cmake版本太低 我的系统的cmake不能兼容它 只需要sqlite的头文件 所以直接绕过它
FetchContent_GetProperties(sqlite_orm)
if(NOT sqlite_orm_POPULATED)
    FetchContent_Populate(sqlite_orm)
endif()