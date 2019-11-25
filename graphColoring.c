#include <stdio.h>
#include <stdlib.h>
#include "VirtualLibrary.h"
     
ProgramInformation info;
//int addresses[200];

struct node
{
	int vertex;
    struct node* next;
    int memDir;
};
struct node* createNode(int);
struct Graph
{
    int numVertices;
    struct node** adjLists;
    int memDir;
    int memDirList;
};
struct Graph* createGraph(int vertices);
void graphColoring(struct Graph* graph, int numV);
void addEdge(struct Graph* graph, int src, int dest);
void printGraph(struct Graph* graph);
int main()
{
	info = setupProgram();
	
	int c;
	
    struct Graph* graph = createGraph(4);
    
    printf("Se creo el grafo"); 
    scanf("%d", &c);
   
    addEdge(graph, 0, 1); 
    addEdge(graph, 0, 3);
    addEdge(graph, 1, 2); 
    addEdge(graph, 2, 3);
    
    printf("Se crearon los edges");
    scanf("%d", &c);
    
    

     
    
    printGraph(graph);
    
    printf("Se imprimio el grafo");
    scanf("%d", &c);
    
    
    printf("Termina print\n");
    
    graphColoring(graph,1);
    
    printf("Se coloreo el grafo");
    scanf("%d", &c);
 
	closeProgram(info);
	
    return 0;
}
 
struct node* createNode(int v)
{
	int x_dir = allocate(info, sizeof(struct node));
	
    struct node* newNode = (struct node*) accessMemory(info,x_dir, WRITE_FLAG);
    newNode->vertex = v;
    newNode->next = NULL;
    newNode->memDir = x_dir;
    return newNode;
} 
 
/*
struct node* createNode(int v)
{
    struct node* newNode = malloc(sizeof(struct node));
    newNode->vertex = v;
    newNode->next = NULL;
    return newNode;
}*/
  
struct Graph* createGraph(int vertices)
{
	int x_dirGraph = allocate(info, sizeof(struct node));
	int x_dirAdjList = allocate(info, vertices * sizeof(struct node*));
	
    struct Graph* graph = (struct Graph*) accessMemory(info,x_dirGraph, WRITE_FLAG);
    graph->numVertices = vertices;
    graph->adjLists = (struct node**) accessMemory(info, x_dirAdjList, WRITE_FLAG);
  
    int i;
    for (i = 0; i < vertices; i++)
        graph->adjLists[i] = NULL;
 
    return graph;
}  
   
/*
struct Graph* createGraph(int vertices)
{
     struct Graph* graph = malloc(sizeof(struct Graph));
     graph->numVertices = vertices;
  
     graph->adjLists = malloc(vertices * sizeof(struct node*));
  
     int i;
     for (i = 0; i < vertices; i++)
         graph->adjLists[i] = NULL;
 
     return graph;
}*/
 
void addEdge(struct Graph* graph, int src, int dest)
{
     // Add edge from src to dest
     struct node* newNode = createNode(dest);
     
     //Making sure page is loaded
     int x_dirGraphLists = graph->memDirList;
     int x_dirNode = newNode->memDir;
     
     accessMemory(info, x_dirGraphLists, READ_FLAG);
     accessMemory(info,x_dirNode, READ_FLAG);
     newNode->next = graph->adjLists[src];
     graph->adjLists[src] = newNode;
 
	 
     // Add edge from dest to src
     newNode = createNode(src);
     x_dirNode = newNode->memDir;
     //Making sure page is loaded
     accessMemory(info, x_dirGraphLists, READ_FLAG);
     accessMemory(info,x_dirNode, READ_FLAG);
     
     newNode->next = graph->adjLists[dest];
     graph->adjLists[dest] = newNode;
}
     
void printGraph(struct Graph* graph)
{
     int v;
     
     //Making sure page is loaded
     int x_dirGraphLists = graph->memDirList;
     accessMemory(info, x_dirGraphLists, READ_FLAG);
     
     printf("Number de vertices %d", graph->numVertices);
     for (v = 0; v < graph->numVertices; v++)
     {
		 
		 printf("Entra al for");
         struct node* temp = graph->adjLists[v];
         
         //Making sure node page is loaded
         int x_dirNode = temp->memDir;
		 accessMemory(info, x_dirNode, READ_FLAG);
         
         printf("\n Adjacency list of vertex %d\n ", v);
         while (temp)
         {
			 //Making sure node page is loaded
			 x_dirNode = temp->memDir;
			 accessMemory(info, x_dirNode, READ_FLAG);
			 
             printf("%d -> ", temp->vertex);
             temp = temp->next;
         } 
         printf("Cambia vertice\n");
     }
     printf("Salio vivo");
}

void graphColoring(struct Graph* graph, int numV)
{
	printf("Empieza\n");
	
	//Making sure graph is loaded
	int x_dirGraph = graph->memDir;
    accessMemory(info, x_dirGraph, READ_FLAG);
	
	int vertices = graph->numVertices;
	int result[vertices];
	result[0]  = 0; 
	for (int v = 1; v < vertices; v++)
		result[v] =  -1;
	
	printf("Inicializo vertices sin asignar\n");
	int available[vertices];
	
	for (int color = 0; color < 3; color++) 
        available[color] = 0;
    printf("Se asignaron los posibles colores\n");
    
    int x_dirGraphLists = graph->memDirList;
    accessMemory(info, x_dirGraphLists, READ_FLAG);
    //Assign remaining colors
    for (int u = 1; u < vertices; u++) 
    { 
		struct node* temp = graph->adjLists[u];
		
		//Making sure node page is loaded
        int x_dirNode = temp->memDir;
		accessMemory(info, x_dirNode, READ_FLAG);
		 
		int index = temp->vertex;
		
        //printf("\n Adjacency list of vertex %d\n ", v);
        printf("Inicializa todo en while de asignar colores\n");
        while (temp)
        {
			//Making sure node page is loaded
			x_dirNode = temp->memDir;
			accessMemory(info, x_dirNode, READ_FLAG);
			
			index = temp->vertex;
			if (result[index] != -1)
			{
				available[result[index]] = 1; 
			}
            //printf("%d -> ", temp->vertex);
            temp = temp->next;
        }
        printf("Procesa vertices adyacentes\n");
    
        int color = -1; 
        for (int cr = 0; cr < 3 ; cr++) 
        {
            if (available[cr] == 0)
            { 
				color = cr;
                break;
			}
		}
  
        result[u] = color;
        printf("Asigna el color al vertice\n");
    
		//Making sure graph page is loaded
        x_dirGraphLists = graph->memDirList;
		accessMemory(info, x_dirGraphLists, READ_FLAG);
		 
        temp = graph->adjLists[u];
        
        //Making sure node page is loaded
        x_dirNode = temp->memDir;
		accessMemory(info, x_dirNode, READ_FLAG);
		
		index = temp->vertex;
        //printf("\n Adjacency list of vertex %d\n ", v);
        while (temp)
        {
			//Making sure each node is loaded
			x_dirNode = temp->memDir;
			accessMemory(info, x_dirNode, READ_FLAG);
			
			index = temp->vertex;
			if (result[index] != -1)
			{
				available[result[index]] = 0; 
			}
            //printf("%d -> ", temp->vertex);
            temp = temp->next;
            
        }

    }
    printf("Resuelve todo el coloring\n");
    
    int posible = 1;
    for (int u = 0; u < vertices; u++) 
        if (result[u] == -1)
        {
			printf("Este grafo no se puede resolver con 3 colores\n");
			posible=0;
		}	
    if (posible)   
		for (int u = 0; u < vertices; u++) 
			printf("Vertex %d --->  Color %d\n",u,result[u]);
	
}
