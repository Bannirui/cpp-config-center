include(FetchContent)

FetchContent_Declare(
    sqlite_orm
    GIT_REPOSITORY https://github.com/fnc12/sqlite_orm.git
    GIT_TAG        v1.9
)
FetchContent_MakeAvailable(sqlite_orm)
