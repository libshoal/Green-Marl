#include "gm_edge_list_graph_reader.h"

#include <string>

#include "gm_graph.h"

bool gm_graph::load_edge_list(char* filename,    // input filename
        std::vector<VALUE_TYPE>& vprop_schema,   // input: type of node properties
        std::vector<VALUE_TYPE>& eprop_schema,   // input: type of edge properties
        std::vector<void*>& vertex_props,        // output, vector of arrays
        std::vector<void*>& edge_props,          // output, vector of arrays,
        bool use_hdfs) {

    assert(!use_hdfs);

    gm_edge_list_graph_reader reader(filename, vprop_schema, eprop_schema, vertex_props, edge_props, *this);
    return reader.loadEdgeList();
}

bool gm_graph::store_edge_list(char* filename,  // output filename
        std::vector<VALUE_TYPE>& vprop_schema,  // input: type of node properties
        std::vector<VALUE_TYPE>& eprop_schema,  // input: type of edge properties
        std::vector<void*>& vertex_props,       // input, vector of arrays
        std::vector<void*>& edge_props,         // intput, vector of arrays,
        bool use_hdfs) {

    assert(!use_hdfs);

    gm_edge_list_graph_writer reader(filename, vprop_schema, eprop_schema, vertex_props, edge_props, *this);
    return reader.storeEdgeList();
}

gm_edge_list_graph_reader::gm_edge_list_graph_reader(char* filename, std::vector<VALUE_TYPE>& vprop_schema, std::vector<VALUE_TYPE>& eprop_schema,
        std::vector<void*>& vertex_props, std::vector<void*>& edge_props, gm_graph& Graph, bool hdfs) :
        G(Graph), fileName(filename), useHDFS(hdfs), nodePropertySchemata(vprop_schema), edgePropertySchemata(eprop_schema), nodeProperties(vertex_props), edgeProperties(
                edge_props), currentLine(0) {

    nodePropertyCount = nodePropertySchemata.size();
    edgePropertyCount = edgePropertySchemata.size();
}

gm_edge_list_graph_reader::~gm_edge_list_graph_reader() {
    if (inputFileStream.is_open()) inputFileStream.close();
}

// http://stackoverflow.com/questions/3437404/min-and-max-in-c/3437484#3437484
#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; })

/*
 * For the twitter graph, we need to "normalize" the user IDs. The
 * resulting graph should not contain users who's profile has been
 * deleted.
 *
 * From http://twitter.mpi-sws.org/
 *
 * These accounts were in use in August 2009. We obtained the list of
 * user IDs by repeatedly checking all possible IDs from 0 to 80
 * million. We scanned the list twice at a two week time gap. We did
 * not look beyond 80 million, because no single user in the collected
 * data had a link to a user whose ID was greater than that value.
 *
 * i.e. the biggest ID (for the the input data should be 80'000'000
 */
#define LOOKUP_SIZE 80000000
#define NUM_LINES 1468365182
void gm_edge_list_graph_reader::builtGraph() {

    // Need temporary data structure for mapping USERID -> NODEID
    printf("Stefan: Initializing lookup array .. ");
    int32_t *lookup = (int32_t*) malloc(sizeof(int32_t)*LOOKUP_SIZE);
    assert (lookup!=NULL);
    for (int i=0; i<LOOKUP_SIZE; i++)
        lookup[i] = -1;
    node_t skmax = 0;
    printf("done\n");

    inputFileStream.open(fileName);
    if (!inputFileStream.is_open()) {
        printf("Error: Error while opening file.\n");
        assert(false);
    }

    int maxSize = 1024;
    char lineData[maxSize]; // should be enough right?
    //node_t maxNodeId = -1;
    int sklines = 0;

    // Let's assume that the input graph is sorted
    node_t curr_src = -1;

    // Step 1: determine which node IDs are present
    while (!inputFileStream.eof()) {
        currentLine++; sklines++;
        if (sklines%1000000 == 0) printf("current line is % 15d - % 3.2f\n",
                                         sklines, (float)sklines/(float)NUM_LINES);
        inputFileStream.getline(lineData, maxSize);
        if (strlen(lineData) == 0 || lineData[0] == '#') continue;

        char* p = strtok(lineData, " \t");
        node_t nodeId = readValueFromToken<node_t>(p);
        // mark source node as being used
        lookup[nodeId] = 1;
        p = strtok(NULL, " \t\n\r");
        if (*p != '*') {
            node_t destination = readValueFromToken<node_t>(p);
            // mark destination as being used
            lookup[destination] = 1;
        }

    }

    // Build the lookup table
    int sk_node_ctr = 0;
    for (int i=0; i<LOOKUP_SIZE; i++) {
        lookup[i] = lookup[i] > 0 ? sk_node_ctr++ : -1;
    }
    printf("Number of nodes found: %d\n", sk_node_ctr);

    // Add nodes to graph
    for (int i=0; i<sk_node_ctr; i++) {
            G.add_node();
    }

    // Close and reopen file
    inputFileStream.close();
    inputFileStream.open(fileName);
    if (!inputFileStream.is_open()) {
        printf("Error: Error while opening file.\n");
        assert(false);
    }

    currentLine = 0;

    while (!inputFileStream.eof()) {
        currentLine++; sklines--;
        inputFileStream.getline(lineData, maxSize);
        if (strlen(lineData) == 0 || lineData[0] == '#') continue;

        char* p = strtok(lineData, " \t");
        node_t nodeId = readValueFromToken<node_t>(p);
        // Make sure input is sorted!
        assert(nodeId >= curr_src); curr_src = max(nodeId, curr_src);
        assert(nodeId<LOOKUP_SIZE);
        if (sklines%1000000 == 0) printf("current line is % 15d - % 15d\n",
                                       sklines, skmax);
        assert(lookup[nodeId]>=0);
        nodeId = lookup[nodeId];
        p = strtok(NULL, " \t\n\r");
        if (*p != '*') {
            node_t destination = readValueFromToken<node_t>(p);
            assert (destination<LOOKUP_SIZE);
            assert (lookup[destination]>=0);
            destination = lookup[destination];
            G.add_edge(nodeId, destination);
        }

    }

    printf("maxNodeId: %d\n", skmax);

    inputFileStream.close();
    G.freeze();

    //assert(false); // okay this doen't work. nodeId != nodeIdx
    G.do_semi_sort();
    G.make_reverse_edges();
}

bool gm_edge_list_graph_reader::loadEdgeList() {

    G.clear_graph();
    builtGraph();
    if ((nodePropertyCount == 0) && (edgePropertyCount == 0))
        return true;


    inputFileStream.open(fileName);
    if (!inputFileStream.is_open()) {
        printf("Error: Error while opening file.\n");
        return false;
    }

    currentLine = 1;

    int maxSize = 1024;
    char lineData[maxSize]; // should be enough right?

    createNodeProperties();
    createEdgeProperties();

    while (!inputFileStream.eof()) {
        currentLine++;
        inputFileStream.getline(lineData, maxSize);
        if (strlen(lineData) == 0 || lineData[0] == '#') continue;

        char* p = strtok(lineData, " \t");
        node_t nodeId = readValueFromToken<node_t>(p);
        p = strtok(NULL, " \t");
        if (*p == '*') {
            if (!handleNode(nodeId, p)) return false;
        } else {
            if (!handleEdge(nodeId, p)) return false;
        }
    }

    return true;
}

bool gm_edge_list_graph_reader::handleNode(node_t nodeId, char* p) {
    p = strtok(NULL, " ");
    for (int i = 0; i < nodePropertyCount; i++) {
        if (p == NULL || strlen(p) == 0) {
            raiseNodePropertyMissing(nodePropertySchemata[i]);
        }
        addNodePropertyValue(nodeId, i, p);
        p = strtok(NULL, " ");
    }
    return true;
}

bool gm_edge_list_graph_reader::handleEdge(node_t sourceNode, char* p) {
    node_t targetNode = readValueFromToken<node_t>(p);

    edge_t edgeId = -1;
    for (edge_t edge = G.begin[sourceNode]; edge < G.begin[sourceNode + 1]; edge++) {
        node_t currentTarget = G.node_idx[edge];
        if (currentTarget == targetNode) {
            edgeId = edge;
            break;
        }
    }

    p = strtok(NULL, " ");
    for (int i = 0; i < edgePropertyCount; i++) {
        if (p == NULL || strlen(p) == 0) {
            raiseEdgePropertyMissing(edgePropertySchemata[i]);
        }
        addEdgePropertyValue(edgeId, i, p);
        p = strtok(NULL, " ");
    }
    return true;
}

void gm_edge_list_graph_reader::addNodePropertyValue(node_t nodeId, int propertyId, const char* p) {
    assert(p != NULL);
    VALUE_TYPE type = nodePropertySchemata[propertyId];
    void* property = nodeProperties[propertyId];
    addPropertyValue<node_t>(property, nodeId, type, p);
}

void gm_edge_list_graph_reader::addEdgePropertyValue(edge_t edgeId, int propertyId, const char* p) {
    assert(p != NULL);
    VALUE_TYPE type = edgePropertySchemata[propertyId];
    void* property = edgeProperties[propertyId];
    addPropertyValue<edge_t>(property, edgeId, type, p);
}

void gm_edge_list_graph_reader::createNodeProperties() {
    node_t size = G.num_nodes();

    for (int i = 0; i < nodePropertyCount; i++) {
        VALUE_TYPE type = nodePropertySchemata[i];
        void* property = createProperty(size, i, type);
        nodeProperties.push_back(property);
    }
}

void gm_edge_list_graph_reader::createEdgeProperties() {
    edge_t size = G.num_edges();

    for (int i = 0; i < edgePropertyCount; i++) {
        VALUE_TYPE type = edgePropertySchemata[i];
        void* property = createProperty(size, i, type);
        edgeProperties.push_back(property);
    }
}

void* gm_edge_list_graph_reader::createProperty(int size, int position, VALUE_TYPE type) {
    switch (type) {
        case GMTYPE_BOOL:
            return allocateProperty<bool>(size, position);
        case GMTYPE_INT:
            return allocateProperty<int>(size, position);
        case GMTYPE_LONG:
            return allocateProperty<long>(size, position);
        case GMTYPE_FLOAT:
            return allocateProperty<float>(size, position);
        case GMTYPE_DOUBLE:
            return allocateProperty<double>(size, position);
        case GMTYPE_NODE:
            return allocateProperty<node_t>(size, position);
        case GMTYPE_EDGE:
            return allocateProperty<edge_t>(size, position);
        default:
            assert(false);
            // should never happen
            return NULL;
    }
}

gm_edge_list_graph_writer::gm_edge_list_graph_writer(char* filename, //
        std::vector<VALUE_TYPE>& vprop_schema, //
        std::vector<VALUE_TYPE>& eprop_schema, //
        std::vector<void*>& vertex_props, //
        std::vector<void*>& edge_props, //
        gm_graph& Graph, //
        bool hdfs) :
        G(Graph), //
        writer(filename, hdfs), //
        nodePropertySchemata(vprop_schema), //
        edgePropertySchemata(eprop_schema), //
        nodeProperties(vertex_props), //
        edgeProperties(edge_props) {

    if (writer.failed()) {
        fprintf(stderr, "Could not open %s for writing\n", filename);
        assert(false);
    }

    nodePropertyCount = nodePropertySchemata.size();
    edgePropertyCount = edgePropertySchemata.size();
}

bool gm_edge_list_graph_writer::storeEdgeList() {

    // store nodes and node properties
    for (node_t node = 0; node < G.num_nodes(); node++) {
        storePropertiesForNode(node);
    }

    // store edges and edge properties
    for (node_t source = 0; source < G.num_nodes(); source++) {
        for (edge_t edge = G.begin[source]; edge < G.begin[source + 1]; edge++) {
            storePropertiesForEdge(source, edge);
        }
    }

    return true;
}

void gm_edge_list_graph_writer::storePropertiesForNode(node_t node) {
    writer.write(node);
    writer.write(" * ");

    int propertyCount = nodePropertySchemata.size();
    for (int i = 0; i < propertyCount; i++) {
        void* property = nodeProperties[i];
        VALUE_TYPE type = nodePropertySchemata[i];
        appendProperty(node, property, type);
        if (i < propertyCount - 1) writer.write(" ");
    }
    writer.write("\n");
}

void gm_edge_list_graph_writer::appendProperty(node_t node, void* property, VALUE_TYPE type) {
    switch (type) {
        case GMTYPE_BOOL:
            appendProperty<bool>(node, property);
            break;
        case GMTYPE_INT:
            appendProperty<int>(node, property);
            break;
        case GMTYPE_LONG:
            appendProperty<long>(node, property);
            break;
        case GMTYPE_FLOAT:
            appendProperty<float>(node, property);
            break;
        case GMTYPE_DOUBLE:
            appendProperty<double>(node, property);
            break;
        case GMTYPE_NODE:
            appendProperty<node_t>(node, property);
            break;
        case GMTYPE_EDGE:
            appendProperty<edge_t>(node, property);
            break;
        default:
            assert(false);
            // should never happen
            break;
    }
}

void gm_edge_list_graph_writer::storePropertiesForEdge(node_t source, edge_t edge) {

    node_t target = G.node_idx[edge];

    writer.write(source);
    writer.write(" ");
    writer.write(target);
    writer.write(" ");

    int propertyCount = edgePropertySchemata.size();
    for (int i = 0; i < propertyCount; i++) {
        void* property = edgeProperties[i];
        VALUE_TYPE type = edgePropertySchemata[i];
        switch (type) {
            appendProperty<bool>(edge, property);
            break;
        case GMTYPE_INT:
            appendProperty<int>(edge, property);
            break;
        case GMTYPE_LONG:
            appendProperty<long>(edge, property);
            break;
        case GMTYPE_FLOAT:
            appendProperty<float>(edge, property);
            break;
        case GMTYPE_DOUBLE:
            appendProperty<double>(edge, property);
            break;
        case GMTYPE_NODE:
            appendProperty<node_t>(edge, property);
            break;
        case GMTYPE_EDGE:
            appendProperty<edge_t>(edge, property);
            break;
        default:
            assert(false);
            // should never happen
            break;
        }
        if (i < propertyCount - 1) writer.write(" ");
    }
    writer.write("\n");
}
