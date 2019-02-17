
typedef struct ternary_trie_struct{
unsigned short ch;
struct ternary_trie_struct*left,*mid,*right;
struct sym_tab_struct*sym;
}trie_node;

typedef struct sym_tab_struct{
int file_no;
int line_no;
int serial;
struct sym_tab_struct*link;
octa equiv;
}sym_node;


