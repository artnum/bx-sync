#ifndef BX_DATABASE_H__
#define BX_DATABASE_H__

#include <bx_object_value.h>
#include <mysql/mysql.h>
#include <stdbool.h>

/* max name len, excluding '\0' */
#define BX_DATABASE_PARAMETER_MAX_NAME_LENGTH   30

typedef struct s_BXDatabaseField BXDatabaseField;
struct s_BXDatabaseField {
    char * name;
    int id;
};

typedef struct s_BXDatabaseColumn BXDatabaseColumn;
struct s_BXDatabaseColumn {
    int column;
    int64_t i_value;
    double f_value;
    char * c_value;
    size_t value_len;
    enum enum_field_types type;
    bool is_unsigned;
    bool is_null;
    BXDatabaseColumn * next;
};

typedef struct s_BXDatabaseRow BXDatabaseRow;
struct s_BXDatabaseRow {
    int id;
    int column_count;
    BXDatabaseColumn * columns;
    BXDatabaseRow * next;
};

typedef struct s_BXDatabaseParameter BXDatabaseParameter;
struct s_BXDatabaseParameter {
    int param;
    char name[BX_DATABASE_PARAMETER_MAX_NAME_LENGTH];
    size_t name_length;
    void * value;
    size_t value_length;
    enum enum_field_types type;
    bool is_null;
    bool is_unsigned;
    bool error;
};

typedef struct s_BXDatabaseQuery BXDatabaseQuery;
struct s_BXDatabaseQuery {
    MYSQL_STMT * stmt;
    char * query;
    size_t query_length;
    bool has_dataset;
    bool has_failed;
    uint64_t affected_rows;
    MYSQL_RES * result_metadata;

    BXDatabaseParameter * parameters;
    int param_count;
    
    BXDatabaseField * fields;
    int field_count;
    
    BXDatabaseRow * results;
    int row_count;

    bool exectued;
};

BXDatabaseQuery * bx_database_new_query(MYSQL * mysql, const char * query);
bool bx_database_add_param_str(
    BXDatabaseQuery * query,
    const char * name,
    void * value,
    size_t value_len,
    enum enum_field_types type
);
bool bx_database_add_param_int(
    BXDatabaseQuery * query,
    const char * name,
    void * value,
    size_t value_len,
    enum enum_field_types type,
    bool is_unsigned    
);
bool bx_database_replace_param(
    BXDatabaseQuery * query,
    const char * name,
    void * value,
    size_t value_len
);
bool bx_database_execute(BXDatabaseQuery * query);
bool bx_database_results(BXDatabaseQuery * query);
void bx_database_dump_column(BXDatabaseColumn * column);
void bx_database_dump_row(BXDatabaseQuery * query, BXDatabaseRow * row);
void bx_database_free_query(BXDatabaseQuery * query);
const char * bx_database_get_column_name(BXDatabaseQuery * query, BXDatabaseColumn * column);
bool bx_database_add_bxtype (
    BXDatabaseQuery * query,
    const char * name,
    BXGeneric * value
);

#define bx_database_add_param_int8(query, name, value)\
    bx_database_add_param_int(query, name, value, sizeof(int8_t), MYSQL_TYPE_TINY, false)
#define bx_database_add_param_int16(query, name, value)\
    bx_database_add_param_int(query, name, value, sizeof(int16_t), MYSQL_TYPE_SHORT, false)
#define bx_database_add_param_int32(query, name, value)\
    bx_database_add_param_int(query, name, value, sizeof(int32_t), MYSQL_TYPE_LONG, false)
#define bx_database_add_param_int64(query, name, value)\
    bx_database_add_param_int(query, name, value, sizeof(int64_t), MYSQL_TYPE_LONGLONG, false)
#define bx_database_add_param_uint8(query, name, value)\
    bx_database_add_param_int(query, name, value, sizeof(uint8_t), MYSQL_TYPE_TINY, true)
#define bx_database_add_param_uint16(query, name, value)\
    bx_database_add_param_int(query, name, value, sizeof(uint16_t), MYSQL_TYPE_SHORT, true)
#define bx_database_add_param_uint32(query, name, value)\
    bx_database_add_param_int(query, name, value, sizeof(uint32_t), MYSQL_TYPE_LONG, true)
#define bx_database_add_param_uint64(query, name, value)\
    bx_database_add_param_int(query, name, value, sizeof(uint64_t), MYSQL_TYPE_LONGLONG, true)

#define bx_database_add_param_char(query, name, value, value_len)\
    bx_database_add_param_str(query, name, value, value_len, MYSQL_TYPE_STRING)
#define bx_database_add_param_varchar(query, name, value, value_len)\
    bx_database_add_param_str(query, name, value, value_len, MYSQL_TYPE_VAR_STRING)
#define bx_database_add_param_text(query, name, value, value_len)\
    bx_database_add_param_str(query, name, value, value_len, MYSQL_TYPE_BLOB)

#define bx_database_replace_param_int8(query, name, value)\
    bx_database_replace_param(query, name, value, sizeof(int8_t))
#define bx_database_replace_param_int16(query, name, value)\
    bx_database_replace_param(query, name, value, sizeof(int16_t))
#define bx_database_replace_param_int32(query, name, value)\
    bx_database_replace_param(query, name, value, sizeof(int32_t))
#define bx_database_replace_param_int64(query, name, value)\
    bx_database_replace_param(query, name, value, sizeof(int64_t))
#define bx_database_replace_param_uint8(query, name, value)\
    bx_database_replace_param(query, name, value, sizeof(uint8_t))
#define bx_database_replace_param_uint16(query, name, value)\
    bx_database_replace_param(query, name, value, sizeof(uint16_t))
#define bx_database_replace_param_uint32(query, name, value)\
    bx_database_replace_param(query, name, value, sizeof(uint32_t))
#define bx_database_replace_param_uint64(query, name, value)\
    bx_database_replace_param(query, name, value, sizeof(uint64_t))

#define bx_database_replace_param_char(query, name, value, value_len)\
    bx_database_replace_param(query, name, value, value_len)
#define bx_database_replace_param_varchar(query, name, value, value_len)\
    bx_database_replace_param(query, name, value, value_len)
#define bx_database_replace_param_text(query, name, value, value_len)\
    bx_database_replace_param(query, name, value, value_len)

#endif /* BX_DATABASE_H__ */