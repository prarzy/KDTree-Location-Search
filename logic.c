#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>

// Data Structures

typedef struct{
    char type[50];
    char name[50];
    double lat;
    double lon; 
}Location;

typedef struct KDNode{
    Location loc;
    struct KDNode *left;
    struct KDNode *right;
}KDNode;

// Loading CSV file

int load_csv(const char *filename, Location arr[], int max_size) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Could not open file");
        return -1;
    }

    char line[256];
    int count = 0;

    while (fgets(line, sizeof(line), file) && count < max_size) {
        sscanf(line, "%49[^,],%49[^,],%lf,%lf", arr[count].type, arr[count].name, &arr[count].lat, &arr[count].lon);
        count++;
    }

    fclose(file);
    return count;
}

// KD-Tree Functions

KDNode* create_node(Location loc) {
    KDNode* node = (KDNode*)malloc(sizeof(KDNode));
    node->loc = loc;
    node->left = NULL;
    node->right = NULL;
    return node;
}
