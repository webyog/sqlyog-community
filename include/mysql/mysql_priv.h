/* internal functions */
MYSQL_DATA *read_rows(MYSQL *mysql,MYSQL_FIELD *mysql_fields, uint fields);
void free_rows(MYSQL_DATA *cur);
MYSQL_FIELD * unpack_fields(MYSQL_DATA *data,MEM_ROOT *alloc,uint fields, my_bool default_value, my_bool long_flag_protocol);
