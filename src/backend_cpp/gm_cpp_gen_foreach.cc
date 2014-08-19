#include <stdio.h>
#include "gm_backend_cpp.h"
#include "gm_frontend_api.h"
#include "gm_typecheck.h"
#include "gm_error.h"
#include "gm_traverse.h"
#include "gm_misc.h"
#include "gm_typecheck.h"
#include "gm_ast.h"
#include "gm_transform_helper.h"
#include "gm_builtin.h"
#include "gm_cpplib_words.h"

//--------------------------
// Foreach(...)
//    S;
// ==>
// up_initializer;
// for(.....) {
//   down_initializer;
//   S;
// }
//--------------------------

#include <set>
using namespace std;

set<string> its;

bool gm_cpplib::need_up_initializer(ast_foreach* f) {
    int iter_type = f->get_iter_type();
    if (gm_is_simple_collection_iteration(iter_type) || gm_is_collection_of_collection_iteration(iter_type))
        return true;
    else if (gm_is_common_nbr_iteration(iter_type))
        return true;
    return false;
}

bool gm_cpplib::need_down_initializer(ast_foreach* f) {
    int iter_type = f->get_iter_type();

    if (gm_is_simple_collection_iteration(iter_type) || gm_is_collection_of_collection_iteration(iter_type)) {
        return true;
    } else if (gm_is_common_nbr_iteration(iter_type)) {
        return false;
    }
    // in/out/up/down
    else if (gm_is_any_neighbor_node_iteration(iter_type)) {
        return true;
    }

    return false;
}

void gm_cpplib::generate_up_initializer(ast_foreach* f, gm_code_writer& Body) {
    int iter_type = f->get_iter_type();
    if (gm_is_simple_collection_iteration(iter_type) || gm_is_collection_of_collection_iteration(iter_type)) {
        // for temp
        assert(!f->is_parallel());

        const char* iter_type_str = f->is_parallel() ? "par_iter" : f->is_reverse_iteration() ? "rev_iter" : "seq_iter";
        const char* prep_str = f->is_parallel() ? "prepare_par_iteration" : f->is_reverse_iteration() ? "prepare_rev_iteration" : "prepare_seq_iteration";
        const char* source_type_str;
        int iter_source_type;
        if (f->is_source_field()) {
            source_type_str = get_type_string(f->get_source_field()->get_second()->getTargetTypeInfo());
            iter_source_type = f->get_source_field()->get_second()->getTypeInfo()->getTargetTypeSummary();
        } else {
            gm_symtab_entry* sym = f->get_source()->getSymInfo();
            if(has_optimized_type_name(sym)) {
                source_type_str = get_optimized_type_name(sym);
            } else {
                source_type_str = get_type_string(f->get_source()->getTypeInfo());
            }

            iter_source_type = f->get_source()->getTypeInfo()->getTypeSummary();
        }

        // get a list
        const char* typeString = NULL;
        if (gm_is_collection_of_collection_type(iter_source_type)) {
            ast_id* source = f->get_source();
            sprintf(str_buf, "%s<%s>::%s", get_type_string(source->getTypeInfo()), get_type_string(source->getTargetTypeInfo()), iter_type_str);
        } else {
            sprintf(str_buf, "%s::%s", source_type_str, iter_type_str);
        }
        Body.push(str_buf);

        const char* a_name = FE.voca_temp_name_and_add(f->get_iterator()->get_orgname(), "_I");
        f->add_info_string(CPPBE_INFO_COLLECTION_ITERATOR, a_name);
        sprintf(str_buf, " %s", a_name);
        Body.push(str_buf);
        delete[] a_name;

        if (f->is_source_field()) {
            Body.push(" = ");
            get_main()->generate_rhs_field(f->get_source_field());
            sprintf(str_buf, ".%s();", prep_str);
            Body.pushln(str_buf);
        } else {
            sprintf(str_buf, " = %s.%s();", f->get_source()->get_genname(), prep_str);
            Body.pushln(str_buf);
        }
    } else if (gm_is_common_nbr_iteration(iter_type)) {
        ast_id* source = f->get_source();
        ast_id* graph = source->getTypeInfo()->get_target_graph_id();
        ast_id* source2 = f->get_source2();
        assert(source2!=NULL);
        const char* a_name = FE.voca_temp_name_and_add(f->get_iterator()->get_orgname(), "_I");
        f->add_info_string(CPPBE_INFO_COMMON_NBR_ITERATOR, a_name);
        Body.pushln("// Iterate over Common neighbors");
        sprintf(str_buf, "gm_common_neighbor_iter %s(%s, %s, %s);", a_name, graph->get_genname(), source->get_genname(), source2->get_genname());
        Body.pushln(str_buf);
    }
    else {
        assert(false);
    }
}

void gm_cpplib::generate_down_initializer(ast_foreach* f, gm_code_writer& Body) {
    int iter_type = f->get_iter_type();
    ast_id* iter = f->get_iterator();
    ast_typedecl* source_type;
    if (f->is_source_field()) {
        source_type = f->get_source_field()->get_second()->getTypeInfo();
    } else {
        source_type = f->get_source()->getTypeInfo();
    }

    if (gm_is_simple_collection_iteration(iter_type) || gm_is_collection_of_collection_iteration(iter_type)) {
        assert(f->find_info(CPPBE_INFO_COLLECTION_ITERATOR) != NULL);
        const char* lst_iter_name = f->find_info_string(CPPBE_INFO_COLLECTION_ITERATOR);
        const char* type_name;
        if (gm_is_collection_of_collection_type(source_type->getTypeSummary()))
            type_name = get_type_string(f->get_source()->getTargetTypeInfo());
        else
            type_name = source_type->is_node_collection() ? NODE_T : EDGE_T;

        if (gm_is_collection_of_collection_iteration(iter_type)) {
            sprintf(str_buf, "%s& %s = %s.get_next();", type_name, f->get_iterator()->get_genname(), lst_iter_name);
        } else {
            sprintf(str_buf, "%s %s = %s.get_next();", type_name, f->get_iterator()->get_genname(), lst_iter_name);
        }
        Body.pushln(str_buf);
    } else if (gm_is_any_neighbor_node_iteration(iter_type)) {
        ast_id* source = f->get_source();
        const char* alias_name = f->find_info_string(CPPBE_INFO_NEIGHBOR_ITERATOR);
        const char* type_name = get_type_string(iter->getTypeInfo());
        const char* graph_name = source->getTypeInfo()->get_target_graph_id()->get_genname();
        const char* var_name = iter->get_genname();
        const char* array_name;

        // should be neighbor iterator
        assert(gm_is_node_iteration(iter_type));

        // [XXX] should be changed if G is transposed!
        array_name = gm_is_iteration_use_reverse(iter_type) ? R_NODE_IDX : NODE_IDX;

        if (gm_is_down_nbr_node_iteration(iter_type)) {
            sprintf(str_buf, "if (!is_down_edge(%s)) continue;", alias_name);
            Body.pushln(str_buf);

            sprintf(str_buf, "%s %s = /*5*/ %s.%s [%s];", type_name, var_name, graph_name, array_name, alias_name);
            Body.pushln(str_buf);
        } else if (gm_is_updown_node_iteration(iter_type)) {
            sprintf(str_buf, "%s %s = /*4*/ %s.%s [%s];", type_name, var_name, graph_name, array_name, alias_name);
            Body.pushln(str_buf);

            sprintf(str_buf, "if (get_level(%s) != (get_curr_level() %c 1)) continue;", iter->get_genname(),
                    gm_is_up_nbr_node_iteration(iter_type) ? '-' : '+');
            Body.pushln(str_buf);
        } else {

#ifndef SHOAL_ACTIVATE
            sprintf(str_buf, "%s %s = %s.%s [%s];", type_name, var_name, graph_name, array_name, alias_name);
            Body.pushln(str_buf);
#else
            sprintf(str_buf, "%s %s = ", type_name, var_name);
            Body.push(str_buf);

            sk_m_array_access(&Body, array_name, alias_name,
                              (std::string(graph_name) + "." + array_name));
#endif
            Body.pushln(";");
        }
    }
}

void gm_cpplib::generate_foreach_header(ast_foreach* fe, gm_code_writer& Body) {
    //ast_id* source = fe->get_source();
    ast_id* iter = fe->get_iterator();
    int type = fe->get_iter_type();

    if (gm_is_all_graph_iteration(type)) {
        assert(!fe->is_source_field());
        ast_id* source = fe->get_source();
        char* graph_name;
        if (gm_is_node_property_type(source->getTypeSummary()) || gm_is_edge_property_type(source->getTypeSummary())) {
            graph_name = source->getTypeInfo()->get_target_graph_id()->get_orgname();
        } else {
            graph_name = source->get_genname();
        }
        char* it_name = iter->get_genname();
        assert(its.insert(string(it_name)).second); // otherwise, the index is already there, which is an error

        sprintf(str_buf, "for (%s %s = 0; %s < %s.%s(); %s ++) /*7 => N*/ ",
                get_type_string(iter->getTypeSummary()),
                it_name, it_name, graph_name,
                gm_is_node_iteration(type) ? NUM_NODES : NUM_EDGES, it_name);

#ifdef SHOAL_ACTIVATE
        sk_iterator(&Body, iter->get_genname(), get_type_string(iter->getTypeSummary()));

        Body.pushln(str_buf);

        const char *sk_g_name = NULL;
        const char *sk_array_name = NULL;

        // Extract type information
        // This will always be the graph G
        if (gm_is_node_property_type(source->getTypeSummary())) {
            sk_g_name = "NODE";
        }
        else if (gm_is_edge_property_type(source->getTypeSummary())) {
            sk_g_name = "EDGE";
        } else {
            sk_g_name = source->get_genname();
        }

        if (gm_is_node_iteration(type)) {
            sk_array_name = "node";
        } else {
            sk_array_name = "edge";
        }

        sk_forall(&Body, sk_g_name, sk_array_name);
#endif

    } else if (gm_is_common_nbr_iteration(type)) {
        assert(!fe->is_source_field());
        ast_id* source = fe->get_source();
        const char* iter_name = fe->find_info_string(CPPBE_INFO_COMMON_NBR_ITERATOR);
        char* graph_name = source->get_genname();
        char* it_name = iter->get_genname();
        sprintf(str_buf, "for (%s %s = %s.get_next(); %s != gm_graph::NIL_NODE ; %s = %s.get_next()) ",
                NODE_T, it_name, iter_name, it_name, it_name,
                iter_name);

        Body.pushln(str_buf);

        // NBRS, UP_NBRS, DOWN_NBRS, ...
    } else if (gm_is_any_neighbor_node_iteration(type)) {

        assert(gm_is_node_iteration(type));
        assert(!fe->is_source_field());
        ast_id* source = fe->get_source();

        //-----------------------------------------------
        // create additional information
        //-----------------------------------------------
        const char* a_name = FE.voca_temp_name_and_add(iter->get_orgname(), "_idx");
        fe->add_info_string(CPPBE_INFO_NEIGHBOR_ITERATOR, a_name);
        ast_id* iterator = fe->get_iterator();
        iterator->getSymInfo()->add_info_string(CPPBE_INFO_NEIGHBOR_ITERATOR, a_name);
        delete[] a_name;

        // [todo] check name-conflict
        const char* alias_name = fe->find_info_string(CPPBE_INFO_NEIGHBOR_ITERATOR);
        const char* graph_name = source->getTypeInfo()->get_target_graph_id()->get_genname();
        const char* array_name = gm_is_iteration_use_reverse(type) ? R_BEGIN : BEGIN;
        const char* src_name = source->get_genname();

        sk_log(&Body, "found loop, but not over array");

#ifndef SHOAL_ACTIVATE
        sprintf(str_buf, "for (%s %s = %s.%s[%s];", EDGE_T, alias_name, graph_name, array_name, src_name);
        Body.push(str_buf);
        sprintf(str_buf, "%s < %s.%s[%s+1] ; %s ++) ", alias_name, graph_name, array_name, src_name, alias_name);
        Body.pushln(str_buf);
#else
        assert (its.insert(alias_name).second);
        sprintf(str_buf, "for (%s %s = ", EDGE_T, alias_name);
        Body.push(str_buf);

        sk_m_array_access(&Body, array_name, src_name, (std::string(graph_name) + "." + array_name));

        sprintf(str_buf, ";%s < ", alias_name);
        Body.push(str_buf);

        sprintf(str_buf, "%s+1", src_name);
        sk_m_array_access(&Body, array_name, str_buf, (std::string(graph_name) + "." + array_name));

        sprintf(str_buf, "; %s ++) /*8 => neighbor node*/ ", alias_name);
        Body.push(str_buf);
#endif
        // SET_TYPE
    } else if (gm_is_simple_collection_iteration(type) || gm_is_collection_of_collection_iteration(type)) {

        assert(!fe->is_parallel());
        assert(gm_is_node_collection_iteration(type) || gm_is_collection_of_collection_iteration(type));

        const char* iter_name = fe->find_info_string(CPPBE_INFO_COLLECTION_ITERATOR);
        sprintf(str_buf, "while (%s.has_next())", iter_name);
        Body.pushln(str_buf);

    } else {
        assert(false);
    }

    return;
}
