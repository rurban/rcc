/* GCC Bug #121217 - [15] internal compiler error: Segmentation fault comptypes_equiv_p since 15.1 with -std=c2x
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=121217
 */
/* { dg-do compile } */


typedef union{
        char *nordic_ref;
        unsigned long long int bit_number;
        enum PinMode mode : 2;
        unsigned char value;
    } s; typedef struct{
        union{
            char *nordic_ref;
            unsigned long long int bit_number;
            enum PinMode mode : 2;
            unsigned char value;
        } s;
}
//    13 |         } s;
// 0x2547d65 diagnostic_context::diagnostic_impl(rich_location*, diagnostic_metadata const*, diagnostic_option_id, char const*, __va_list_tag (*) [1], diagnostic_t)
// 0x2569a46 internal_error(char const*, ...)
// 0xa54207 comptypes_equiv_p(tree_node*, tree_node*)
// 0xa460c0 hash_table<c_struct_hasher, false, xcallocator>::find_slot_with_hash(tree_node* const&, unsigned int, insert_option)
// 0xa423e1 finish_struct(unsigned long, tree_node*, tree_node*, tree_node*, c_struct_parse_info*, tree_node**)
// 0xaae9c8 c_parser_declspecs(c_parser*, c_declspecs*, bool, bool, bool, bool, bool, bool, bool, c_lookahead_kind)
// 0xaae8e4 c_parser_declspecs(c_parser*, c_declspecs*, bool, bool, bool, bool, bool, bool, bool, c_lookahead_kind)
// 0xacc7ce c_parse_file()
// 0xb4e6e9 c_common_parse_file()


