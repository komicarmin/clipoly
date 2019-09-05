typedef struct event event_t;

typedef struct point {
    double x;
    double y;
    struct point * prev;
    struct point * next;
} point_t;

typedef struct region {
    point_t *first;
    struct region *next;
} reg_t;

typedef struct segment {
    point_t * startp;
    point_t * endp;
    char myFillAbove;
    char myFillBelow;
    char otherFillAbove;
    char otherFillBelow;
    struct segment * next;
} seg_t;

typedef struct status {
    event_t *ev;
    struct status * prev;
    struct status * next;
} status_t;

typedef struct event {
    bool isStart;
    bool primary;
    point_t * pt;
    seg_t * seg;
    struct event *other;
    status_t *status;
    struct event * prev;
    struct event * next;
} event_t;

typedef struct intersection {
    point_t *pt;
    double alongA;
    double alongB;
} intersection_t;

// Global variable
status_t *status_root;

void *chkMalloc (size_t sz);
void addPoint (point_t **first, double x, double y);
int *ReadFile(char *filename, reg_t **list_head);
void dumpPoly (point_t *first);
int pointsCompare(point_t **p1, point_t **p2);
void eventAdd(event_t **ev_root, event_t **ev, point_t **other_pt);
int eventCompare(bool p1_isStart, point_t **p1_1, point_t **p1_2, bool p2_isStart, point_t **p2_1, point_t **p2_2);
bool pointAboveOrOnLine(point_t **pt, point_t **left, point_t **right);
event_t *polyToEvents(event_t **ev_root, point_t **poly, bool primary);
event_t *segmentsToEvents(event_t **ev_root, seg_t *poly, bool primary);
seg_t *eventLoop(event_t **ev_root, bool selfIntersection, bool primaryPolyInverted, bool secondaryPolyInverted);
seg_t *select(seg_t *segments, int opID);
reg_t *merge(seg_t *segments);
void reverse(point_t **head_ref);

