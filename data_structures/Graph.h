// Original code by Gonçalo Leão
// Updated by DA 2024/2025 Team

#ifndef DA_TP_CLASSES_GRAPH
#define DA_TP_CLASSES_GRAPH

#include <iostream>
#include <vector>
#include <queue>
#include <limits>
#include <algorithm>

template <class T>
class Edge;

#define INF std::numeric_limits<double>::max()

/************************* Vertex  **************************/

/**
 * @class Vertex
 * @brief Represents a node in the graph.
 * @tparam T The data type of the information stored in the vertex.
 */
template <class T>
class Vertex
{
public:
    Vertex(T in);
    ~Vertex();

    T getInfo() const;
    std::vector<Edge<T> *> getAdj() const;
    bool isVisited() const;
    bool isProcessing() const;
    unsigned int getIndegree() const;
    double getDist() const;
    Edge<T> *getPath() const;
    std::vector<Edge<T> *> getIncoming() const;

    void setInfo(T info);
    void setVisited(bool visited);
    void setProcessing(bool processing);

    int getLow() const;
    void setLow(int value);
    int getNum() const;
    void setNum(int value);

    void setIndegree(unsigned int indegree);
    void setDist(double dist);
    void setPath(Edge<T> *path);
    /**
     * @brief Adds an outgoing edge from this vertex to the destination.
     * @param dest The destination vertex.
     * @param w The weight of the edge.
     * @return Pointer to the newly created edge.
     * @complexity O(1) amortized.
     */
    Edge<T> *addEdge(Vertex<T> *dest, double w);
    /**
     * @brief Removes all outgoing edges to a vertex with the given content.
     * @param in The content of the destination vertex.
     * @return True if at least one edge was removed.
     * @complexity O(E_v) where E_v is the number of outgoing edges from this vertex.
     */
    bool removeEdge(T in);
    /**
     * @brief Removes all outgoing edges from this vertex.
     * @complexity O(E_v) where E_v is the number of outgoing edges.
     */
    void removeOutgoingEdges();

protected:
    T info;
    std::vector<Edge<T> *> adj;

    bool visited = false;
    bool processing = false;
    int low = -1, num = -1;
    unsigned int indegree;
    double dist = 0;
    Edge<T> *path = nullptr;

    std::vector<Edge<T> *> incoming;

    void deleteEdge(Edge<T> *edge);
};

/********************** Edge  ****************************/

/**
 * @class Edge
 * @brief Represents a directed connection between two vertices in the graph.
 * @tparam T The data type of the information stored in the connected vertices.
 */
template <class T>
class Edge
{
public:
    Edge(Vertex<T> *orig, Vertex<T> *dest, double w);

    Vertex<T> *getDest() const;
    double getWeight() const;
    bool isSelected() const;
    Vertex<T> *getOrig() const;
    Edge<T> *getReverse() const;
    double getFlow() const;

    void setSelected(bool selected);
    void setReverse(Edge<T> *reverse);
    void setFlow(double flow);

protected:
    Vertex<T> *dest;
    double weight;

    bool selected = false;

    Vertex<T> *orig;
    Edge<T> *reverse = nullptr;

    double flow = 0;
};

/********************** Graph  ****************************/

/**
 * @class Graph
 * @brief Represents a general graph using an adjacency list.
 * @tparam T The data type of the information stored in the graph's vertices.
 */
template <class T>
class Graph
{
public:
    Graph() = default;
    ~Graph();

    // Disable copy
    Graph(const Graph &) = delete;
    Graph &operator=(const Graph &) = delete;

    // Enable move
    Graph(Graph &&other) noexcept;
    Graph &operator=(Graph &&other) noexcept;

    /**
     * @brief Finds a vertex with a given content.
     * @param in The content to search for.
     * @return Pointer to the vertex, or nullptr if not found.
     * @complexity O(V) linear search over the vertex set.  This is the bottleneck in
     *             graph construction, where addEdge calls findVertex for every endpoint.
     *             A possible optimisation is to replace @c std::vector<Vertex<T>*> with
     *             @c std::unordered_map<T, Vertex<T>*>, reducing findVertex to O(1) and
     *             graph building to O(V + E).
     */
    Vertex<T> *findVertex(const T &in) const;
    /**
     * @brief Adds a vertex with a given content to the graph.
     * @param in The content for the new vertex.
     * @return True if successful, false if a vertex with that content already exists.
     * @complexity O(V) due to the duplicate check via findVertex.
     */
    bool addVertex(const T &in);
    /**
     * @brief Removes a vertex and all its incident edges from the graph.
     * @param in The content of the vertex to remove.
     * @return True if successful, false if the vertex does not exist.
     * @complexity O(V + E) where V is the number of vertices and E the number of edges
     *             incident to the removed vertex.
     */
    bool removeVertex(const T &in);

    /**
     * @brief Adds a directed edge to the graph.
     * @param sourc The content of the source vertex.
     * @param dest The content of the destination vertex.
     * @param w The weight (capacity) of the edge.
     * @return True if successful, false if either vertex does not exist.
     * @complexity O(V) due to findVertex lookups for both endpoints.
     */
    bool addEdge(const T &sourc, const T &dest, double w);
    /**
     * @brief Removes a directed edge from the graph.
     * @param source The content of the source vertex.
     * @param dest The content of the destination vertex.
     * @return True if the edge was removed, false if it does not exist.
     * @complexity O(V + E_v) due to findVertex and the vertex-level removeEdge.
     */
    bool removeEdge(const T &source, const T &dest);
    /**
     * @brief Adds a bidirectional (undirected) edge to the graph.
     * @param sourc The content of the first vertex.
     * @param dest The content of the second vertex.
     * @param w The weight of the edge in both directions.
     * @return True if successful, false if either vertex does not exist.
     * @complexity O(V) due to findVertex lookups for both endpoints.
     */
    bool addBidirectionalEdge(const T &sourc, const T &dest, double w);

    /**
     * @brief Returns the number of vertices in the graph.
     * @return The vertex count.
     * @complexity O(1)
     */
    int getNumVertex() const;
    /**
     * @brief Returns a copy of the vertex set.
     * @return Vector of pointers to all vertices.
     * @complexity O(V) to copy the vector.
     */
    std::vector<Vertex<T> *> getVertexSet() const;

protected:
    std::vector<Vertex<T> *> vertexSet; // vertex set

    double **distMatrix = nullptr; // dist matrix for Floyd-Warshall
    int **pathMatrix = nullptr;    // path matrix for Floyd-Warshall

    /**
     * @brief Finds the index of the vertex with a given content.
     * @param in The content to search for.
     * @return The index in vertexSet, or -1 if not found.
     * @complexity O(V) linear search.
     */
    int findVertexIdx(const T &in) const;
};

void deleteMatrix(int **m, int n);
void deleteMatrix(double **m, int n);

/************************* Vertex  **************************/

// Vertex constructor: stores the given content.
template <class T>
Vertex<T>::Vertex(T in) : info(in) {}

template <class T>
Vertex<T>::~Vertex()
{
    for (auto edge : adj)
    {
        delete edge;
    }
}

template <class T>
Edge<T> *Vertex<T>::addEdge(Vertex<T> *d, double w)
{
    auto newEdge = new Edge<T>(this, d, w);
    adj.push_back(newEdge);
    d->incoming.push_back(newEdge);
    return newEdge;
}

// Remove all outgoing edges to a vertex with given destination content.
template <class T>
bool Vertex<T>::removeEdge(T in)
{
    bool removedEdge = false;
    auto it = adj.begin();
    while (it != adj.end())
    {
        Edge<T> *edge = *it;
        Vertex<T> *dest = edge->getDest();
        if (dest->getInfo() == in)
        {
            it = adj.erase(it);
            deleteEdge(edge);
            removedEdge = true; // allows for multiple edges to connect the same pair of vertices (multigraph)
        }
        else
        {
            it++;
        }
    }
    return removedEdge;
}

// Remove all outgoing edges from this vertex.
template <class T>
void Vertex<T>::removeOutgoingEdges()
{
    auto it = adj.begin();
    while (it != adj.end())
    {
        Edge<T> *edge = *it;
        it = adj.erase(it);
        deleteEdge(edge);
    }
}

template <class T>
T Vertex<T>::getInfo() const
{
    return this->info;
}

template <class T>
int Vertex<T>::getLow() const
{
    return this->low;
}

template <class T>
void Vertex<T>::setLow(int value)
{
    this->low = value;
}

template <class T>
int Vertex<T>::getNum() const
{
    return this->num;
}

template <class T>
void Vertex<T>::setNum(int value)
{
    this->num = value;
}

template <class T>
std::vector<Edge<T> *> Vertex<T>::getAdj() const
{
    return this->adj;
}

template <class T>
bool Vertex<T>::isVisited() const
{
    return this->visited;
}

template <class T>
bool Vertex<T>::isProcessing() const
{
    return this->processing;
}

template <class T>
unsigned int Vertex<T>::getIndegree() const
{
    return this->indegree;
}

template <class T>
double Vertex<T>::getDist() const
{
    return this->dist;
}

template <class T>
Edge<T> *Vertex<T>::getPath() const
{
    return this->path;
}

template <class T>
std::vector<Edge<T> *> Vertex<T>::getIncoming() const
{
    return this->incoming;
}

template <class T>
void Vertex<T>::setInfo(T in)
{
    this->info = in;
}

template <class T>
void Vertex<T>::setVisited(bool visited)
{
    this->visited = visited;
}

template <class T>
void Vertex<T>::setProcessing(bool processing)
{
    this->processing = processing;
}

template <class T>
void Vertex<T>::setIndegree(unsigned int indegree)
{
    this->indegree = indegree;
}

template <class T>
void Vertex<T>::setDist(double dist)
{
    this->dist = dist;
}

template <class T>
void Vertex<T>::setPath(Edge<T> *path)
{
    this->path = path;
}

template <class T>
void Vertex<T>::deleteEdge(Edge<T> *edge)
{
    Vertex<T> *dest = edge->getDest();
    // Remove the corresponding edge from the incoming list
    auto it = dest->incoming.begin();
    while (it != dest->incoming.end())
    {
        if ((*it)->getOrig()->getInfo() == info)
        {
            it = dest->incoming.erase(it);
        }
        else
        {
            it++;
        }
    }
    delete edge;
}

/********************** Edge  ****************************/

// Edge constructor: stores origin, destination, and weight.
template <class T>
Edge<T>::Edge(Vertex<T> *orig, Vertex<T> *dest, double w) : dest(dest), weight(w), orig(orig) {}

template <class T>
Vertex<T> *Edge<T>::getDest() const
{
    return this->dest;
}

template <class T>
double Edge<T>::getWeight() const
{
    return this->weight;
}

template <class T>
Vertex<T> *Edge<T>::getOrig() const
{
    return this->orig;
}

template <class T>
Edge<T> *Edge<T>::getReverse() const
{
    return this->reverse;
}

template <class T>
bool Edge<T>::isSelected() const
{
    return this->selected;
}

template <class T>
double Edge<T>::getFlow() const
{
    return flow;
}

template <class T>
void Edge<T>::setSelected(bool selected)
{
    this->selected = selected;
}

template <class T>
void Edge<T>::setReverse(Edge<T> *reverse)
{
    this->reverse = reverse;
}

template <class T>
void Edge<T>::setFlow(double flow)
{
    this->flow = flow;
}

/********************** Graph  ****************************/

template <class T>
int Graph<T>::getNumVertex() const
{
    return vertexSet.size();
}

template <class T>
std::vector<Vertex<T> *> Graph<T>::getVertexSet() const
{
    return vertexSet;
}

// Find a vertex by content.
template <class T>
Vertex<T> *Graph<T>::findVertex(const T &in) const
{
    for (auto v : vertexSet)
        if (v->getInfo() == in)
            return v;
    return nullptr;
}

// Find the index of a vertex by content.
template <class T>
int Graph<T>::findVertexIdx(const T &in) const
{
    for (unsigned i = 0; i < vertexSet.size(); i++)
        if (vertexSet[i]->getInfo() == in)
            return i;
    return -1;
}
// Add a vertex with given content; returns false if it already exists.
template <class T>
bool Graph<T>::addVertex(const T &in)
{
    if (findVertex(in) != nullptr)
        return false;
    vertexSet.push_back(new Vertex<T>(in));
    return true;
}

// Remove a vertex and all its incident edges.
template <class T>
bool Graph<T>::removeVertex(const T &in)
{
    for (auto it = vertexSet.begin(); it != vertexSet.end(); it++)
    {
        if ((*it)->getInfo() == in)
        {
            auto v = *it;
            v->removeOutgoingEdges();
            for (auto u : vertexSet)
            {
                u->removeEdge(v->getInfo());
            }
            vertexSet.erase(it);
            delete v;
            return true;
        }
    }
    return false;
}

// Add a directed edge between two existing vertices.
template <class T>
bool Graph<T>::addEdge(const T &sourc, const T &dest, double w)
{
    auto v1 = findVertex(sourc);
    auto v2 = findVertex(dest);
    if (v1 == nullptr || v2 == nullptr)
        return false;
    v1->addEdge(v2, w);
    return true;
}

// Remove a directed edge by source and destination content.
template <class T>
bool Graph<T>::removeEdge(const T &sourc, const T &dest)
{
    Vertex<T> *srcVertex = findVertex(sourc);
    if (srcVertex == nullptr)
    {
        return false;
    }
    return srcVertex->removeEdge(dest);
}

template <class T>
bool Graph<T>::addBidirectionalEdge(const T &sourc, const T &dest, double w)
{
    auto v1 = findVertex(sourc);
    auto v2 = findVertex(dest);
    if (v1 == nullptr || v2 == nullptr)
        return false;
    auto e1 = v1->addEdge(v2, w);
    auto e2 = v2->addEdge(v1, w);
    e1->setReverse(e2);
    e2->setReverse(e1);
    return true;
}

inline void deleteMatrix(int **m, int n)
{
    if (m != nullptr)
    {
        for (int i = 0; i < n; i++)
            if (m[i] != nullptr)
                delete[] m[i];
        delete[] m;
    }
}

inline void deleteMatrix(double **m, int n)
{
    if (m != nullptr)
    {
        for (int i = 0; i < n; i++)
            if (m[i] != nullptr)
                delete[] m[i];
        delete[] m;
    }
}

template <class T>
Graph<T>::Graph(Graph<T> &&other) noexcept : vertexSet(std::move(other.vertexSet)), distMatrix(other.distMatrix), pathMatrix(other.pathMatrix)
{
    other.vertexSet.clear();
    other.distMatrix = nullptr;
    other.pathMatrix = nullptr;
}

template <class T>
Graph<T> &Graph<T>::operator=(Graph<T> &&other) noexcept
{
    if (this != &other)
    {
        deleteMatrix(distMatrix, vertexSet.size());
        deleteMatrix(pathMatrix, vertexSet.size());
        for (auto v : vertexSet)
            delete v;

        vertexSet = std::move(other.vertexSet);
        distMatrix = other.distMatrix;
        pathMatrix = other.pathMatrix;

        other.vertexSet.clear();
        other.distMatrix = nullptr;
        other.pathMatrix = nullptr;
    }
    return *this;
}

template <class T>
Graph<T>::~Graph()
{
    deleteMatrix(distMatrix, vertexSet.size());
    deleteMatrix(pathMatrix, vertexSet.size());
    for (auto v : vertexSet)
    {
        delete v;
    }
}

#endif /* DA_TP_CLASSES_GRAPH */