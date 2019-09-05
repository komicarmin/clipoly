#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include "events.h"

int main() {
    int operation = 1;

    reg_t *polygons = NULL;
    if(_argc == 3) {
        ReadFile(_argv[1], &polygons);
        char *op = _argv[2];
        operation = atoi(op);
    }
    else {
        printf("Program is expecting two command line arguments ([file-name].txt [operationNum]).");
        return 1;
    }

    event_t *event_root = NULL;
    event_t *event_root2 = NULL;
    event_t *event_root_res = NULL;
    status_root = NULL;

    clock_t t1 = clock();
    polyToEvents(&event_root, &polygons->first, true);
    seg_t *segments = eventLoop(&event_root, true, false, false);
    polygons = polygons->next;
    while (polygons) {
        polyToEvents(&event_root2, &polygons->first, true);
        seg_t *segments2 = eventLoop(&event_root2, true, false, false);
        segmentsToEvents(&event_root_res, segments, true);
        segmentsToEvents(&event_root_res, segments2, false);
        seg_t *segmentsRes = eventLoop(&event_root_res, false, false, false);
        segments = select(segmentsRes, operation);
        polygons = polygons->next;
    }
    reg_t *res = merge(segments);
    clock_t t2 = clock();

    while(res != NULL) {
        dumpPoly(res->first);
        printf("\n");
        res = res->next;
    }

    double elapsed = (double) (t2 - t1) / CLOCKS_PER_SEC;
    printf("\nTime elapsed: %.5f\n", elapsed);

    return 0;
}
