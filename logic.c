#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>

#define MAX_NAME_LEN 100
#define MAX_CAT_LEN 50
#define PI 3.141592653
#define EARTH_RADIUS 6371.0
#define MAX_LINE_LEN 1024
#define K 5

// Data structures
typedef struct {
    char amenity[MAX_CAT_LEN];
    char name[MAX_NAME_LEN];
    double lat;
    double lon;
    double x, y, z;
} Point;

typedef struct KDNode {
    Point point;
    struct KDNode *left;
    struct KDNode *right;
    int axis;
} KDNode;

typedef struct {
    KDNode *node;
    double dist_sq;
} Result;

// Math helpers
double to_radians(double degrees){ return degrees * PI / 180.0; }
void lat_lon_to_ecef(double lat, double lon, double *x, double *y, double *z){
    double lat_rad = to_radians(lat), lon_rad = to_radians(lon);
    *x = EARTH_RADIUS * cos(lat_rad) * cos(lon_rad);
    *y = EARTH_RADIUS * cos(lat_rad) * sin(lon_rad);
    *z = EARTH_RADIUS * sin(lat_rad);
}
double dist_sq(Point p1, Point p2){
    return (p1.x-p2.x)*(p1.x-p2.x)+(p1.y-p2.y)*(p1.y-p2.y)+(p1.z-p2.z)*(p1.z-p2.z);
}

// String helpers
void to_lowercase(char *str){ for(int i=0; str[i]; i++) if(str[i]>='A' && str[i]<='Z') str[i]+='a'-'A'; }

// CSV parsing
void parse_csv_line(char *line, Point *p){
    char *cursor=line; int i=0;
    while(*cursor && *cursor!=','){ if(i<MAX_CAT_LEN-1)p->amenity[i++]=*cursor; cursor++; }
    p->amenity[i]='\0'; if(*cursor==',') cursor++;
    i=0;
    if(*cursor=='"'){ cursor++; while(*cursor && *cursor!='"'){ if(i<MAX_NAME_LEN-1)p->name[i++]=*cursor; cursor++; } cursor++; if(*cursor==',') cursor++; }
    else{ while(*cursor && *cursor!=','){ if(i<MAX_NAME_LEN-1)p->name[i++]=*cursor; cursor++; } if(*cursor==',') cursor++; }
    p->name[i]='\0';
    p->lon=strtod(cursor,&cursor); if(*cursor==',') cursor++; p->lat=strtod(cursor,NULL);
    lat_lon_to_ecef(p->lat,p->lon,&p->x,&p->y,&p->z);
}

int load_points(const char *filename, Point **points_array){
    FILE *file=fopen(filename,"r"); if(!file)return 0;
    int capacity=100,count=0; *points_array=malloc(sizeof(Point)*capacity);
    char line[MAX_LINE_LEN]; fgets(line,MAX_LINE_LEN,file);
    while(fgets(line,MAX_LINE_LEN,file)){
        if(count>=capacity){ capacity*=2; *points_array=realloc(*points_array,sizeof(Point)*capacity); }
        parse_csv_line(line,&(*points_array)[count]); count++;
    }
    fclose(file); return count;
}

// KD tree
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
KDNode* build_kdtree(Point *points,int n,int depth){
    if(n<=0) return NULL;
    int axis=depth%3;
    qsort(points,n,sizeof(Point),(axis==0?compare_x:(axis==1?compare_y:compare_z)));
    int mid=n/2;
    KDNode *node=malloc(sizeof(KDNode));
    node->point=points[mid]; node->axis=axis;
    node->left=build_kdtree(points,mid,depth+1);
    node->right=build_kdtree(points+mid+1,n-mid-1,depth+1);
    return node;
}
void free_kdtree(KDNode *node){ if(!node)return; free_kdtree(node->left); free_kdtree(node->right); free(node); }

// Nearest neighbor
void try_insert(Result res[], KDNode *node, double d){
    for(int i=0;i<K;i++){
        if(!res[i].node || d<res[i].dist_sq){
            for(int j=K-1;j>i;j--) res[j]=res[j-1];
            res[i].node=node; res[i].dist_sq=d; break;
        }
    }
}
void find_k_nearest(KDNode *root, Point target, const char *category, Result res[]){
    if(!root) return;
    double d=dist_sq(root->point,target);
    if(strcmp(category,"all")==0 || strcmp(root->point.amenity,category)==0) try_insert(res,root,d);
    double diff=(root->axis==0?target.x-root->point.x:(root->axis==1?target.y-root->point.y:target.z-root->point.z));
    KDNode *near=(diff<0?root->left:root->right), *far=(diff<0?root->right:root->left);
    find_k_nearest(near,target,category,res);
    double worst=res[K-1].node?res[K-1].dist_sq:1e18;
    if(diff*diff<worst) find_k_nearest(far,target,category,res);
}

int main(int argc, char *argv[]){
    if(argc < 4){
        fprintf(stderr,"Usage: %s <lat> <lon> <category> [radius_km]\n", argv[0]);
        return 1;
    }
    // Parse inputs
    double lat = atof(argv[1]);
    double lon = atof(argv[2]);
    char category[MAX_CAT_LEN]; strncpy(category, argv[3], MAX_CAT_LEN-1); category[MAX_CAT_LEN-1] = '\0'; to_lowercase(category);

    double radius_km = -1; // default: no radius
    if(argc == 5) radius_km = atof(argv[4]);

    Point *points = NULL;
    int count = load_points("coordinates.csv", &points);
    if(count == 0){ fprintf(stderr,"No points loaded\n"); return 1; }
    
    // Convert target to ECEF
    Point target = {0}; target.lat = lat; target.lon = lon;
    lat_lon_to_ecef(lat, lon, &target.x, &target.y, &target.z);

    printf("[");
    int first = 1;

    if(radius_km > 0){
        // Find all points within radius
        for(int i = 0; i < count; i++){
            if(strcmp(category, "all") == 0 || strcmp(points[i].amenity, category) == 0){
                double dist = sqrt(dist_sq(points[i], target));
                if(dist <= radius_km){
                    if(!first) printf(",");
                    printf("{\"name\":\"%s\",\"amenity\":\"%s\",\"lat\":%.6f,\"lon\":%.6f,\"dist\":%.2f}",
                        points[i].name, points[i].amenity, points[i].lat, points[i].lon, dist);
                    first = 0;
                }
            }
        }
    } else {
        // Original: top K nearest using KD-tree
        KDNode *root = build_kdtree(points, count, 0);
        Result res[K] = {0};
        find_k_nearest(root, target, category, res);
        for(int i = 0; i < K; i++){
            if(res[i].node){
                double dist = sqrt(res[i].dist_sq);
                if(!first) printf(",");
                printf("{\"name\":\"%s\",\"amenity\":\"%s\",\"lat\":%.6f,\"lon\":%.6f,\"dist\":%.2f}",
                    res[i].node->point.name, res[i].node->point.amenity, res[i].node->point.lat, res[i].node->point.lon, dist);
                first = 0;
            }
        }
        free_kdtree(root);
    }

    printf("]\n");
    free(points);
    return 0;
}

