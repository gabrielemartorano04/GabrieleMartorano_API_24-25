#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <stdint.h>
#include <math.h>

#define MAX_LINE 256
#define INF 1000000000
#define MAX_EDGE_COST 100

typedef struct {
    int dest_x;
    int dest_y;
    int cost;
} AirRoute;

typedef struct {
    int ground_cost;
    int air_route_count;
    AirRoute air_routes[5];
} Hexagon;

typedef struct {
    int width;
    int height;
    Hexagon **grid;
} HexMap;

typedef struct {
    int x, y;
    int dist;
} QueueNode;

typedef struct {
    int x, y;
    int cost;
    int mode;
} Node;

HexMap *map = NULL;
static int heap_cap = 0;
static int *dist_flat = NULL;
static QueueNode *bfs_queue = NULL;
static bool *bfs_visited = NULL;
static int bfs_cap = 0;

static int *bucket_head = NULL;
static int *bucket_next = NULL;

static const int dx_even[6] = { +1,  0, -1, -1, -1,  0 };
static const int dy_even[6] = {  0, +1, +1,  0, -1, -1 };

static const int dx_odd [6] = { +1, +1,  0, -1,  0, +1 };
static const int dy_odd [6] = {  0, +1, +1,  0, -1, -1 };

void free_map();
void init(int width, int height);
void change_cost(int x, int y, int v, int raggio);
void toggle_air_route(int x1, int y1, int x2, int y2);
void travel_cost(int x1, int y1, int x2, int y2);
static inline void push(int ui, int d, int B);

int main(int argc, char const *argv[]) {
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), stdin)) {
        char command[32];
        int args[4];
        if (sscanf(line, "%31s", command) != 1) continue;
        if (strcmp(command, "init") == 0 && sscanf(line, "%*s %d %d", &args[0], &args[1]) == 2) {
            init(args[0], args[1]);
        }
        else if (strcmp(command, "change_cost") == 0 && sscanf(line, "%*s %d %d %d %d", &args[0], &args[1], &args[2], &args[3]) == 4) {
            change_cost(args[0], args[1], args[2], args[3]);
        }
        else if (strcmp(command, "toggle_air_route") == 0 && sscanf(line, "%*s %d %d %d %d", &args[0], &args[1], &args[2], &args[3]) == 4) {
            toggle_air_route(args[0], args[1], args[2], args[3]);
        }
        else if (strcmp(command, "travel_cost") == 0 && sscanf(line, "%*s %d %d %d %d", &args[0], &args[1], &args[2], &args[3]) == 4) {
            travel_cost(args[0], args[1], args[2], args[3]);
        }

        fflush(stdout);
    }

    return 0;
}

void free_map() {
    if (!map) {
        return;
    }
    for (int y = 0; y < map->height; y++) {
        free(map->grid[y]);
    }
    free(map->grid);
    free(map);
    free(dist_flat);
    heap_cap = 0;
    dist_flat = NULL;
    free(bfs_queue);
    free(bfs_visited);
    bfs_queue = NULL;
    bfs_visited = NULL;
    bfs_cap = 0;

    free(bucket_head);
    free(bucket_next);
    bucket_head = NULL;
    bucket_next = NULL;

    map = NULL;
}

void init(int width, int height) {
    free_map();

    map = malloc(sizeof(HexMap));
    map->width = width;
    map->height = height;
    map->grid = malloc(sizeof(Hexagon*) * height);

    for (int y = 0; y < height; y++) {
        map->grid[y] = malloc(sizeof(Hexagon) * width);
        for (int x = 0; x < width; x++) {
            map->grid[y][x].ground_cost = 1;
            map->grid[y][x].air_route_count = 0;
        }
    }

    heap_cap = (size_t)width * height * 2;
    dist_flat = malloc(sizeof *dist_flat * heap_cap);
    bfs_cap = (size_t)width * height;
    bfs_queue = malloc(sizeof *bfs_queue * bfs_cap);
    bfs_visited = malloc(sizeof *bfs_visited * bfs_cap);

    bucket_head = malloc(sizeof *bucket_head * (MAX_EDGE_COST + 1));
    bucket_next = malloc(sizeof *bucket_next * heap_cap);

    printf("OK\n");
}

void change_cost(int x, int y, int v, int raggio) {
    if (!map || !map->grid
        || x < 0 || x >= map->width
        || y < 0 || y >= map->height
        || raggio <= 0) {
        printf("KO\n");
        return;
    }

    if(v > 10 || v < -10) {
        printf("KO\n");
        return;
    }

    int W = map->width, H = map->height;

    int delta_array[raggio];
    for (int d = 0; d < raggio; ++d) {
        float num = (float)(raggio - d) / (float)raggio;
        delta_array[d] = (int)floorf(v * num);
    }

    for (int i = 0; i < bfs_cap; ++i) {
        bfs_visited[i] = false;
    }

    int front = 0, back = 0;
    bfs_queue[back++] = (QueueNode){ x, y, 0 };
    bfs_visited[y * W + x] = true;

    while (front < back) {
        QueueNode curr = bfs_queue[front++];

        if(curr.dist >= raggio) continue;

        int delta = delta_array[curr.dist];

        Hexagon *h = &map->grid[curr.y][curr.x];
        int gc = h->ground_cost + (int)floorf(delta);
        if (gc < 0) gc = 0;
        else if (gc > 100) gc = 100;
        h->ground_cost = gc;
        for (int i = 0; i < h->air_route_count; i++) {
            int cc = h->air_routes[i].cost + (int)floorf(delta);
            if (cc < 0) cc = 0;
            else if (cc > 100) cc = 100;
            h->air_routes[i].cost = cc;
        }

        const int *dx = (curr.y % 2 == 0) ? dx_even : dx_odd;
        const int *dy = (curr.y % 2 == 0) ? dy_even : dy_odd;

        for (int d = 0; d < 6; ++d) {
            int nx = curr.x + dx[d], ny = curr.y + dy[d];
            
            if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
            int idx = ny * W + nx;
            if(!bfs_visited[idx]) {
                bfs_visited[idx] = true;
                bfs_queue[back++] = (QueueNode){ nx, ny, curr.dist + 1 };
            }
        }
    }
    printf("OK\n");
}

void toggle_air_route(int x1, int y1, int x2, int y2) {
    if (!map ||
        x1 < 0 || x1 >= map->width || y1 < 0 || y1 >= map->height ||
        x2 < 0 || x2 >= map->width || y2 < 0 || y2 >= map->height) {
        printf("KO\n");
        return;
    }

    Hexagon *from = &map->grid[y1][x1];

    for (int i = 0; i < from->air_route_count; i++) {
        if (from->air_routes[i].dest_x == x2 && from->air_routes[i].dest_y == y2) {
            from->air_routes[i] = from->air_routes[from->air_route_count - 1];
            from->air_route_count--;
            printf("OK\n");
            return;
        }
    }

    if (from->air_route_count >= 5) {
        printf("KO\n");
        return;
    }

    int somma = from->ground_cost;
    for (int i = 0; i < from->air_route_count; i++) {
        somma += from->air_routes[i].cost;
    }
    int n = from->air_route_count + 1;
    int media = somma / n;

    if (media < 0) {
        media = 0;
    }else if (media > 100) {
        media = 100;
    }

    from->air_routes[from->air_route_count] = (AirRoute){x2, y2, media};
    from->air_route_count++;

    printf("OK\n");
}

void travel_cost(int x1, int y1, int x2, int y2) {
    if (!map || x1<0||x1>=map->width||y1<0||y1>=map->height||
        x2<0||x2>=map->width||y2<0||y2>=map->height) {
        printf("-1\n");
        return;
    }
    if (x1==x2 && y1==y2) {
        printf("0\n");
        return;
    }

    const int W = map->width, H = map->height;
    const int N = W * H * 2;
    const int B = MAX_EDGE_COST + 1;

    for (int i = 0; i < N; ++i) {
        dist_flat[i] = INF;
    }
    for (int b = 0; b < B; ++b) bucket_head[b] = -1;

    int curr_bucket = 0;
    int curr_dist   = 0;
    int pending     = 0;

    int start = (y1 * W + x1) * 2 + 0;
    dist_flat[start] = 0;
    push(start, 0, B);
    pending++;

    while (pending > 0) {
        while (pending > 0 && bucket_head[curr_bucket] == -1) {
            if (++curr_bucket == B) curr_bucket = 0;
            curr_dist++;
        }
        if (pending == 0) break;

        int ui = bucket_head[curr_bucket];
        bucket_head[curr_bucket] = bucket_next[ui];
        bucket_next[ui] = -1;
        pending--;

        int cell = ui >> 1;
        int cx = cell % W;
        int cy = cell / W;

        if (cx == x2 && cy == y2) break;

        if (dist_flat[ui] != curr_dist) continue;

        Hexagon *hex = &map->grid[cy][cx];
        int base_cost = dist_flat[ui];

        if (hex->ground_cost == 0) continue;

        for (int i = 0; i < hex->air_route_count; ++i) {
            int nx = hex->air_routes[i].dest_x;
            int ny = hex->air_routes[i].dest_y;
            int rc = hex->air_routes[i].cost;
            if (rc <= 0) continue;

            int nd = base_cost + rc;
            int vi = (ny * W + nx) * 2 + 1;
            if (nd < dist_flat[vi]) {
                dist_flat[vi] = nd;
                push(vi, nd, B);
                pending++;
            }
        }

        if (hex->ground_cost > 0) {
            const int *dx = (cy % 2 == 0) ? dx_even : dx_odd;
            const int *dy = (cy % 2 == 0) ? dy_even : dy_odd;
            int step = hex->ground_cost;

            for (int d = 0; d < 6; ++d) {
                int nx = cx + dx[d];
                int ny = cy + dy[d];
                if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;

                int nd = base_cost + step;
                int vi = (ny * W + nx) * 2 + 0;
                if (nd < dist_flat[vi]) {
                    dist_flat[vi] = nd;
                    push(vi, nd, B);
                    pending++;
                }
            }
        }
    }

    int idx0 = (y2 * W + x2) * 2 + 0;
    int idx1 = idx0 + 1;
    int d0 = dist_flat[idx0];
    int d1 = dist_flat[idx1];

    int best;
    if (d0 == INF && d1 == INF) best = -1;
    else if (d0 == INF) best = d1;
    else if (d1 == INF) best = d0;
    else best = (d0 < d1 ? d0 : d1);

    printf("%d\n", best);
}

static inline void push(int ui, int d, int B) {
    int idx = d % B;
    bucket_next[ui] = bucket_head[idx];
    bucket_head[idx] = ui;
}