DS_LOADER_BEGIN()

/* DB1 */
#undef DB_NAME
#define DB_NAME my_db1

//{
    /* table 1 */
#   undef TABLE_NAME
#   define TABLE_NAME my_table1
//    {
        DS_FIELD_LOADER(uint32_t, field1)
        DS_FIELD_LOADER(uint32_t, field2)
//    }
    /* table 2 */
#   undef TABLE_NAME
#   define TABLE_NAME my_table2
//    {
        DS_FIELD_LOADER(uint32_t, field1)
        DS_FIELD_LOADER(uint32_t, field2)
//    }
//}

/* DB2 */
#undef DB_NAME
#define DB_NAME my_db2
//{
    /* table 1 */
#    undef TABLE_NAME
#    define TABLE_NAME my_table1
//    {
        DS_FIELD_LOADER(uint32_t, field1)
        DS_FIELD_LOADER(uint32_t, field2)
//    }
//}

DS_LOADER_END()
