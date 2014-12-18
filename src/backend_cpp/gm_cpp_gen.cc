#include <stdio.h>
#include "gm_backend_cpp.h"
#include "gm_error.h"
#include "gm_code_writer.h"
#include "gm_frontend.h"
#include "gm_transform_helper.h"
#include "gm_builtin.h"
#include "gm_cpplib_words.h"
#include "gm_argopts.h"

#include "shl_extensions.h"

std::map<std::string,std::string> f_global;
std::map<std::string,std::string> f_thread;

bool sk_lhs = false;
bool sk_lhs_open = false; // generating write accessor, close after rhs
char sk_buf[1024];
bool sk_fr_global_init = false;
bool sk_fr_thread_init = false;
std::string last_lhs_id;
std::vector<std::string> sk_iterators;
std::vector<struct sk_prop> sk_props;
std::map<std::string,std::string> sk_array_mapping;
std::set<std::string> sk_write_set;
std::set<std::string> sk_read_set;

/**
 * \brief
 *
 * Key is the name of the array as returned by sk_convert_array_name,
 * which I t hink is the full shoal name. shl__G_ .. bla
 */
std::map<std::string,struct sk_gm_array> sk_gm_arrays;

using namespace std;

void gm_cpp_gen::setTargetDir(const char* d) {
    assert(d != NULL);
    if (dname != NULL) delete[] dname;
    dname = new char[strlen(d) + 1];
    strcpy(dname, d);
}

void gm_cpp_gen::setFileName(const char* f) {
    assert(f != NULL);
    if (fname != NULL) delete[] fname;
    fname = new char[strlen(f) + 1];
    strcpy(fname, f);
}

bool gm_cpp_gen::open_output_files() {
    char temp[1024];
    assert(dname != NULL);
    assert(fname != NULL);

    sprintf(temp, "%s/%s.h", dname, fname);
    f_header = fopen(temp, "w");
    if (f_header == NULL) {
        gm_backend_error(GM_ERROR_FILEWRITE_ERROR, temp);
        return false;
    }
    Header.set_output_file(f_header);

    sprintf(temp, "%s/%s.cc", dname, fname);
    f_body = fopen(temp, "w");
    if (f_body == NULL) {
        gm_backend_error(GM_ERROR_FILEWRITE_ERROR, temp);
        return false;
    }
    Body.set_output_file(f_body);

    get_lib()->set_code_writer(&Body);

    if (OPTIONS.get_arg_bool(GMARGFLAG_CPP_CREATE_MAIN)) {
        sprintf(temp, "%s/%s_compile.mk", dname, fname);
        f_shell = fopen(temp, "w");
        if (f_shell == NULL) {
            gm_backend_error(GM_ERROR_FILEWRITE_ERROR, temp);
            return false;
        }
    }
    return true;
}

void gm_cpp_gen::close_output_files(bool remove_files) {
    char temp[1024];
    if (f_header != NULL) {
        Header.flush();
        fclose(f_header);
        if (remove_files) {
            sprintf(temp, "rm %s/%s.h", dname, fname);
            system(temp);
        }
        f_header = NULL;
    }
    if (f_body != NULL) {
        Body.flush();
        fclose(f_body);
        if (remove_files) {
            sprintf(temp, "rm %s/%s.cc", dname, fname);
            system(temp);
        }
        f_body = NULL;
    }

    if (f_shell != NULL) {
        fclose(f_shell);
        f_shell = NULL;
        if (remove_files) {
            sprintf(temp, "rm %s/%s_compile.mk", dname, fname);
            system(temp);
        }
    }
}

void gm_cpp_gen::add_include(const char* string, gm_code_writer& Out, bool is_clib, const char* str2) {
    Out.push("#include ");
    if (is_clib)
        Out.push('<');
    else
        Out.push('"');
    Out.push(string);
    Out.push(str2);
    if (is_clib)
        Out.push('>');
    else
        Out.push('"');
    Out.NL();
}
void gm_cpp_gen::add_ifdef_protection(const char* s) {
    Header.push("#ifndef GM_GENERATED_CPP_");
    Header.push_to_upper(s);
    Header.pushln("_H");
    Header.push("#define GM_GENERATED_CPP_");
    Header.push_to_upper(s);
    Header.pushln("_H");
    Header.NL();
}

void gm_cpp_gen::do_generate_begin() {
    //----------------------------------
    // header
    //----------------------------------
    add_ifdef_protection(fname);
    add_include("stdio.h", Header);
    add_include("stdlib.h", Header);
    add_include("stdint.h", Header);
    add_include("float.h", Header);
    add_include("limits.h", Header);
    add_include("cmath", Header);
    add_include("algorithm", Header);
    add_include("omp.h", Header);
    add_include("shl.h", Header);
    add_include("shl_graph.h", Header);
    add_include("shl_arrays.hpp", Header);
    //add_include(get_lib()->get_header_info(), Header, false);
    add_include(RT_INCLUDE, Header, false);
    Header.NL();

    //----------------------------------------
    // Body
    //----------------------------------------
    sprintf(temp, "%s.h", fname);
    add_include(temp, Body, false);
    add_include("shl.h", Body, false);
    add_include("shl_arrays.hpp", Body, false);
    add_include("omp.h", Body, false);
    Body.NL();
}

void gm_cpp_gen::do_generate_end() {
    char tmp[1024];
    Header.NL();

    Header.pushln("/* w/ SHOAL extensions */");
    std::map<std::string,struct sk_gm_array>::iterator k;

    size_t num_arrays = sk_gm_arrays.size();

    sprintf(tmp, "#define COST shl__estimate_working_set_size(%d, \\\n",
            num_arrays);
    Header.push(tmp);

    int kit = 0;
    for (k=sk_gm_arrays.begin(); k!=sk_gm_arrays.end(); ++k) {

        kit++;
        struct sk_gm_array a = k->second;

        const char* dest = a.dest.c_str();
        const char* src = a.src.c_str();
        const char* type = a.type.c_str();
        const char* num = a.num.c_str();


        // Due to data layout in adjacency lists, node and edge arrays are +1
        if (strcmp(src, "G.begin") == 0 ||
            strcmp(src, "G.r_begin") == 0 ||
            strcmp(src, "G.node_idx") == 0 ||
            strcmp(src, "G.r_node_idx") == 0) {

            num = (std::string("(") + a.num + "+1" + ")").c_str();
        }

        // Header:

        // Allocate array
        sprintf(tmp, "shl__estimate_size<%s>(%s, \"%s\", %s_IS_RO, "
                "%s_IS_DYNAMIC, %s_IS_USED, %s_IS_GRAPH, %s_IS_INDEXED)",
                type,   // 3) type
                num,    // 4) size
                src,    // 5) name of source
                dest,   // 5) read-only property
                dest,   // 6) dynamic property
                dest,   // 7) used property
                dest,   // 8) graph property
                dest);  // 9) indexed property
        Header.push(tmp);

        if (kit!=num_arrays)
            Header.push(",\\\n");

        k->second.init_done = true;
    }

    Header.push(")");
    Header.NL();


    // Dump information about all arrays
    printf("\n");
    printf("+--------------------------------------------------------------------+\n");
    printf("| %-30s     %c    %c    %c    %c    %c    %c   %c |\n",
           "ARRAY", 'N', 'E', 'U', 'R', 'G', 'D', 'I');
    printf("+--------------------------------------------------------------------+\n");
    std::map<std::string,struct sk_gm_array>::iterator i;
    for (i=sk_gm_arrays.begin(); i!=sk_gm_arrays.end(); ++i) {

        struct sk_gm_array a = i->second;

        const char* dest = a.dest.c_str();
        const char* src = a.src.c_str();
        const char* type = a.type.c_str();
        const char* num = a.num.c_str();

        // array name after translation
        const char* s = sk_convert_array_name(std::string(src)).c_str();

        // XXX maybe s == dest ?

        bool is_used = sk_arr_is_read(s) || sk_arr_is_write(s);
        bool is_ro = !sk_arr_is_write(s) && is_used;
        bool is_buildin = a.buildin && is_used;
        bool is_dynamic = a.dynamic && is_used;
        bool is_indexed = a.is_indexed && is_used;

        printf("| %-30s    [%c]  [%c]  [%c]  [%c]  [%c]  [%c] [%c] |\n",
               dest, a.is_node_property ? 'X' : ' ',
               a.is_edge_property ? 'X' : ' ',
               is_used ? 'X' : ' ',
               is_ro ? 'X' : ' ',
               is_buildin ? 'X' : ' ',
               is_dynamic ? 'X' : ' ',
               is_indexed ? 'X' : ' ');
    }
    printf("+--------------------------------------------------------------------+\n");

    // Dump information about all arrays (for ORG)
    printf("\n");
    printf("|-\n");
    printf("| array | node | edge | used | ro | std | dyn | idx |\n");
    printf("|-\n");
    for (i=sk_gm_arrays.begin(); i!=sk_gm_arrays.end(); ++i) {

        struct sk_gm_array a = i->second;

        const char* dest = a.dest.c_str();
        const char* src = a.src.c_str();
        const char* type = a.type.c_str();
        const char* num = a.num.c_str();

        // array name after translation
        const char* s = sk_convert_array_name(std::string(src)).c_str();

        // XXX maybe s == dest ?

        bool is_used = sk_arr_is_read(s) || sk_arr_is_write(s);
        bool is_ro = !sk_arr_is_write(s) && is_used;
        bool is_buildin = a.buildin && is_used;
        bool is_dynamic = a.dynamic && is_used;
        bool is_indexed = a.is_indexed && is_used;

        printf("| =%s= | %c  | %c  | %c  | %c  | %c  | %c | %c |\n",
               dest, a.is_node_property ? 'X' : ' ',
               a.is_edge_property ? 'X' : ' ',
               is_used ? 'X' : ' ',
               is_ro ? 'X' : ' ',
               is_buildin ? 'X' : ' ',
               is_dynamic ? 'X' : ' ',
               is_indexed ? 'X' : ' ');
    }
    printf("|-\n");


    // Print write and read set
    // --------------------------------------------------
    printf("\n");
    printf("+------------------------------------------------+\n");
    printf("| WRITE SET                                      |\n");
    printf("+------------------------------------------------+\n");
    for (std::set<std::string>::iterator i=sk_write_set.begin();
         i!=sk_write_set.end(); i++) {

        printf("| %-46s |\n", (*i).c_str());
    }
    printf("+------------------------------------------------+\n");
    printf("| READ SET                                       |\n");
    printf("+------------------------------------------------+\n");
    for (std::set<std::string>::iterator i=sk_read_set.begin();
         i!=sk_read_set.end(); i++) {

        printf("| %-46s |\n", (*i).c_str());
    }
    printf("+------------------------------------------------+\n");

    // Access functions
    // --------------------------------------------------

    printf("what's in sk_array_mapping: \n");
    for (std::map<std::string,std::string>::iterator i=sk_array_mapping.begin();
         i!=sk_array_mapping.end(); i++) {

        printf("[%s] --> [%s]\n", (*i).first.c_str(), (*i).second.c_str());
    }
    printf("end of map\n");

    for (std::map<std::string,std::string>::iterator i=sk_array_mapping.begin();
         i!=sk_array_mapping.end(); i++) {

        const char *dest = sk_convert_array_name((*i).second).c_str();

        bool is_write = sk_arr_is_write(dest);
        bool is_read = sk_arr_is_read(dest);

        // We need to generate two accessor functions:
        // 1) direct
        // 2) direct with double write (for wr-rep)

        // wr-rep
        // --------------------------------------------------

        // Accessor to arrays! ALWAYS generate that, for simplicity
        sprintf(tmp, "static class arr_thread_ptr<%s> %s_thread_ptr;", shl__get_array_type(dest), dest);
        Header.pushln(tmp);

        sprintf(tmp, "#pragma omp threadprivate(%s_thread_ptr)\n", dest);

        Header.pushln(tmp);

        sprintf(tmp, "#ifdef %s_%s_WR_REP\n", SHOAL_PREFIX, (*i).first.c_str());
        Header.pushln(tmp);

        // Write
        if (is_write) {

            sprintf(tmp, ("#define %s_%s_%s(i, v) { "
                          "%s_%s_thread_ptr.ptr1[i] = v; "
                          "%s_%s_thread_ptr.ptr2[i] = v;  "
                          "}"),
                    SHOAL_PREFIX, (*i).first.c_str(), SHOAL_SUFFIX_WR,
                    SHOAL_PREFIX, (*i).first.c_str(),
                    SHOAL_PREFIX, (*i).first.c_str());
            Header.pushln(tmp);
        }

        // Read
        if (is_read) {
            sprintf(tmp, "#define %s_%s_%s(i) %s_%s_thread_ptr.rep_ptr[i]",
                    SHOAL_PREFIX, (*i).first.c_str(), SHOAL_SUFFIX_RD,
                    SHOAL_PREFIX, (*i).first.c_str());
            Header.pushln(tmp);
        }

        Header.pushln("#else\n");

        // normal
        // --------------------------------------------------

        // Write
        if (is_write) {
            sprintf(tmp, "#define %s_%s_%s(i, v) %s[i] = v", SHOAL_PREFIX,
                    (*i).first.c_str(), SHOAL_SUFFIX_WR, dest);
            Header.pushln(tmp);
        }

        // Read
        if (is_read) {
            sprintf(tmp, "#define %s_%s_%s(i) %s[i]", SHOAL_PREFIX,
                    (*i).first.c_str(), SHOAL_SUFFIX_RD, dest);
            Header.pushln(tmp);
        }

        Header.pushln("#endif\n");
    }


    // Generate init function for per-thread wr-rep accessors
    Header.pushln("// Generate per-thread wr-rep accessors");
    Header.pushln("#define SHL__THREAD_INIT(foo) \\");
    for (std::map<std::string,std::string>::iterator i=sk_array_mapping.begin();
         i!=sk_array_mapping.end(); i++) {

        const char *dest = sk_convert_array_name((*i).second).c_str();

        sprintf(tmp, "shl__wr_rep_ptr_thread_init<%s>(%s__set, &%s_thread_ptr); \\",
                shl__get_array_type(dest), dest, dest);

        Header.pushln(tmp);
    }
    Header.NL();

    Header.NL();
    sprintf(tmp, "struct %sframe {", SHOAL_PREFIX);
    Header.pushln(tmp);

    for (std::map<std::string,std::string>::iterator i=f_global.begin();
         i!=f_global.end(); i++) {

        sprintf(tmp, "%s %s;", (*i).second.c_str(), (*i).first.c_str());
        Header.pushln(tmp);
    }
    Header.pushln("node_t G_num_nodes;");
    Header.pushln("edge_t G_num_edges;");
    Header.pushln("};");
    Header.NL();

#ifdef BARRELFISH
    Header.push("#define FRAME_DEFAULT(f) f = (struct shl_frame *)shl__alloc_struct_shared(sizeof(struct shl_frame))");
#else
    int j = 0;
    Header.push("#define FRAME_DEFAULT(f) struct shl_frame sfr = { ");

    for (std::map<std::string,std::string>::iterator i=f_global.begin();
         i!=f_global.end(); i++) {

        int t = get_type_id((*i).second.c_str());
        Header.push(get_lhs_default(t));
        Header.push(", ");
    }
    // sprintf(tmp, "%s, %s}", get_lhs_default(get_type_id("node_t")),
    //         get_lhs_default(get_type_id("edge_t")));
    sprintf(tmp, "0, 0}");
    Header.push(tmp);
    Header.pushln("; f = &sfr; ");
#endif
    Header.NL();

    sprintf(tmp, "struct %sper_thread_frame {", SHOAL_PREFIX);
    Header.pushln(tmp);

    for (std::map<std::string,std::string>::iterator i=f_thread.begin();
         i!=f_thread.end(); i++) {

        sprintf(tmp, "%s %s;", (*i).second.c_str(), (*i).first.c_str());
        Header.pushln(tmp);
    }

    Header.pushln("};");
    Header.NL();

    Header.push("#define FRAME_THREAD_DEFAULT {");
    j = 0;
    for (std::map<std::string,std::string>::iterator i=f_thread.begin();
         i!=f_thread.end(); i++) {

        if (j++>0)
            Header.push(", ");

        int t = get_type_id((*i).second.c_str());
        Header.push(get_lhs_default(t));
    }
    Header.pushln("}");
    Header.NL();

    Header.pushln("#endif");
}

void gm_cpp_gen::generate_proc(ast_procdef* proc) {
    //-------------------------------
    // declare function name
    //-------------------------------
    generate_proc_decl(proc, false);  // declare in header file

    //-------------------------------
    // BFS definitions
    //-------------------------------
    if (proc->find_info_bool(CPPBE_INFO_HAS_BFS)) {
        ast_extra_info_list* L = (ast_extra_info_list*) proc->find_info(CPPBE_INFO_BFS_LIST);
        assert(L != NULL);
        std::list<void*>::iterator I;
        Body.NL();
        Body.pushln("// BFS/DFS definitions for the procedure");
        for (I = L->get_list().begin(); I != L->get_list().end(); I++) {
            ast_bfs* bfs = (ast_bfs*) *I;
            generate_bfs_def(bfs);
        }
    }

    //-------------------------------
    // function definition
    //-------------------------------
    generate_proc_decl(proc, true);   // declare in body file
    generate_sent(proc->get_body());
    Body.NL();

    return;
}

void gm_cpp_gen::generate_proc_decl(ast_procdef* proc, bool is_body_file) {
    // declare in the header or body
    gm_code_writer& Out = is_body_file ? Body : Header;

    if (!is_body_file && proc->is_local()) return;

    if (proc->is_local()) Out.push("static ");

    // return type
    Out.push_spc(get_type_string(proc->get_return_type()));
    Out.push(proc->get_procname()->get_genname());
    Out.push('(');

    int max_arg_per_line = 2;
    int arg_curr = 0;
    int remain_args = proc->get_in_args().size() + proc->get_out_args().size();
    {
        std::list<ast_argdecl*>& lst = proc->get_in_args();
        std::list<ast_argdecl*>::iterator i;
        for (i = lst.begin(); i != lst.end(); i++) {
            remain_args--;
            arg_curr++;

            ast_typedecl* T = (*i)->get_type();
            Out.push(get_type_string(T));
            if (T->is_primitive() || T->is_property())
                Out.push(" ");
            else
                Out.push("& ");

            if (T->is_property()) {

                const char *sk_name = (*i)->get_idlist()->get_item(0)->get_genname();
                assert(T->is_node_property() || T->is_edge_property());
                sk_property(&Out, sk_name, get_type_string(T), false,
                            T->is_node_property());

            }

            assert((*i)->get_idlist()->get_length() == 1);
            Out.push((*i)->get_idlist()->get_item(0)->get_genname());
            if (remain_args > 0) {
                Out.push(", ");
            }

            if ((arg_curr == max_arg_per_line) && (remain_args > 0)) {
                Out.NL();
                arg_curr = 0;
            }
        }
    }
    {
        std::list<ast_argdecl*>& lst = proc->get_out_args();
        std::list<ast_argdecl*>::iterator i;
        for (i = lst.begin(); i != lst.end(); i++) {
            remain_args--;
            arg_curr++;

            Out.push(get_type_string((*i)->get_type()));
            ast_typedecl* T = (*i)->get_type();
            if (!T->is_property()) Out.push_spc("& ");

            if (T->is_property()) {
                const char *sk_name = (*i)->get_idlist()->get_item(0)->get_genname();
                assert(T->is_node_property() || T->is_edge_property());
                if (is_body_file) {
                    sk_property(&Out, sk_name, get_type_string(T), false,
                                T->is_node_property());
                }
            }

            Out.push((*i)->get_idlist()->get_item(0)->get_genname());

            if (remain_args > 0) {
                Out.push(", ");
            }
            if ((arg_curr == max_arg_per_line) && (remain_args > 0)) {
                Out.NL();
                arg_curr = 0;
            }
        }
    }

    Out.push(')');
    if (!is_body_file) Out.push(';');

    Out.NL();
    return;
}

void gm_cpp_gen::generate_idlist(ast_idlist* idl) {
    int z = idl->get_length();
    for (int i = 0; i < z; i++) {
        ast_id* id = idl->get_item(i);
        generate_lhs_id(id);
        if (i < z - 1) Body.push_spc(',');
    }
}

void gm_cpp_gen::generate_idlist_primitive(ast_idlist* idList) {
    int length = idList->get_length();
    for (int i = 0; i < length; i++) {
        ast_id* id = idList->get_item(i);
        generate_lhs_id(id);
        generate_lhs_default(id->getTypeSummary());
        if (i < length - 1) Body.push_spc(',');
    }
}

void gm_cpp_gen::generate_lhs_default(int type) {
    switch (type) {
        case GMTYPE_BYTE:
        case GMTYPE_SHORT:
        case GMTYPE_INT:
        case GMTYPE_LONG:
            Body.push_spc(" = 0");
            break;
        case GMTYPE_FLOAT:
        case GMTYPE_DOUBLE:
            Body.push_spc(" = 0.0");
            break;
        case GMTYPE_BOOL:
            Body.push_spc(" = false");
            break;
        default:
            assert(false);
            return;
    }
}
const char* gm_cpp_gen::get_lhs_default(int type) {
    switch (type) {
        case GMTYPE_BYTE:
        case GMTYPE_SHORT:
        case GMTYPE_INT:
        case GMTYPE_LONG:
            return "0";
            break;
        case GMTYPE_FLOAT:
        case GMTYPE_DOUBLE:
            return "0.0";
            break;
        case GMTYPE_BOOL:
            return "false";
            break;
        default:
            assert(false);
            return NULL;
    }
}

void sk_init_done(gm_code_writer *Body)
{
    char tmp[1024];
    static bool first = true;

    assert (sk_gm_arrays.begin()!=sk_gm_arrays.end());
    std::map<std::string,struct sk_gm_array>::iterator i;

    if (first) {
        Body->pushln("COST;");
    }

    first = false;

    char timerlabel[512];
    snprintf(timerlabel, 512, "shl__start_timer(2*%u);", sk_gm_arrays.size());
    Body->pushln(timerlabel);
    for (i=sk_gm_arrays.begin(); i!=sk_gm_arrays.end(); ++i) {

        struct sk_gm_array a = i->second;

        if (a.init_done) {
            continue;
        }

        const char* dest = a.dest.c_str();
        const char* src = a.src.c_str();
        const char* type = a.type.c_str();
        const char* num = a.num.c_str();

        // Due to data layout in adjacency lists, node and edge arrays are +1
        if (strcmp(src, "G.begin") == 0 ||
            strcmp(src, "G.r_begin") == 0 ||
            strcmp(src, "G.node_idx") == 0 ||
            strcmp(src, "G.r_node_idx") == 0) {

            num = (std::string("(") + a.num + "+1" + ")").c_str();
        }

        // Allocate array
        // --------------------------------------------------

        sprintf(tmp, "\n#ifdef %s_WR_REP", dest);
        Body->pushln(tmp);

        sprintf(tmp, "shl_array<%s>* %s__set = new "
                "shl_array_wr_rep<%s>(%s, \"%s\", shl__get_rep_id);",
                type,   // 1) type
                dest,   // 2) name
                type,   // 3) type
                num,    // 4) size
                src);   // 5) name of source
        Body->pushln(tmp);

        sprintf(tmp, "%s__set->set_dynamic(false);", dest);
        Body->pushln(tmp);

        sprintf(tmp, "%s__set->set_used(true);", dest);
        Body->pushln(tmp);

        sprintf(tmp, "#else /* %s_WR_REP */", dest);
        Body->pushln(tmp);

        sprintf(tmp, "shl_array<%s>* %s__set = "
                "shl__malloc_array<%s>(%s, \"%s\", %s_IS_RO, %s_IS_DYNAMIC, "
                "%s_IS_USED, %s_IS_GRAPH, %s_IS_INDEXED, true /*do init*/);",
                type,   // 1) type
                dest,   // 2) name
                type,   // 3) type
                num,    // 4) size
                dest,   // 5) name of source
                dest,   // 5) read-only property
                dest,   // 6) dynamic property
                dest,   // 7) used property
                dest,   // 8) graph property
                dest);  // 9) indexed property
        Body->pushln(tmp);

        sprintf(tmp, "#endif /* %s_WR_REP */\n", dest);
        Body->pushln(tmp);

        // Alloc Green Marl array
        sprintf(tmp, "%s__set->alloc();",
                dest);   // 1) name
        Body->pushln(tmp);

        sprintf(tmp, "shl__wr_rep_ptr_thread_init<%s>(%s__set, &%s_thread_ptr);",
                type, dest, dest);
        Body->pushln(tmp);

        snprintf(timerlabel, 512, "shl__step_timer(\"%s: allocate\");", dest);
        Body->pushln(timerlabel);

        // Copy Green Marl array
        sprintf(tmp, "%s__set->copy_from(%s);",
                dest,   // 1) name
                src);   // 2) Green Marl source
        Body->pushln(tmp);

        snprintf(timerlabel, 512, "shl__step_timer(\"%s: copy_from\");", dest);
        Body->pushln(timerlabel);

        i->second.init_done = true;
    }

    Body->pushln("shl__end_timer(\"SHOAL_Copyin\");");

    Body->NL();
}

void sk_copy_func(gm_code_writer *Body, gm_code_writer *Header)
{
    char tmp[1024];

    assert (sk_gm_arrays.begin()!=sk_gm_arrays.end());
    std::map<std::string,struct sk_gm_array>::iterator i;

    for (i=sk_gm_arrays.begin(); i!=sk_gm_arrays.end(); ++i) {

        struct sk_gm_array a = i->second;

        const char* dest = a.dest.c_str();
        const char* src = a.src.c_str();

        const char* s = sk_convert_array_name(std::string(src)).c_str();

        // Specify everything that needs to be copied
        bool is_ro = !sk_arr_is_write(s);
        bool is_used = sk_arr_is_read(s) || sk_arr_is_write(s);
        bool is_graph = a.buildin;

        sprintf(tmp, "#define %s_IS_USED %d", dest, (is_used));
        Header->pushln(tmp);
        sprintf(tmp, "#define %s_IS_RO %d", dest, (is_ro));
        Header->pushln(tmp);
        sprintf(tmp, "#define %s_IS_GRAPH %d", dest, is_graph);
        Header->pushln(tmp);
        sprintf(tmp, "#define %s_IS_DYNAMIC %d", dest, (a.dynamic));
        Header->pushln(tmp);
        sprintf(tmp, "#define %s_IS_INDEXED %d", dest, (a.is_indexed));
        Header->pushln(tmp);
    }
}

void gm_cpp_gen::generate_lhs_id(ast_id* id) {

    if (f_global.find(id->get_genname()) != f_global.end()) {
        Body.push("f->");
    }
    else if (f_thread.find(id->get_genname()) != f_thread.end()) {
        if (!sk_fr_thread_init) {

            char tmp[1024];
            sprintf(tmp, "struct %sper_thread_frame ft = FRAME_THREAD_DEFAULT;", SHOAL_PREFIX);
            Body.pushln(tmp);

            sk_fr_thread_init = true;
        }
        Body.push("ft.");
    }

    Body.push(id->get_genname());
    last_lhs_id = id->get_genname();
}

void gm_cpp_gen::generate_rhs_id(ast_id* id) {
    if (id->getTypeInfo()->is_edge_compatible()) {
        // reverse edge
        if (id->find_info_bool(CPPBE_INFO_IS_REVERSE_EDGE)) {
            Body.push(get_lib()->fw_edge_index(id));
        }
        //else if (id->getTypeInfo()->is_edge_iterator()) {
                // original edge index if semi-sorted
                //sprintf(temp, "%s.%s(%s)",
                    //id->getTypeInfo()->get_target_graph_id()->get_genname(),
                    //GET_ORG_IDX,
                    //id->get_genname()
                //);
                //Body.push(temp);
        //}
        else
            generate_lhs_id(id);
    }
    else {
        generate_lhs_id(id);
    }
}
void gm_cpp_gen::generate_lhs_field(ast_field* f) {

    gm_code_writer skBody;

    const char *sk_name;
    const char *sk_index;

    skBody.push(f->get_second()->get_genname());
    skBody.push('[');
    if (f->getTypeInfo()->is_node_property()) {
        skBody.push(get_lib()->node_index(f->get_first()));
        sk_name = f->get_second()->get_genname();
        sk_index = get_lib()->node_index(f->get_first());
    } else if (f->getTypeInfo()->is_edge_property()) {

        if (f->is_rarrow()) { // [XXX] this feature is still not finished, should be re-visited
            const char* alias_name = f->get_first()->getSymInfo()->find_info_string(CPPBE_INFO_NEIGHBOR_ITERATOR);
            assert(alias_name != NULL);
            assert(strlen(alias_name) > 0);
            skBody.push(alias_name);
            sk_name = f->get_second()->get_genname();
            sk_index = alias_name;
        }
        // check if the edge is a back-edge
        else if (f->get_first()->find_info_bool(CPPBE_INFO_IS_REVERSE_EDGE)) {
            skBody.push(get_lib()->fw_edge_index(f->get_first()));
            sk_name = f->get_second()->get_genname();
            sk_index = get_lib()->fw_edge_index(f->get_first());
        }
        else {
            // original edge index if semi-sorted
            //sprintf(temp, "%s.%s(%s)",
               //f->get_first()->getTypeInfo()->get_target_graph_id()->get_genname(),
               //GET_ORG_IDX,
               //f->get_first()->get_genname()
              //);
              //skBody.push(temp);
              skBody.push(get_lib()->edge_index(f->get_first()));
              sk_name = f->get_second()->get_genname();
              sk_index = get_lib()->edge_index(f->get_first());
        }
    }
    else {
        assert(false);
    }
    skBody.push(']');
#ifdef SHOAL_ACTIVATE
    std::string s = skBody.get_buffer_content();
    printf("buffer content is %s\n", s.c_str());
    sk_m_array_access(&Body, sk_name, sk_index, s.substr(0, s.find('[')));
#else
    skBody.copy_buffer_content(&Body);
#endif
    return;
}

void gm_cpp_gen::generate_rhs_field(ast_field* f) {

    generate_lhs_field(f);
}

int gm_cpp_gen::get_type_id(const char* type_string) {

    if (strcmp(type_string, "int8_t")==0)
        return GMTYPE_BYTE;
    else if (strcmp(type_string, "int16_t")==0)
        return GMTYPE_SHORT;
    else if (strcmp(type_string, "int32_t")==0)
        return GMTYPE_INT;
    else if (strcmp(type_string, "int64_t")==0)
        return GMTYPE_LONG;
    else if (strcmp(type_string, "float")==0)
        return GMTYPE_FLOAT;
    else if (strcmp(type_string, "double")==0)
        return GMTYPE_DOUBLE;
    else if (strcmp(type_string, "bool")==0)
        return GMTYPE_BOOL;
    else
        assert (!"Unknown input to get_type_id");
}

const char* gm_cpp_gen::get_type_string(int type_id) {

    if (gm_is_prim_type(type_id)) {
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
    } else {
        return get_lib()->get_type_string(type_id);
    }
}

const char* gm_cpp_gen::get_type_string(ast_typedecl* t) {
    if ((t == NULL) || (t->is_void())) {
        return "void";
    }

    if (t->is_primitive()) {
        return get_type_string(t->get_typeid());
    } else if (t->is_property()) {
        ast_typedecl* t2 = t->get_target_type();
        assert(t2 != NULL);
        if (t2->is_primitive()) {
            switch (t2->get_typeid()) {
                case GMTYPE_BYTE:
                    return "char_t*";
                case GMTYPE_SHORT:
                    return "int16_t*";
                case GMTYPE_INT:
                    return "int32_t*";
                case GMTYPE_LONG:
                    return "int64_t*";
                case GMTYPE_FLOAT:
                    return "float*";
                case GMTYPE_DOUBLE:
                    return "double*";
                case GMTYPE_BOOL:
                    return "bool*";
                default:
                    assert(false);
                    break;
            }
        } else if (t2->is_nodeedge()) {
            char temp[128];
            sprintf(temp, "%s*", get_lib()->get_type_string(t2));
            return gm_strdup(temp);
        } else if (t2->is_collection()) {
            char temp[128];
            sprintf(temp, "%s<%s>&", PROP_OF_COL, get_lib()->get_type_string(t2));
            return gm_strdup(temp);
        } else {
            assert(false);
        }
    } else if (t->is_map()) {
        char temp[256];
        ast_maptypedecl* mapType = (ast_maptypedecl*) t;
        const char* keyType = get_type_string(mapType->get_key_type());
        const char* valueType = get_type_string(mapType->get_value_type());
        sprintf(temp, "gm_map<%s, %s>", keyType, valueType);
        return gm_strdup(temp);
    } else
        return get_lib()->get_type_string(t);

    return "ERROR";
}

extern bool gm_cpp_should_be_dynamic_scheduling(ast_foreach* f);

void gm_cpp_gen::generate_sent_foreach(ast_foreach* f) {

    int ptr;
    bool need_init_before = get_lib()->need_up_initializer(f);

    if (need_init_before) {
        assert(f->get_parent()->get_nodetype() == AST_SENTBLOCK);
        get_lib()->generate_up_initializer(f, Body);
    }

    bool sk_needs_init = false;

    if (f->is_parallel()) {
        Body.NL();
        sk_needs_init = prepare_parallel_for(gm_cpp_should_be_dynamic_scheduling(f));
    }

    shl__loop_t lt = get_lib()->generate_foreach_header(f, Body);

    if (get_lib()->need_down_initializer(f)) {
        Body.push("{");
        shl__loop_enter(lt);
        get_lib()->generate_down_initializer(f, Body);

        if (f->get_body()->get_nodetype() != AST_SENTBLOCK) {
            generate_sent(f->get_body());
        } else {
            // '{' '} already handled
            generate_sent_block((ast_sentblock*) f->get_body(), false);
        }
        Body.push("}");
        shl__loop_leave(lt);

    } else if (f->get_body()->get_nodetype() == AST_SENTBLOCK) {
        shl__loop_enter(lt);
        generate_sent(f->get_body());
        shl__loop_leave(lt);
    } else {
        shl__loop_enter(lt);
        Body.push_indent();
        generate_sent(f->get_body());
        Body.pop_indent();
        shl__loop_leave(lt);
        Body.NL();
    }

    if (sk_needs_init) {
        Body.pushln("} // opened in prepare_parallel_for");
        Body.NL();
    }
}

void gm_cpp_gen::generate_sent_call(ast_call* c) {
    assert(c->is_builtin_call());
    generate_expr_builtin(c->get_builtin());
    Body.pushln(";");
}

// t: type (node_prop)
// id: field name
void gm_cpp_gen::declare_prop_def(ast_typedecl* t, ast_id * id) {
    ast_typedecl* t2 = t->get_target_type();
    assert(t2 != NULL);

    Body.push(" = ");
    switch (t2->getTypeSummary()) {
        case GMTYPE_INT:
            Body.push(ALLOCATE_INT);
            break;
        case GMTYPE_LONG:
            Body.push(ALLOCATE_LONG);
            break;
        case GMTYPE_BOOL:
            Body.push(ALLOCATE_BOOL);
            break;
        case GMTYPE_DOUBLE:
            Body.push(ALLOCATE_DOUBLE);
            break;
        case GMTYPE_FLOAT:
            Body.push(ALLOCATE_FLOAT);
            break;
        case GMTYPE_NODE:
            Body.push(ALLOCATE_NODE);
            break;
        case GMTYPE_EDGE:
            Body.push(ALLOCATE_EDGE);
            break;
        case GMTYPE_NSET:
        case GMTYPE_ESET:
        case GMTYPE_NSEQ:
        case GMTYPE_ESEQ:
        case GMTYPE_NORDER:
        case GMTYPE_EORDER: {
            char temp[128];
            bool lazyInitialization = false; //TODO: get some information here to check if lazy init is better
            sprintf(temp, "%s<%s, %s>", ALLOCATE_COLLECTION, get_lib()->get_type_string(t2), (lazyInitialization ? "true" : "false"));
            Body.push(temp);
            break;
        }
        default:
            assert(false);
            break;
    }

    Body.push('(');
    if (t->is_node_property()) {
        Body.push(get_lib()->max_node_index(t->get_target_graph_id()));
    } else if (t->is_edge_property()) {
        Body.push(get_lib()->max_edge_index(t->get_target_graph_id()));
    }
    Body.push(',');
    Body.push(THREAD_ID);
    Body.pushln("());");

    /*
     */

    // register to memory controller
}

void gm_cpp_gen::generate_sent_vardecl(ast_vardecl* v) {
    ast_typedecl* t = v->get_type();

    if (t->is_collection_of_collection()) {
        Body.push(get_type_string(t));
        ast_typedecl* targetType = t->get_target_type();
        Body.push("<");
        Body.push(get_type_string(t->getTargetTypeSummary()));
        Body.push("> ");
        ast_idlist* idl = v->get_idlist();
        assert(idl->get_length() == 1);
        generate_lhs_id(idl->get_item(0));
        get_lib()->add_collection_def(idl->get_item(0));
        return;
    }

    if (t->is_map()) {
        ast_maptypedecl* map = (ast_maptypedecl*) t;
        ast_idlist* idl = v->get_idlist();
        assert(idl->get_length() == 1);
        get_lib()->add_map_def(map, idl->get_item(0));
        return;
    }

    gm_code_writer skBody;
    skBody.sk_start();

    if (t->is_sequence_collection()) {
        //for sequence-list-vector optimization
        ast_id* id = v->get_idlist()->get_item(0);
        const char* type_string;
        if(get_lib()->has_optimized_type_name(id->getSymInfo())) {
            type_string = get_lib()->get_optimized_type_name(id->getSymInfo());
        } else {
            type_string = get_type_string(t);
        }
        skBody.push_spc(type_string);
    } else {
        skBody.push_spc(get_type_string(t));
    }

    // This is the type! We need this to generate the struct.
    // BUT: we do NOT need to generate anything in case this is global state
    std::string sk_type = skBody.sk_end();

    bool sk_on_frame = true;
    std::string sk_name = "NONE";

    if (t->is_property()) {

        sk_on_frame = false;
        skBody.copy_buffer_content(&Body);

        ast_idlist* idl = v->get_idlist();
        assert(idl->get_length() == 1);
        generate_lhs_id(idl->get_item(0));
        declare_prop_def(t, idl->get_item(0));

        assert(t->is_node_property() || t->is_edge_property());
        sk_property(&Body, idl->get_item(0)->get_genname(),
                    get_type_string(t), true,
                    t->is_node_property());
        sk_init_done(&Body);

    } else if (t->is_collection()) {
        ast_idlist* idl = v->get_idlist();
        assert(idl->get_length() == 1);
        generate_lhs_id(idl->get_item(0));
        get_lib()->add_collection_def(idl->get_item(0));
    } else if (t->is_primitive()) {
        Body.sk_disable(); Body.sk_start();
        generate_idlist_primitive(v->get_idlist());
        printf("%d\n", v->get_idlist()->get_length());
        sk_name = last_lhs_id;
        skBody.pushln(";");
        Body.sk_revert();
    } else {
        generate_idlist(v->get_idlist());
        skBody.pushln(";");
    }

    if (sk_on_frame) {

        sk_add_to_frame(sk_type.c_str(), sk_name.c_str(),
                        !is_under_parallel_sentblock());
    }
}

const char* gm_cpp_gen::get_function_name_map_reduce_assign(int reduceType) {

    switch (reduceType) {
        case GMREDUCE_PLUS:
            return "changeValueAtomicAdd";
        default:
            assert(false);
            return "ERROR";
    }
}

void gm_cpp_gen::generate_sent_map_assign(ast_assign_mapentry* a) {
    ast_mapaccess* mapAccess = a->to_assign_mapentry()->get_lhs_mapaccess();
    ast_id* map = mapAccess->get_map_id();

    char buffer[256];
    if (a->is_under_parallel_execution()) {
        if (a->is_reduce_assign() && a->get_reduce_type() == GMREDUCE_PLUS) {
            sprintf(buffer, "%s.%s(", map->get_genname(), get_function_name_map_reduce_assign(a->get_reduce_type()));
        } else {
            sprintf(buffer, "%s.setValue_par(", map->get_genname());
        }
    } else {
        if (a->is_reduce_assign() && a->get_reduce_type() == GMREDUCE_PLUS) {
            //TODO do this without CAS overhead
            sprintf(buffer, "%s.%s(", map->get_genname(), get_function_name_map_reduce_assign(a->get_reduce_type()));
        } else {
            sprintf(buffer, "%s.setValue_seq(", map->get_genname());
        }
    }
    Body.push(buffer);

    ast_expr* key = mapAccess->get_key_expr();
    generate_expr(key);
    Body.push(", ");
    generate_expr(a->get_rhs());
    Body.pushln(");");
}

void gm_cpp_gen::generate_sent_assign(ast_assign* a) {

    sk_lhs = true;
    if (a->is_target_scalar()) {
        ast_id* leftHandSide = a->get_lhs_scala();
        if (leftHandSide->is_instantly_assigned()) { //we have to add the variable declaration here
            Body.push("/*1*/");
            Body.push(get_lib()->get_type_string(leftHandSide->getTypeSummary()));
            if (a->is_reference()) {
                Body.push("& ");
            } else {
                Body.push(" ");
            }
            Body.push("/*2*/");
        }
        generate_lhs_id(a->get_lhs_scala());
    } else if (a->is_target_map_entry()) {
        generate_sent_map_assign(a->to_assign_mapentry());
        return;
    } else {
        generate_lhs_field(a->get_lhs_field());
    }

    sk_lhs = false;

    if (!sk_lhs_open)
        _Body.push(" = ");

    generate_expr(a->get_rhs());

    sk_rhs_end(&_Body);

    _Body.pushln(" ;");
}

void gm_cpp_gen::generate_sent_block(ast_sentblock* sb) {
    generate_sent_block(sb, true);
}

void gm_cpp_gen::generate_sent_block_enter(ast_sentblock* sb) {
    if (sb->find_info_bool(CPPBE_INFO_IS_PROC_ENTRY) && !FE.get_current_proc()->is_local()) {
        Body.pushln("//Initializations");
        sprintf(temp, "%s();", RT_INIT);
        Body.pushln(temp);

        //----------------------------------------------------
        // freeze graph instances
        //----------------------------------------------------
        ast_procdef* proc = FE.get_current_proc();
        gm_symtab* vars = proc->get_symtab_var();
        gm_symtab* fields = proc->get_symtab_field();
        //std::vector<gm_symtab_entry*>& E = vars-> get_entries();
        //for(int i=0;i<E.size();i++) {
        // gm_symtab_entry* e = E[i];
        std::set<gm_symtab_entry*>& E = vars->get_entries();
        std::set<gm_symtab_entry*>::iterator I;
        for (I = E.begin(); I != E.end(); I++) {
            gm_symtab_entry* e = *I;
            if (e->getType()->is_graph()) {
                sprintf(temp, "%s.%s();", e->getId()->get_genname(), FREEZE);
                Body.pushln(temp);

                // currrently every graph is an argument
                if (e->find_info_bool(CPPBE_INFO_USE_REVERSE_EDGE)) {
                    sprintf(temp, "%s.%s();", e->getId()->get_genname(), MAKE_REVERSE);
                    Body.pushln(temp);
                }
                if (e->find_info_bool(CPPBE_INFO_NEED_SEMI_SORT)) {
                    bool has_edge_prop = false;
#if 0
                    // Semi-sorting must be done before edge-property creation
                    //std::vector<gm_symtab_entry*>& F = fields-> get_entries();
                    //for(int j=0;j<F.size();j++) {
                    // gm_symtab_entry* f = F[j];
                    std::set<gm_symtab_entry*>& F = fields->get_entries();
                    std::set<gm_symtab_entry*>::iterator J;
                    for (J = F.begin(); J != F.end(); J++) {
                        gm_symtab_entry* f = *J;
                        if ((f->getType()->get_target_graph_sym() == e) && (f->getType()->is_edge_property())) has_edge_prop = true;
                    }

                    if (has_edge_prop) {
                        Body.pushln("//[xxx] edge property must be created before semi-sorting");
                        sprintf(temp, "assert(%s.%s());", e->getId()->get_genname(), IS_SEMI_SORTED);
                        Body.pushln(temp);
                    } else {
                        sprintf(temp, "%s.%s();", e->getId()->get_genname(), SEMI_SORT);
                        Body.pushln(temp);
                    }
#else
                    sprintf(temp, "%s.%s();", e->getId()->get_genname(), SEMI_SORT);
                    Body.pushln(temp);
#endif
                }

                if (e->find_info_bool(CPPBE_INFO_NEED_FROM_INFO)) {
                    sprintf(temp, "%s.%s();", e->getId()->get_genname(), PREPARE_FROM_INFO);
                    Body.pushln(temp);
                }
            }

        }

        // SK: Graph has just been intialized, copy it and
        // initialize global frame
        assert (!sk_fr_global_init);
        sk_fr_global_init = true;

        sk_add_default_arrays();
        sk_init_done(&Body);

        char tmp[1024];
        sprintf(tmp, "struct %sframe *f;", SHOAL_PREFIX);
        Body.pushln(tmp);
        Body.pushln("FRAME_DEFAULT(f);");
        Body.pushln("f->G_num_nodes = G.num_nodes();");
        Body.pushln("f->G_num_edges = G.num_edges();");

        Body.NL();
    }

}

void gm_cpp_gen::generate_sent_block(ast_sentblock* sb, bool need_br) {

    bool sk_parallel = false;
    if (is_target_omp()) {
        bool is_par_scope = sb->find_info_bool(LABEL_PAR_SCOPE);
        if (is_par_scope) {
            assert(is_under_parallel_sentblock() == false);
            set_under_parallel_sentblock(true);
            need_br = true;
            Body.pushln("#pragma omp parallel");
            sk_parallel = true;
        }
    }

    if (need_br) Body.pushln("{");

    assert(!sk_parallel || need_br);
    if (sk_parallel) sk_init_accessors(&Body);

    // sentblock exit
    generate_sent_block_enter(sb);

    std::list<ast_sent*>& sents = sb->get_sents();
    std::list<ast_sent*>::iterator it;
    bool vardecl_started = false;
    bool other_started = false;
    for (it = sents.begin(); it != sents.end(); it++) {
        // insert newline after end of VARDECL
        ast_sent* s = *it;
        if (!vardecl_started) {
            if (s->get_nodetype() == AST_VARDECL) vardecl_started = true;
        } else {
            if (other_started == false) {
                if (s->get_nodetype() != AST_VARDECL) {
                    Body.NL();
                    other_started = true;
                }
            }
        }
        generate_sent(*it);
    }

    // sentblock exit
    generate_sent_block_exit(sb);

    if (need_br) Body.pushln("}");

    if (sb->find_info_bool(CPPBE_INFO_IS_PROC_ENTRY))
        sk_copy_func(&Body, &Header);

    if (is_under_parallel_sentblock()) set_under_parallel_sentblock(false);

    return;
}

void gm_cpp_gen::generate_sent_block_exit(ast_sentblock* sb) {
    bool has_prop_decl = sb->find_info_bool(CPPBE_INFO_HAS_PROPDECL);
    bool is_proc_entry = sb->find_info_bool(CPPBE_INFO_IS_PROC_ENTRY);
    assert(sb->get_nodetype() == AST_SENTBLOCK);
    bool has_return_ahead = gm_check_if_end_with_return(sb);

    char timerlabel[512];


    if (has_prop_decl && !has_return_ahead) {
        if (is_proc_entry) {
            Body.pushln("shl__end();\n");

            // SK: copy back arrays into original arrays! Otherwise,
            // accesses from the main file (or any other calling file
            // for that matter) might be returning the wrong result.
            char tmp[1024];

            assert (sk_gm_arrays.begin()!=sk_gm_arrays.end());
            std::map<std::string,struct sk_gm_array>::iterator i;

            char timerlabel[512];
            snprintf(timerlabel, 512, "shl__start_timer(%u);", sk_gm_arrays.size());
            Body.pushln(timerlabel);
            for (i=sk_gm_arrays.begin(); i!=sk_gm_arrays.end(); ++i) {

                struct sk_gm_array a = i->second;

                const char* dest = a.dest.c_str();
                const char* src = a.src.c_str();
                const char* type = a.type.c_str();
                const char* num = a.num.c_str();

                // Copy Green Marl array back
                sprintf(tmp, "%s__set->copy_back(%s);", dest, src);   // 1) name
                Body.pushln(tmp);
                snprintf(timerlabel, 512, "shl__step_timer(\"%s\");", dest);
                Body.pushln(timerlabel);
            }

            Body.pushln("shl__end_timer(\"SHOAL_Copyback\");\n");

            for (i=sk_gm_arrays.begin(); i!=sk_gm_arrays.end(); ++i) {

                 struct sk_gm_array a = i->second;

                 const char* dest = a.dest.c_str();
                 const char* src = a.src.c_str();
                 const char* type = a.type.c_str();
                 const char* num = a.num.c_str();

                 // Copy Green Marl array back
                 sprintf(tmp, "%s__set->print_crc();", dest, src);   // 1) name
                 Body.pushln(tmp);
             }



            sprintf(temp, "%s();", CLEANUP_PTR);
            Body.pushln(temp);

        } else {
            Body.NL();
            gm_symtab* tab = sb->get_symtab_field();
            //std::vector<gm_symtab_entry*>& entries = tab->get_entries();
            //std::vector<gm_symtab_entry*>::iterator I;
            std::set<gm_symtab_entry*>& entries = tab->get_entries();
            std::set<gm_symtab_entry*>::iterator I;
            for (I = entries.begin(); I != entries.end(); I++) {
                gm_symtab_entry *e = *I;
                sprintf(temp, "%s(%s,%s());", DEALLOCATE, e->getId()->get_genname(), THREAD_ID);
                Body.pushln(temp);
            }
        }
    }

}

void gm_cpp_gen::generate_sent_reduce_assign(ast_assign *a) {
    if (a->is_argminmax_assign()) {
        generate_sent_reduce_argmin_assign(a);
        return;
    }

    GM_REDUCE_T r_type = (GM_REDUCE_T) a->get_reduce_type();
    const char* method_name = get_lib()->get_reduction_function_name(r_type);
    bool is_scalar = (a->get_lhs_type() == GMASSIGN_LHS_SCALA);

    ast_typedecl* lhs_target_type;
    if (a->get_lhs_type() == GMASSIGN_LHS_SCALA) {
        lhs_target_type = a->get_lhs_scala()->getTypeInfo();
    } else {
        lhs_target_type = a->get_lhs_field()->getTypeInfo()->get_target_type();
    }

    char templateParameter[32];
    if (r_type != GMREDUCE_OR && r_type != GMREDUCE_AND) {
        sprintf(templateParameter, "<%s>", get_type_string(lhs_target_type));
    } else {
        sprintf(templateParameter, "");
    }

    Body.push(method_name);
    Body.push(templateParameter);
    Body.push("(&");
    if (is_scalar)
        generate_rhs_id(a->get_lhs_scala());
    else
        generate_rhs_field(a->get_lhs_field());
    Body.push(", ");
    generate_expr(a->get_rhs());
    ;
    Body.push(");\n");
}

void gm_cpp_gen::generate_sent_reduce_argmin_assign(ast_assign *a) {
    assert(a->is_argminmax_assign());

    //-----------------------------------------------
    // <LHS; L1,...> min= <RHS; R1,...>;
    //
    // {
    //    RHS_temp = RHS;
    //    if (LHS > RHS_temp) {
    //        <type> L1_temp = R1;
    //        ...
    //        LOCK(LHS) // if LHS is scalar,
    //                  // if LHS is field, lock the node
    //            if (LHS > RHS_temp) {
    //               LHS = RHS_temp;
    //               L1 = L1_temp;  // no need to lock for L1.
    //               ...
    //            }
    //        UNLOCK(LHS)
    //    }
    // }
    //-----------------------------------------------
    Body.pushln("{ // argmin(argmax) - test and test-and-set");
    const char* rhs_temp;
    int t;
    if (a->is_target_scalar()) {
        t = a->get_lhs_scala()->getTypeSummary();
        rhs_temp = (const char*) FE.voca_temp_name_and_add(a->get_lhs_scala()->get_genname(), "_new");
    } else {
        t = a->get_lhs_field()->get_second()->getTargetTypeSummary();
        rhs_temp = (const char*) FE.voca_temp_name_and_add(a->get_lhs_field()->get_second()->get_genname(), "_new");
    }
    Body.push(get_type_string(t));
    Body.SPC();
    Body.push(rhs_temp);
    Body.push(" = ");
    generate_expr(a->get_rhs());
    Body.pushln(";");

    Body.push("if (");
    if (a->is_target_scalar()) {
        generate_rhs_id(a->get_lhs_scala());
    } else {
        generate_rhs_field(a->get_lhs_field());
    }
    if (a->get_reduce_type() == GMREDUCE_MIN) {
        Body.push(">");
    } else {
        Body.push("<");
    }
    Body.push(rhs_temp);
    Body.pushln(") {");

    std::list<ast_node*> L = a->get_lhs_list();
    std::list<ast_expr*> R = a->get_rhs_list();
    std::list<ast_node*>::iterator I;
    std::list<ast_expr*>::iterator J;
    char** names = new char*[L.size()];
    int i = 0;
    J = R.begin();
    for (I = L.begin(); I != L.end(); I++, J++, i++) {
        ast_node* n = *I;
        ast_id* id;
        int type;
        if (n->get_nodetype() == AST_ID) {
            id = (ast_id*) n;
            type = id->getTypeSummary();
        } else {
            assert(n->get_nodetype() == AST_FIELD);
            ast_field* f = (ast_field*) n;
            id = f->get_second();
            type = id->getTargetTypeSummary();
        }
        names[i] = (char*) FE.voca_temp_name_and_add(id->get_genname(), "_arg");
        Body.push(get_type_string(type));
        Body.SPC();
        Body.push(names[i]);
        Body.push(" = ");
        generate_expr(*J);
        Body.pushln(";");
    }

    // LOCK, UNLOCK
    const char* LOCK_FN_NAME;
    const char* UNLOCK_FN_NAME;
    if (a->is_target_scalar()) {
        LOCK_FN_NAME = "gm_spinlock_acquire_for_ptr";
        UNLOCK_FN_NAME = "gm_spinlock_release_for_ptr";
    } else {
        ast_id* drv = a->get_lhs_field()->get_first();
        if (drv->getTypeInfo()->is_node_compatible()) {
            LOCK_FN_NAME = "gm_spinlock_acquire_for_node";
            UNLOCK_FN_NAME = "gm_spinlock_release_for_node";
        } else if (drv->getTypeInfo()->is_edge_compatible()) {
            LOCK_FN_NAME = "gm_spinlock_acquire_for_edge";
            UNLOCK_FN_NAME = "gm_spinlock_release_for_edge";
        } else {
            assert(false);
        }
    }

    Body.push(LOCK_FN_NAME);
    Body.push("(");
    if (a->is_target_scalar()) {
        Body.push("&");
        generate_rhs_id(a->get_lhs_scala());
    } else {
        generate_rhs_id(a->get_lhs_field()->get_first());
    }
    Body.pushln(");");

    Body.push("if (");
    if (a->is_target_scalar()) {
        generate_rhs_id(a->get_lhs_scala());
    } else {
        generate_rhs_field(a->get_lhs_field());
    }
    if (a->get_reduce_type() == GMREDUCE_MIN) {
        Body.push(">");
    } else {
        Body.push("<");
    }
    Body.push(rhs_temp);
    Body.pushln(") {");

    sk_lhs = true;

    // lhs = rhs_temp
    if (a->is_target_scalar()) {
        generate_lhs_id(a->get_lhs_scala());
    } else {
        generate_lhs_field(a->get_lhs_field());
    }

    //    Body.push(" = ");
    sk_lhs = false;

    Body.push(rhs_temp);

    sk_rhs_end(&Body);
    Body.pushln(";");

    i = 0;
    for (I = L.begin(); I != L.end(); I++, i++) {
        sk_lhs = true;
        ast_node* n = *I;
        if (n->get_nodetype() == AST_ID) {
            generate_lhs_id((ast_id*) n);
        } else {
            generate_lhs_field((ast_field*) n);
        }
        sk_lhs = false;
        //        Body.push(" = ");
        Body.push(names[i]);
        sk_rhs_end(&Body);
        Body.pushln(";");
    }

    Body.pushln("}"); // end of inner if

    Body.push(UNLOCK_FN_NAME);
    Body.push("(");
    if (a->is_target_scalar()) {
        Body.push("&");
        generate_rhs_id(a->get_lhs_scala());
    } else {
        generate_rhs_id(a->get_lhs_field()->get_first());
    }
    Body.pushln(");");

    Body.pushln("}"); // end of outer if

    Body.pushln("}"); // end of reduction
    // clean-up
    for (i = 0; i < (int) L.size(); i++)
        delete[] names[i];
    delete[] names;
    delete[] rhs_temp;
}

void gm_cpp_gen::generate_sent_return(ast_return *r) {
    if (FE.get_current_proc()->find_info_bool(CPPBE_INFO_HAS_PROPDECL)) {
        Body.push(CLEANUP_PTR);
        Body.pushln("();");
    }

    Body.pushln("shl__end();\n");

    Body.push("return");
    if (r->get_expr() != NULL) {
        Body.SPC();
        generate_expr(r->get_expr());
    }
    Body.pushln("; ");
}

void gm_cpp_gen::generate_sent_nop(ast_nop* n) {
    switch (n->get_subtype()) {
        case NOP_REDUCE_SCALAR:
            ((nop_reduce_scalar*) n)->generate(this);
            break;
            /* otherwise ask library to hande it */
        default: {
            get_lib()->generate_sent_nop(n);
            break;
        }
    }

    return;
}

void gm_cpp_gen::generate_expr_inf(ast_expr *e) {
    char* temp = temp_str;
    assert(e->get_opclass() == GMEXPR_INF);
    int t = e->get_type_summary();
    switch (t) {
        case GMTYPE_INF:
        case GMTYPE_INF_INT:
            sprintf(temp, "%s", e->is_plus_inf() ? "INT_MAX" : "INT_MIN");
            break;
        case GMTYPE_INF_LONG:
            sprintf(temp, "%s", e->is_plus_inf() ? "LLONG_MAX" : "LLONG_MIN");
            break;
        case GMTYPE_INF_FLOAT:
            sprintf(temp, "%s", e->is_plus_inf() ? "FLT_MAX" : "FLT_MIN");
            break;
        case GMTYPE_INF_DOUBLE:
            sprintf(temp, "%s", e->is_plus_inf() ? "DBL_MAX" : "DBL_MIN");
            break;
        default:
            printf("what type is it? %d", t);
            assert(false);
            sprintf(temp, "%s", e->is_plus_inf() ? "INT_MAX" : "INT_MIN");
            break;
    }
    Body.push(temp);
    return;
}

void gm_cpp_gen::generate_expr_abs(ast_expr *e) {
    Body.push(" std::abs(");
    generate_expr(e->get_left_op());
    Body.push(") ");
}
void gm_cpp_gen::generate_expr_minmax(ast_expr *e) {
    if (e->get_optype() == GMOP_MIN) {
        Body.push(" std::min(");
    } else {
        Body.push(" std::max(");
    }
    generate_expr(e->get_left_op());
    Body.push(",");
    generate_expr(e->get_right_op());
    Body.push(") ");
}

void gm_cpp_gen::generate_expr_nil(ast_expr* ee) {
    get_lib()->generate_expr_nil(ee, Body);
}

const char* gm_cpp_gen::get_function_name(int methodId, bool& addThreadId) {
    switch (methodId) {
        case GM_BLTIN_TOP_DRAND:
            addThreadId = true;
            return "gm_rt_uniform";
        case GM_BLTIN_TOP_IRAND:
            addThreadId = true;
            return "gm_rt_rand";
        case GM_BLTIN_TOP_LOG:
            return "log";
        case GM_BLTIN_TOP_EXP:
            return "exp";
        case GM_BLTIN_TOP_POW:
            return "pow";
        default:
            assert(false);
            return "ERROR";
    }
}

void gm_cpp_gen::generate_expr_builtin(ast_expr* ee) {
    ast_expr_builtin* e = (ast_expr_builtin*) ee;

    gm_builtin_def* def = e->get_builtin_def();

    ast_id* driver;
    if (e->driver_is_field())
        driver = ((ast_expr_builtin_field*) e)->get_field_driver()->get_second();
    else
        driver = e->get_driver();

    assert(def != NULL);
    int method_id = def->get_method_id();
    if (driver == NULL) {
        bool add_thread_id = false;
        const char* func_name = get_function_name(method_id, add_thread_id);
        Body.push(func_name);
        Body.push('(');
        generate_expr_list(e->get_args());
        if (add_thread_id) {
            if (e->get_args().size() > 0)
                Body.push(",gm_rt_thread_id()");
            else
                Body.push("gm_rt_thread_id()");
        }
        Body.push(")");
    } else {
        get_lib()->generate_expr_builtin((ast_expr_builtin*) e, Body);
    }
}

bool gm_cpp_gen::prepare_parallel_for(bool need_dynamic) {
    bool res = false;

    if (is_under_parallel_sentblock()) {
        Body.pushln("#ifdef SHL_STATIC");
        Body.pushln("#pragma omp for nowait schedule(static,1024)");
        Body.pushln("#else");
        Body.push("#pragma omp for nowait"); // already under parallel region.
        if (need_dynamic) {
            Body.push(" schedule(dynamic,128)");

        }
        Body.NL();
        Body.pushln("#endif");
    }
    else {
        Body.push("#pragma omp parallel ");

        Body.NL();
        res = true;
        Body.pushln("{");
        sk_init_accessors(&Body);
        Body.pushln("#ifdef SHL_STATIC");
        Body.pushln("#pragma omp for schedule(static,1024)");
        Body.pushln("#else");
        Body.push("#pragma omp for");
        if (need_dynamic) {
            Body.push(" schedule(dynamic,128)");

        }
        Body.NL();
        Body.pushln("#endif");
    }

    return res;
}
