#ifndef GM_BACKEND_CPP
#define GM_BACKEND_CPP

#include "gm_backend.h"
#include "gm_misc.h"
#include "gm_code_writer.h"
#include "gm_compile_step.h"
#include "gm_backend_cpp_opt_steps.h"
#include "gm_backend_cpp_gen_steps.h"
#include "../backend_cpp/gm_cpplib_words.h"
#include "shl_extensions.h"

#include <list>
#include <algorithm>

//-----------------------------------------------------------------
// interface for graph library Layer
//  ==> will be deprecated
//-----------------------------------------------------------------
class gm_cpp_gen;
class gm_cpplib: public gm_graph_library
{
public:
    gm_cpplib() {
        main = NULL;
    }
    gm_cpplib(gm_cpp_gen* gen) {
        main = gen;
    }
    void set_main(gm_cpp_gen* gen) {
        main = gen;
    }

    virtual const char* get_header_info() {
        return "gm.h";
    }
    virtual const char* get_type_string(ast_typedecl* t);
    virtual const char* get_type_string(int prim_type);

    virtual const char* max_node_index(ast_id* graph);
    virtual const char* max_edge_index(ast_id* graph);
    virtual const char* node_index(ast_id* iter);
    virtual const char* edge_index(ast_id* iter);
    virtual const char* fw_edge_index(ast_id* iter);

    virtual bool do_local_optimize();

    virtual void generate_sent_nop(ast_nop* n);
    virtual void generate_expr_builtin(ast_expr_builtin* e, gm_code_writer& Body);
    virtual void generate_expr_nil(ast_expr* e, gm_code_writer& Body);
    virtual bool add_collection_def(ast_id* set);
    virtual void add_map_def(ast_maptypedecl* map, ast_id* mapId);
    virtual void build_up_language_voca(gm_vocabulary& V);

    virtual bool need_up_initializer(ast_foreach* fe);
    virtual bool need_down_initializer(ast_foreach* fe);
    virtual void generate_up_initializer(ast_foreach* fe, gm_code_writer& Body);
    virtual void generate_down_initializer(ast_foreach* fe, gm_code_writer& Body);
    virtual shl__loop_t generate_foreach_header(ast_foreach* fe, gm_code_writer& Body);

    const char* get_reduction_function_name(GM_REDUCE_T type);

    virtual bool has_optimized_type_name(gm_symtab_entry* sym);
    virtual const char* get_optimized_type_name(gm_symtab_entry* t);

protected:
    gm_cpp_gen* get_main() {return main;}

private:
    //map sizes
    static const int SMALL = 0;
    static const int MEDIUM = 1;
    static const int LARGE = 2;
    static const int PRIORITY_MIN = 3;
    static const int PRIORITY_MAX = 4;

    char str_buf[1024 * 8];
    gm_cpp_gen* main;

    virtual void generate_expr_builtin_field(ast_expr_builtin_field* builtinExpr, gm_code_writer& body);
    const char* get_function_name_graph(int methodId);
    const char* get_function_name_nset(int methodId, bool in_parallel = false);
    const char* get_function_name_nseq(int methodId);
    const char* get_function_name_norder(int methodId);
    const char* get_function_name_map(int methodId, bool in_parallel = false);
    const char* get_function_name_map_seq(int methodId);
    const char* get_function_name_map_par(int methodId);
    void add_arguments_and_thread(gm_code_writer& body, ast_expr_builtin* builtinExpr, bool addThreadId);
    const char* getMapDefaultValueForType(int type);
    const char* getMapTypeString(int mapType);
    const char* getAdditionalMapParameters(int mapType);

    static const char* get_primitive_type_string(int type_id) {
        switch (type_id) {
            case GMTYPE_BYTE:
                return "int8_t";
            case GMTYPE_SHORT:
                return "int16_t";
            case GMTYPE_INT:
                return "int32_t";
            case GMTYPE_LONG:
                return "int64_t";
            case GMTYPE_FLOAT:
                return "float";
            case GMTYPE_DOUBLE:
                return "double";
            case GMTYPE_BOOL:
                return "bool";
            default:
                assert(false);
                return "??";
        }
    }

    static const char* getTypeString(int type) {
        if (gm_is_prim_type(type)) {
            return get_primitive_type_string(type);
        } else if (gm_is_node_type(type)) {
            return NODE_T;
        } else if (gm_is_edge_type(type)) {
            return EDGE_T;
        } else {
            assert(false);
        }
        return NULL;
    }


};

//-----------------------------------------------------------------
// interface for graph library Layer
//-----------------------------------------------------------------
class gm_cpp_gen: public gm_backend, public gm_code_generator
{
    friend class nop_reduce_scalar;
public:
    gm_cpp_gen() :
            gm_code_generator(Body), fname(NULL), dname(NULL), f_header(NULL), f_body(NULL), _target_omp(false), _pblock(false) {
        f_shell = NULL;
        glib = new gm_cpplib(this);
        init();
    }
    gm_cpp_gen(gm_cpplib* l) :
            gm_code_generator(Body), fname(NULL), dname(NULL), f_header(NULL), f_body(NULL), _target_omp(false), _pblock(false) {
        assert(l != NULL);
        glib = l;
        glib->set_main(this);
        init();
    }
protected:
    void init() {
        init_opt_steps();
        init_gen_steps();
        build_up_language_voca();
    }

public:
    virtual ~gm_cpp_gen() {
        close_output_files();
    }
    virtual void setTargetDir(const char* dname);
    virtual void setFileName(const char* fname);

    virtual bool do_local_optimize_lib();
    virtual bool do_local_optimize();
    virtual bool do_generate();
    virtual void do_generate_begin();
    virtual void do_generate_end();

    /*
     */

protected:
    std::list<gm_compile_step*> opt_steps;
    std::list<gm_compile_step*> gen_steps;

    virtual void build_up_language_voca();
    virtual void init_opt_steps();
    virtual void init_gen_steps();

    virtual bool prepare_parallel_for(bool b);
    int _ptr, _indent;

public:
    gm_cpplib* get_lib() {
        return glib;
    }

    //std::list<const char*> local_names;

public:
    virtual void set_target_omp(bool b) {
        _target_omp = b;
    }
    virtual bool is_target_omp() {
        return _target_omp;
    }

protected:
    // data structure for generation
    char *fname;        // current source file (without extension)
    char *dname;        // output directory

    gm_code_writer Header;
    gm_code_writer Body;
    FILE *f_header;
    FILE *f_body;
    FILE *f_shell;
    bool open_output_files();
    void close_output_files(bool remove_files=false);
    void remove_output_files();
    void do_generate_compile_shell(std::map<std::string,std::string>& setup);
    void do_generate_user_main();

    bool _target_omp;
    gm_cpplib* glib; // graph library

    // some common sentence
    virtual void add_include(const char* str1, gm_code_writer& Out, bool is_clib = true, const char* str2 = "");
    virtual void add_ifdef_protection(const char* str);

    //------------------------------------------------------------------------------
    // Generate Method from gm_code_generator
    //------------------------------------------------------------------------------
public:
    virtual void generate_rhs_id(ast_id* i);
    virtual void generate_rhs_field(ast_field* i);
    virtual void generate_expr_builtin(ast_expr* e);
    virtual void generate_expr_minmax(ast_expr* e);
    virtual void generate_expr_abs(ast_expr* e);
    virtual void generate_expr_inf(ast_expr* e);
    virtual void generate_expr_nil(ast_expr* e);

    virtual const char* get_type_string(ast_typedecl* t);
    virtual const char* get_type_string(int prim_type);
    virtual int get_type_id(const char *type_string);
    virtual void generate_lhs_id(ast_id* i);
    virtual void generate_lhs_field(ast_field* i);
    virtual void generate_sent_nop(ast_nop* n);
    virtual void generate_sent_reduce_assign(ast_assign *a);
    virtual void generate_sent_defer_assign(ast_assign *a) {
        assert(false);
    } // should not be here
    virtual void generate_sent_vardecl(ast_vardecl *a);
    virtual void generate_sent_foreach(ast_foreach *a);
    virtual void generate_sent_bfs(ast_bfs* b);
    virtual void generate_sent_block(ast_sentblock *b);
    virtual void generate_sent_block(ast_sentblock* b, bool need_br);
    virtual void generate_sent_return(ast_return *r);
    virtual void generate_sent_call(ast_call* c);
    virtual void generate_sent_assign(ast_assign* a);
    virtual const char* get_function_name_map_reduce_assign(int reduceType);

    virtual void generate_sent_block_enter(ast_sentblock *b);
    virtual void generate_sent_block_exit(ast_sentblock* b);

    virtual void generate_idlist(ast_idlist *i);
    virtual void generate_proc(ast_procdef* proc);
    void generate_proc_decl(ast_procdef* proc, bool is_body_file);

protected:
    bool is_under_parallel_sentblock() {
        return _pblock;
    }
    void set_under_parallel_sentblock(bool b) {
        _pblock = b;
    }

    virtual void declare_prop_def(ast_typedecl* t, ast_id* i);
    virtual void generate_sent_reduce_argmin_assign(ast_assign *a);

    bool _pblock;

protected:
    void generate_bfs_def(ast_bfs* bfs);
    void generate_bfs_body_fw(ast_bfs* bfs);
    void generate_bfs_body_bw(ast_bfs* bfs);
    void generate_bfs_navigator(ast_bfs* bfs);

    const char* i_temp;  // temporary variable name
    char temp[2048];

private:
    const char* get_function_name(int methodId, bool& addThreadId);
    void generate_idlist_primitive(ast_idlist* idList);
    void generate_lhs_default(int type);
    const char* get_lhs_default(int type);
    void generate_sent_map_assign(ast_assign_mapentry* a);
};

extern gm_cpp_gen CPP_BE;

//---------------------------------------------------
// (NOPS) for CPP/CPP_LIB
//---------------------------------------------------
enum nop_enum_cpp
{
    NOP_REDUCE_SCALAR = 1000,
};

class nop_reduce_scalar: public ast_nop
{
public:
    nop_reduce_scalar() :
            ast_nop(NOP_REDUCE_SCALAR) {
    }
    void set_symbols(std::list<gm_symtab_entry*>& O, std::list<gm_symtab_entry*>& N, std::list<int>& R, std::list<std::list<gm_symtab_entry*> >& O_S,
            std::list<std::list<gm_symtab_entry*> >& N_S);

    virtual bool do_rw_analysis();
    void generate(gm_cpp_gen* gen);

public:
    std::list<gm_symtab_entry*> old_s;
    std::list<gm_symtab_entry*> new_s;
    std::list<int> reduce_op;
    std::list<std::list<gm_symtab_entry*> > old_supple; // supplimental lhs for argmin/argmax
    std::list<std::list<gm_symtab_entry*> > new_supple;
};

//-----------------------------------
// define labels (flags for additional information)
//-----------------------------------
DEF_STRING(LABEL_PAR_SCOPE);
DEF_STRING(CPPBE_INFO_HAS_BFS);
DEF_STRING(CPPBE_INFO_IS_PROC_ENTRY);
DEF_STRING(CPPBE_INFO_HAS_PROPDECL);
DEF_STRING(CPPBE_INFO_BFS_SYMBOLS);
DEF_STRING(CPPBE_INFO_BFS_NAME);
DEF_STRING(CPPBE_INFO_BFS_LIST);
DEF_STRING(CPPBE_INFO_COLLECTION_LIST);
DEF_STRING(CPPBE_INFO_COLLECTION_ITERATOR);
DEF_STRING(CPPBE_INFO_COMMON_NBR_ITERATOR);
DEF_STRING(CPPBE_INFO_NEIGHBOR_ITERATOR);
DEF_STRING(CPPBE_INFO_USE_REVERSE_EDGE);
DEF_STRING(CPPBE_INFO_USE_DOWN_NBR);
DEF_STRING(CPPBE_INFO_NEED_SEMI_SORT);
DEF_STRING(CPPBE_INFO_NEED_FROM_INFO);
DEF_STRING(CPPBE_INFO_IS_REVERSE_EDGE);

DEF_STRING(CPPBE_INFO_USE_VECTOR_SEQUENCE);
DEF_STRING(CPPBE_INFO_USE_PRIORITY_MAP_MIN);
DEF_STRING(CPPBE_INFO_USE_PRIORITY_MAP_MAX);

//----------------------------------------
// For runtime
//----------------------------------------
static const char* MAX_THREADS = "gm_rt_get_num_threads";
static const char* THREAD_ID = "gm_rt_thread_id";
static const char* ALLOCATE_BOOL = "gm_rt_allocate_bool";
static const char* ALLOCATE_LONG = "gm_rt_allocate_long";
static const char* ALLOCATE_INT = "gm_rt_allocate_int";
static const char* ALLOCATE_DOUBLE = "gm_rt_allocate_double";
static const char* ALLOCATE_FLOAT = "gm_rt_allocate_float";
static const char* ALLOCATE_NODE = "gm_rt_allocate_node_t";
static const char* ALLOCATE_EDGE = "gm_rt_allocate_edge_t";
static const char* ALLOCATE_COLLECTION = "gm_rt_allocate_collection";
static const char* DEALLOCATE = "gm_rt_deallocate";
static const char* CLEANUP_PTR = "gm_rt_cleanup";
static const char* RT_INIT = "gm_rt_initialize";
static const char* BFS_TEMPLATE = "gm_bfs_template";
static const char* DFS_TEMPLATE = "gm_dfs_template";
static const char* DO_BFS_FORWARD = "do_bfs_forward";
static const char* DO_BFS_REVERSE = "do_bfs_reverse";
static const char* DO_DFS = "do_dfs";
static const char* RT_INCLUDE = "gm.h";
static const char* PREPARE = "prepare";
static const char* FREEZE = "freeze";
static const char* MAKE_REVERSE = "make_reverse_edges";
static const char* SEMI_SORT = "do_semi_sort";
static const char* IS_SEMI_SORTED = "is_semi_sorted";
static const char* PREPARE_FROM_INFO = "prepare_edge_source";

struct sk_prop {

    std::string name;
    std::string type;
};

extern bool sk_lhs;
extern bool sk_lhs_open;
extern char sk_buf[];
extern std::vector<std::string> sk_iterators;
extern std::vector<struct sk_prop> sk_props;
extern std::map<std::string,std::string> sk_array_mapping;

#define SHOAL_PREFIX "shl_"
#define SHOAL_ACTIVATE 1
#define SHOAL_IDX_NAME "__idx__"
#define SHOAL_SUFFIX_RD "_rd"
#define SHOAL_SUFFIX_WR "_wr"

extern std::map<std::string,std::string> f_global;
extern std::map<std::string,std::string> f_thread;

extern std::set<std::string> sk_write_set;
extern std::set<std::string> sk_read_set;

extern std::map<std::string,struct sk_gm_array> sk_gm_arrays;

//#define SK_DEBUG 1

extern std::set<std::string> its;
using namespace std;

static std::string sk_convert_array_name(std::string in)
{
    size_t i = in.find(".");
    if (i!=std::string::npos)
        in.replace(i, 1, "_");

    return std::string(SHOAL_PREFIX) + "_" + in;
}

static bool sk_m_is_iterator(std::string it)
{
    return its.find(it)!=its.end();
}

/*
 * \brief Record array access
 *
 * \param array_name The name of the array after sk_convert_array_name
 */
static void sk_record_array_access(const char* array_name, bool is_indexed,
                                   bool is_write)
{

    if (!is_indexed) {

        string s(array_name);
        assert (sk_gm_arrays.find(s) != sk_gm_arrays.end()); // otherwise the array name used is wrong

        struct sk_gm_array a = sk_gm_arrays[s];
        a.is_indexed = false;
        sk_gm_arrays[s] = a;
    }

#ifdef SK_DEBUG
    sprintf(str_buf, "/* RTS array %s [wr=%d] [idx=%d]*/",
            array_name, is_write, is_indexed);
    Body->push(str_buf);
#endif

    printf("arr access: [%-30s] indexed [%c] write [%c] cost [%s]\n",
           array_name, is_indexed ? 'X' : ' ', is_write ? 'X' : ' ',
           shl__loop_print());

    if (is_write) {
        sk_write_set.insert(array_name);
    } else {
        sk_read_set.insert(array_name);
    }

}

static char* sk_m_array_access_gen(const char* array_name, const char* index,
                   std::string original_array)
{
    char str_buf[1024*8];
    bool is_write = sk_lhs;

    // XXX I don't know what the following is, but it does not work,
    // so let's remove it
    bool is_indexed = std::find(sk_iterators.begin(),
                                sk_iterators.end(), index)!=sk_iterators.end();

    bool is_indexed_2 = sk_m_is_iterator(string(index));

    printf("Adding [%s] ->[%s] to sk_gm_arrays\n",
           array_name, original_array.c_str());

    sk_array_mapping.insert(make_pair(array_name, original_array));
    sk_record_array_access(sk_convert_array_name(string(original_array)).c_str(), is_indexed, is_write);

    if (is_write) {
        sprintf(str_buf, "%s_%s_%s(%s, ", SHOAL_PREFIX, array_name,
                SHOAL_SUFFIX_WR, index);

        sk_lhs_open = true;

    }
    else {
        sprintf(str_buf, "%s_%s_%s(%s)", SHOAL_PREFIX, array_name,
                SHOAL_SUFFIX_RD, index);
    }
    return str_buf;
}

static void sk_m_array_access(gm_code_writer* Body,
                              const char* array_name,
                              const char* index,
                              std::string original_array)
{
#ifdef SHOAL_ACTIVATE
    Body->push(sk_m_array_access_gen(array_name, index, original_array));
#endif
}

static void sk_log(gm_code_writer* Body, const char* log)
{
#ifdef SK_DEBUG
    char str_buf[1024*8];
    sprintf(str_buf, "/* SK %s */", log);
    Body->push(str_buf);
#endif
}

static void sk_iterator(gm_code_writer* Body,
                        const char* it,
                        const char* it_type)
{
#ifdef SK_DEBUG
    char str_buf[1024*8];
    sprintf(str_buf, "new iterator %s of type %s", it, it_type);
    sk_log(Body, str_buf);
#endif

    sk_iterators.push_back(it);
}

static void sk_property(gm_code_writer *Body,
                        const char* prop,
                        const char* prop_type,
                        bool dynamic,
                        bool is_node)
{
    assert (is_node); // If the input is not a node property, is it an edge property?

    printf("found property [%s] of type [%s], dynamic=[%d], is_node=%d\n",
           prop, prop_type, dynamic, is_node);

    sk_props.push_back({std::string(prop), std::string(prop_type)});

    // Remove the stupid * in double*
    // XXX assuming that there is one in prop_type
    std::string t = std::string(prop_type);
    t = t.substr(0, t.find("*"));

    sk_gm_arrays.insert(std::make_pair<std::string,struct sk_gm_array>(sk_convert_array_name(prop),
                                  {sk_convert_array_name(prop),
                                          std::string(prop), t,
                                          std::string(is_node ? "G.num_nodes()" : "G.num_edges()"),
                                          dynamic,
                                          false,
                                          false,
                                          !is_node,
                                          is_node,
                                          true
                                          }));
}

static gm_code_writer sk_temp_buffer(void)
{
    return gm_code_writer();
}

static void sk_temp_buf_flush(gm_code_writer* w, gm_code_writer *org)
{
    w->copy_buffer_content(org);
}

static void sk_forall(gm_code_writer *Body,
                      const char *graph,
                      const char *array)
{
    char str_buf[1000];
    sprintf(str_buf, "found forall graph [%s] array [%s]",
            graph, array);
    sk_log(Body, str_buf);

    // XXX do something
}

static void sk_rhs_end(gm_code_writer *Body)
{
    if (sk_lhs_open) {
#ifdef SK_DEBUG
        Body->push("/* END of accessor */");
#endif
        sk_lhs_open = false;

#ifdef SHOAL_ACTIVATE
        Body->push(')');
#endif
    }
}

static void sk_add_to_frame(const char *type, const char *name, bool global)
{
    std::string s = std::string(type);

    // this is ridiculous ..
    // from: http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());

    if (global) {
        f_global.insert(std::make_pair(name, s.c_str()));
    }
    else {
        f_thread.insert(std::make_pair(name, s.c_str()));
    }
}

static bool sk_arr_is_read(const char* name)
{
    return sk_read_set.find(name)!=sk_read_set.end();

}

static bool sk_arr_is_write(const char* name)
{
    return sk_write_set.find(name)!=sk_write_set.end();
}

static void sk_add_default_arrays(void)
{
    sk_gm_arrays.insert(std::make_pair<std::string, struct sk_gm_array>(sk_convert_array_name("G.begin"),
        {sk_convert_array_name("G.begin"),
                std::string("G.begin"),
                std::string("edge_t"),
                std::string("G.num_nodes()"),
                false,
                true, false, false, true, true
                }));
    sk_gm_arrays.insert(std::make_pair<std::string, struct sk_gm_array>(sk_convert_array_name("G.r_begin"),
        {sk_convert_array_name("G.r_begin"),
                std::string("G.r_begin"),
                std::string("edge_t"),
                std::string("G.num_nodes()"),
                false,
                true, false, false, true, true
                }));

    sk_gm_arrays.insert(std::make_pair<std::string, struct sk_gm_array>(sk_convert_array_name("G.node_idx"),
        {sk_convert_array_name("G.node_idx"),
                std::string("G.node_idx"),
                std::string("node_t"),
                std::string("G.num_edges()"),
                false,
                true, false, true, false, true
                }));
    sk_gm_arrays.insert(std::make_pair<std::string, struct sk_gm_array>(sk_convert_array_name("G.r_node_idx"),
        {sk_convert_array_name("G.r_node_idx"),
                std::string("G.r_node_idx"),
                std::string("node_t"),
                std::string("G.num_edges()"),
                false,
                true, false, true, false, true
                }));

    const char* default_arrays[4] = {
        "begin",
        "r_begin",
        "node_idx",
        "r_node_idx"
    };

    for (int i=0; i<4; i++) {
        char buf[1024];
        sprintf(buf, "G.%s", default_arrays[i]);

        printf("Adding [%s] --> [%s]\n", default_arrays[i], buf);

        sk_array_mapping.insert(make_pair(std::string(default_arrays[i]), std::string(buf)));
    }

}

static void sk_init_accessors(gm_code_writer *Body)
{
    char tmp[1024];

    std::map<std::string,struct sk_gm_array>::iterator i;
    for (i=sk_gm_arrays.begin(); i!=sk_gm_arrays.end(); ++i) {

        struct sk_gm_array a = i->second;

        const char* dest = a.dest.c_str();
        const char* src = a.src.c_str();
        const char* type = a.type.c_str();
        const char* num = a.num.c_str();

        /* sprintf(tmp, "%s* %s = LOOKUP_%s;", type, dest, dest); */
        /* Body->pushln(tmp); */

        sprintf(tmp, "%s* %s __attribute__ ((unused)) = %s__set->get_array();",
                type, dest, dest);
        Body->pushln(tmp);
    }

    int num = 0;

    Body->push("shl_graph shl_G(");
    Body->push(sk_convert_array_name("G.begin").c_str()); Body->push(", ");
    Body->push(sk_convert_array_name("G.r_begin").c_str()); Body->push(", ");
    Body->push(sk_convert_array_name("G.node_idx").c_str()); Body->push(", ");
    Body->push(sk_convert_array_name("G.r_node_idx").c_str());
    Body->pushln(");");

    Body->pushln("SHL__THREAD_INIT();");
    Body->pushln("shl__thread_init();");
}

#endif
