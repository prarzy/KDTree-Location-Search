#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>

// Constants and Definitions
#define MAX_NAME_LEN 100
#define MAX_CAT_LEN 50
#define PI 3.141592653
#define EARTH_RADIUS 6371.0 
#define MAX_LINE_LEN 1024

// Data Structures

typedef struct{
    char amenity[MAX_CAT_LEN];
    char name[MAX_NAME_LEN];
    double lat;
    double lon; 

    double x;
    double y;
    double z;
}Point;

typedef struct KDNode{
    Point point;
    struct KDNode *left;
    struct KDNode *right;
    int axis; // 0=x, 1=y, 2=z
}KDNode;

// Math Helper Functions

double to_radians(double degrees) {
    return degrees * PI / 180.0;
}

double lat_lon_to_ecef(double lat, double lon, double *x, double *y, double *z) {
    double lat_rad = to_radians(lat);
    double lon_rad = to_radians(lon);
    
    *x = EARTH_RADIUS * cos(lat_rad) * cos(lon_rad);
    *y = EARTH_RADIUS * cos(lat_rad) * sin(lon_rad);
    *z = EARTH_RADIUS * sin(lat_rad);
}

int compare_x(const void *a, const void *b) {
    double diff = ((Point *)a)->x - ((Point *)b)->x;
    return (diff > 0) - (diff < 0);
}
int compare_y(const void *a, const void *b) {
    double diff = ((Point *)a)->y - ((Point *)b)->y;
    return (diff > 0) - (diff < 0);
}
int compare_z(const void *a, const void *b) {
    double diff = ((Point *)a)->z - ((Point *)b)->z;
    return (diff > 0) - (diff < 0);
}
// CSV Parsing

void parse_csv_line(char *line, Point *p) {
    char *cursor = line;
    int i = 0;

    while (*cursor && *cursor != ',') {
        if (i < MAX_CAT_LEN - 1) p->amenity[i++] = *cursor;
        cursor++;
    }
    p->amenity[i] = '\0';
    if (*cursor == ',') cursor++;

    i = 0;
    if (*cursor == '"') {
        cursor++; // Skip opening quote
        while (*cursor && *cursor != '"') {
            if (i < MAX_NAME_LEN - 1) p->name[i++] = *cursor;
            cursor++;
        }
        cursor++; // Skip closing quote
        if (*cursor == ',') cursor++;
    } else {
        while (*cursor && *cursor != ',') {
            if (i < MAX_NAME_LEN - 1) p->name[i++] = *cursor;
            cursor++;
        }
        if (*cursor == ',') cursor++;
    }
    p->name[i] = '\0';

    // Lon & Lat
    p->lon = strtod(cursor, &cursor);
    if (*cursor == ',') cursor++;
    p->lat = strtod(cursor, NULL);

    // Convert to 3D
    lat_lon_to_ecef(p->lat, p->lon, &p->x, &p->y, &p->z);
}
int load_points(const char *filename, Point **points_array){
    FILE *file = fopen(filename, "r");
    if (!file) return 0;

    int capacity = 100;
    int count = 0;
    *points_array = malloc(sizeof(Point) * capacity);

    char line[MAX_LINE_LEN];
    fgets(line, MAX_LINE_LEN, file); // Skip header

    while (fgets(line, MAX_LINE_LEN, file)) {
        if (count >= capacity) {
            capacity *= 2;
            *points_array = realloc(*points_array, sizeof(Point) * capacity);
        }
        parse_csv_line(line, &(*points_array)[count]);
        count++;
    }
    fclose(file);
    return count;
}

// KD tree logic
KDNode* build_kdtree(Point *points, int n, int depth){
    if (n <= 0) return NULL;
    int axis = depth % 3;

    if (axis == 0) qsort(points, n, sizeof(Point), compare_x);
    else if (axis == 1) qsort(points, n, sizeof(Point), compare_y);
    else qsort(points, n, sizeof(Point), compare_z);

    int mid = n / 2;
    KDNode *node = malloc(sizeof(KDNode));
    node->point = points[mid];
    node->axis = axis;
    node->left = build_kdtree(points, mid, depth + 1);
    node->right = build_kdtree(points + mid + 1, n - mid - 1, depth + 1);

    return node;
}

// Loading CSV file

// int load_csv(const char *filename, Location arr[], int max_size) {
//     FILE *file = fopen(filename, "r");
//     if (!file) {
//         perror("Could not open file");
//         return -1;
//     }

//     char line[256];
//     int count = 0;

//     while (fgets(line, sizeof(line), file) && count < max_size) {
//         sscanf(line, "%49[^,],%49[^,],%lf,%lf", arr[count].type, arr[count].name, &arr[count].lat, &arr[count].lon);
//         count++;
//     }

//     fclose(file);
//     return count;
// }

// KD-Tree Functions

// KDNode* create_node(Location loc) {
//     KDNode* node = (KDNode*)malloc(sizeof(KDNode));
//     node->loc = loc;
//     node->left = NULL;
//     node->right = NULL;
//     return node;
// }
